#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <filesystem>
#include <queue>
#include <mutex>
#include <nlohmann/json.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

// --- Data Structures ---
struct Job {
    std::string id;
    std::string type; // "process"
    json params;      // { "count": 1000, "sort_by": "age" }
};

struct HistoryEntry {
    std::string timestamp;
    std::string filename;
    std::string metadata;
};

// --- Global State ---
std::vector<HistoryEntry> g_history; // History Registry
std::queue<Job> g_jobQueue;          // Pending Jobs
std::mutex g_stateMutex;             // Protects History and Queue

// Producer Management
class Session; // Forward decl
std::shared_ptr<Session> g_activeProducer = nullptr; // Simple ptr to active producer (single for now)
bool g_producerBusy = false;

// Helpers
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

// --- WebSocket Session ---
class Session : public std::enable_shared_from_this<Session> {
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    bool isProducer_ = false;

public:
    explicit Session(tcp::socket&& socket) : ws_(std::move(socket)) {}

    void run() {
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        ws_.async_accept(beast::bind_front_handler(&Session::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec) {
        if (ec) return fail(ec, "accept");
        do_read();
    }

    void do_read() {
        ws_.async_read(buffer_, beast::bind_front_handler(&Session::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        if (ec == websocket::error::closed) {
            handle_disconnect();
            return;
        }
        if (ec) return fail(ec, "read");

        std::string msg = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());
        
        process_message(msg);
    }
    
    void process_message(const std::string& msg) {
        try {
            json j = json::parse(msg);
            std::string cmd = j["command"];
            
            // --- PRODUCER COMMANDS ---
            if (cmd == "register_producer") {
                std::lock_guard<std::mutex> lock(g_stateMutex);
                isProducer_ = true;
                g_activeProducer = shared_from_this();
                g_producerBusy = false; // Registered = Ready
                std::cout << "[Server] Producer Registered.\n";
                do_write(json({{"status", "registered"}}).dump());
                
                // Try dispatching if pending
                dispatch_next_job_unsafe();
                
            } else if (cmd == "upload") {
                // Producer finished a job
                std::cout << "[Server] Received Upload/Ack from Producer.\n";
                
                // Save Data
                saveToHistory(j["payload"]);
                
                // Mark Idle and Dispatch Next
                {
                    std::lock_guard<std::mutex> lock(g_stateMutex);
                    g_producerBusy = false;
                    dispatch_next_job_unsafe();
                }
                
                do_write(json({{"status", "ack"}}).dump());

            // --- CONSUMER COMMANDS ---
            } else if (cmd == "submit_job") {
                std::cout << "[Server] Job Submitted by Consumer.\n";
                
                Job job;
                job.id = getCurrentTimestamp(); // Simple ID
                job.type = "process";
                job.params = j["params"];
                
                {
                    std::lock_guard<std::mutex> lock(g_stateMutex);
                    g_jobQueue.push(job);
                    dispatch_next_job_unsafe();
                }
                
                do_write(json({{"status", "queued"}, {"job_id", job.id}}).dump());
                
            } else if (cmd == "list_history") {
                json arr = json::array();
                std::lock_guard<std::mutex> lock(g_stateMutex);
                for (const auto& h : g_history) {
                    arr.push_back({{"timestamp", h.timestamp}, {"filename", h.filename}, {"stats", h.metadata}});
                }
                do_write(json({{"status", "ok"}, {"history", arr}}).dump());
                
            } else if (cmd == "download") {
                std::string ts = j["timestamp"];
                std::string content = "";
                {
                    std::lock_guard<std::mutex> lock(g_stateMutex);
                    for (const auto& h : g_history) {
                        if (h.timestamp == ts) {
                            std::ifstream f(h.filename);
                            std::stringstream buffer;
                            buffer << f.rdbuf();
                            content = buffer.str();
                            break;
                        }
                    }
                }
                do_write(json({{"status", "ok"}, {"file_content", content}}).dump());
            }

        } catch (const std::exception& e) {
            std::cerr << "Processing Error: " << e.what() << "\n";
        }
        
        do_read(); // Continue loop
    }

    void do_write(std::string msg) {
        auto sp = std::make_shared<std::string>(msg);
        ws_.async_write(net::buffer(*sp), [sp, self = shared_from_this()](beast::error_code ec, std::size_t) {
            if (ec) return fail(ec, "write");
        });
    }
    
    // Call with mutex held
    void dispatch_job(const Job& job) {
        json req;
        req["command"] = "dispatch_job";
        req["params"] = job.params;
        do_write(req.dump());
    }

private:
    void handle_disconnect() {
        if (isProducer_) {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            if (g_activeProducer == shared_from_this()) {
                g_activeProducer = nullptr;
                std::cout << "[Server] Producer Disconnected.\n";
            }
        }
    }

    void dispatch_next_job_unsafe() {
        if (g_activeProducer && !g_producerBusy && !g_jobQueue.empty()) {
            Job job = g_jobQueue.front();
            g_jobQueue.pop();
            
            g_producerBusy = true;
            std::cout << "[Server] Dispatching Job " << job.id << " to Producer.\n";
            g_activeProducer->dispatch_job(job);
        }
    }

    void saveToHistory(const json& payload) {
        std::string ts = getCurrentTimestamp();
        std::string filename = "data/history/processed_" + ts + ".csv";
        
        std::filesystem::create_directories("data/history");
        std::ofstream out(filename);
        out << "id,name,age,grade\n";
        
        if (payload.contains("data") && payload["data"].is_array()) {
            for (const auto& item : payload["data"]) {
                out << item["id"] << "," << item["name"].get<std::string>() << "," 
                    << item["age"] << "," << item["grade"] << "\n";
            }
        }
        
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_history.push_back({ts, filename, payload["stats"].dump()});
        std::cout << "[Server] Saved history: " << filename << "\n";
    }

    static void fail(beast::error_code ec, char const* what) {
        std::cerr << what << ": " << ec.message() << "\n";
    }
};

// --- Listener ---
class Listener : public std::enable_shared_from_this<Listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint)
        : ioc_(ioc), acceptor_(ioc) {
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        acceptor_.bind(endpoint, ec);
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "Listen error: " << ec.message() << "\n";
            return;
        }
    }

    void run() { do_accept(); }

private:
    void do_accept() {
        acceptor_.async_accept(net::make_strand(ioc_), beast::bind_front_handler(&Listener::on_accept, shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<Session>(std::move(socket))->run();
        }
        do_accept();
    }
};

#include "../common/Config.h"

int main(int argc, char* argv[]) {
    // Load Config
    json conf = Config::load();
    int port = conf["server"]["port"];
    if (argc > 1) port = std::stoi(argv[1]); // Override

    net::io_context ioc{1};
    std::make_shared<Listener>(ioc, tcp::endpoint{tcp::v4(), (unsigned short)port})->run();
    
    std::cout << "[Server] Central Broker (with Job Queue) listening on port " << port << "...\n";
    std::cout << "[Server] Waiting for Producer registration...\n";
    
    ioc.run();
    return 0;
}
