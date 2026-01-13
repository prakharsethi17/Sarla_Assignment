#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <psapi.h>

// Include Data Logic
#include "Student.h"
#include "CSVHandler.h"
#include "DataOperations.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

// --- Stats ---
struct ProcessStats {
    long long parseTimeMs;
    long long processTimeMs;
    size_t memoryUsageBytes;
};

std::vector<Student> g_cache;

size_t getMemoryUsage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0;
}

// --- Worker Client ---
class WorkerClient : public std::enable_shared_from_this<WorkerClient> {
    websocket::stream<beast::tcp_stream> ws_;
    tcp::resolver resolver_; // Keep alive
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;

public:
    WorkerClient(net::io_context& ioc, const std::string& host, const std::string& port)
        : ws_(net::make_strand(ioc)), resolver_(net::make_strand(ioc)), host_(host), port_(port) {}

    void run() {
        resolver_.async_resolve(host_, port_, beast::bind_front_handler(&WorkerClient::on_resolve, shared_from_this()));
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if(ec) return fail(ec, "resolve");
        beast::get_lowest_layer(ws_).async_connect(results, beast::bind_front_handler(&WorkerClient::on_connect, shared_from_this()));
    }

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
        if(ec) return fail(ec, "connect");
        ws_.async_handshake(host_, "/", beast::bind_front_handler(&WorkerClient::on_handshake, shared_from_this()));
    }

    void on_handshake(beast::error_code ec) {
        if(ec) return fail(ec, "handshake");
        
        std::cout << "[Producer] Connected to Server.\n";
        
        // Register
        json reg;
        reg["command"] = "register_producer";
        do_write(reg.dump());
    }

    void do_read() {
        ws_.async_read(buffer_, beast::bind_front_handler(&WorkerClient::on_read, shared_from_this()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        if(ec) return fail(ec, "read");

        std::string msg = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());
        
        handle_message(msg);
        do_read(); // Loop
    }
    
    void handle_message(const std::string& msg) {
        try {
            json j = json::parse(msg);
            if (j.contains("command") && j["command"] == "dispatch_job") {
                std::cout << "[Producer] Received Job: " << j["params"].dump() << "\n";
                process_job(j["params"]);
            } else if (j.contains("status") && j["status"] == "registered") {
                std::cout << "[Producer] Registration Confirmed. Waiting for jobs...\n";
            }
        } catch (...) {
            std::cerr << "Message Error.\n";
        }
    }
    
    void process_job(json params) {
        ProcessStats stats = {0, 0, 0};
        
        // 1. Parse/Cache
        auto t1 = std::chrono::high_resolution_clock::now();
        if (g_cache.empty()) {
             g_cache = CSVHandler::readCSV("data/students.csv");
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        stats.parseTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        
        // 2. Process
        std::vector<Student> workingData = g_cache; // Copy
        std::vector<Student> resultData;
        auto t3 = std::chrono::high_resolution_clock::now();
        
        std::string operation = params.value("operation", "list");
        
        if (operation == "list") {
            resultData = workingData;
        } 
        else if (operation == "search") {
            std::string field = params.value("field", "id");
            if (field == "id") {
                int val = std::stoi(params.value("value", "0"));
                resultData = DataOperations::searchById(workingData, val);
            } else if (field == "name") {
                std::string val = params.value("value", "");
                resultData = DataOperations::searchByName(workingData, val);
            } else if (field == "age") {
                int val = std::stoi(params.value("value", "0"));
                resultData = DataOperations::searchByAge(workingData, val);
            } else if (field == "grade") {
                int val = std::stoi(params.value("value", "0"));
                resultData = DataOperations::searchByGrade(workingData, val);
            }
        } 
        else if (operation == "sort") {
            std::string field = params.value("field", "id");
            std::string orderStr = params.value("order", "asc");
            SortOrder order = (orderStr == "desc") ? SortOrder::DESC : SortOrder::ASC;
            
            SortField sField = SortField::ID;
            if (field == "name") sField = SortField::NAME;
            else if (field == "age") sField = SortField::AGE;
            else if (field == "grade") sField = SortField::GRADE;
            
            DataOperations::sortStudents(workingData, sField, order);
            resultData = workingData;
        }

        auto t4 = std::chrono::high_resolution_clock::now();
        stats.processTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
        stats.memoryUsageBytes = getMemoryUsage();
        
        // 3. Upload Result
        json uploadPayload;
        uploadPayload["stats"] = {
            {"parse_ms", stats.parseTimeMs}, 
            {"process_ms", stats.processTimeMs}, 
            {"memory_bytes", stats.memoryUsageBytes},
            {"record_count", resultData.size()}
        };
        
        json dataArr = json::array();
        for(const auto& s : resultData) {
            dataArr.push_back({{"id", s.id}, {"name", s.name}, {"age", s.age}, {"grade", s.grade}});
        }
        uploadPayload["data"] = dataArr;
        
        json response;
        response["command"] = "upload";
        response["payload"] = uploadPayload;
        
        std::cout << "[Producer] Job processed (" << resultData.size() << " records). Uploading...\n";
        do_write(response.dump());
    }

    void do_write(std::string msg) {
        auto sp = std::make_shared<std::string>(msg);
        ws_.async_write(net::buffer(*sp), [sp, self = shared_from_this()](beast::error_code ec, std::size_t) {
            if(!ec) self->do_read(); 
        });
    }

    void fail(beast::error_code ec, char const* what) {
        std::cerr << what << ": " << ec.message() << "\n";
    }
};

#include "../common/Config.h"

int main(int argc, char* argv[]) {
    // Load Config
    json conf = Config::load();
    std::string host = conf["producer"]["host"];
    int port = conf["producer"]["port"];

    // Preload
    try {
        g_cache = CSVHandler::readCSV("data/students.csv");
        std::cout << "[Producer] Cache Ready.\n";
    } catch(...) {}

    net::io_context ioc;
    std::make_shared<WorkerClient>(ioc, host, std::to_string(port))->run();
    ioc.run();
    return 0;
}
