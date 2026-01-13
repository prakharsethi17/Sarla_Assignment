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
#include "DataGenerator.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

// --- Stats ---
struct ProcessStats {
    long long parseTimeUs;
    long long processTimeUs;
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
    tcp::resolver resolver_; 
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
        do_read(); 
    }
    
    void handle_message(const std::string& msg) {
        try {
            json j = json::parse(msg);
            std::string cmd = j.value("command", "");
            
            if (cmd == "dispatch_job") {
                std::cout << "[Producer] Received Job: " << j["params"].dump() << "\n";
                process_job(j["params"]);
            } 
            else if (cmd == "generate_data") {
                int count = j.value("count", 1000);
                std::cout << "[Producer] Generating " << count << " new records...\n";
                DataGenerator::generate(count);
                
                // Reload Cache
                g_cache = CSVHandler::readCSV("data/students.csv");
                std::cout << "[Producer] Cache Reloaded with " << g_cache.size() << " records.\n";
                
                // Ack? Not strictly required by protocol but good for logs. 
                // The server treats Producer as 'Busy' only during dispatch_job? 
                // Actually, Server logic tracks busy state. If this was a job, we should respond.
                // But generate_data is a custom command. 
                // Currently Server dispatches "dispatch_job". 
                // If Server relays "generate_data" as a job, we should treat it as one.
                // Assuming 'dispatch_job' -> params: { operation: "generate", count: N } if we used that path.
                // BUT, let's treat it as a direct command for now, OR better:
                // Let's assume the Server dispatches a job `{ "operation": "generate", "count": N }`.
            }
            else if (j.contains("status") && j["status"] == "registered") {
                std::cout << "[Producer] Registration Confirmed.\n";
            }
        } catch (...) {
            std::cerr << "Message Error.\n";
        }
    }
    
    void process_job(json params) {
        ProcessStats stats = {0, 0, 0};
        
        std::string operation = params.value("operation", "list");
        
        // --- Special Case: Generation ---
        if (operation == "generate") {
            int count = params.value("count", 1000);
            DataGenerator::generate(count);
            // Reload
            auto t1 = std::chrono::high_resolution_clock::now();
            g_cache = CSVHandler::readCSV("data/students.csv");
            auto t2 = std::chrono::high_resolution_clock::now();
            stats.parseTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
            
            // Result is full list
            upload_result(g_cache, stats);
            return;
        }

        // 1. Check Cache
        auto t1 = std::chrono::high_resolution_clock::now();
        if (g_cache.empty()) {
             g_cache = CSVHandler::readCSV("data/students.csv");
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        stats.parseTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
        
        // 2. Process
        std::vector<Student> workingData = g_cache; 
        std::vector<Student> resultData;
        auto t3 = std::chrono::high_resolution_clock::now();
        
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
        stats.processTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
        stats.memoryUsageBytes = getMemoryUsage();
        
        upload_result(resultData, stats);
    }
    
    void upload_result(const std::vector<Student>& data, ProcessStats stats) {
        json uploadPayload;
        uploadPayload["stats"] = {
            {"parse_us", stats.parseTimeUs}, 
            {"process_us", stats.processTimeUs}, 
            {"memory_bytes", stats.memoryUsageBytes},
            {"record_count", data.size()}
        };
        
        json dataArr = json::array();
        for(const auto& s : data) {
            dataArr.push_back({{"id", s.id}, {"name", s.name}, {"age", s.age}, {"grade", s.grade}});
        }
        uploadPayload["data"] = dataArr;
        
        json response;
        response["command"] = "upload";
        response["payload"] = uploadPayload;
        
        std::cout << "[Producer] Job processed. Sending " << data.size() << " records.\n";
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
    json conf = Config::load();
    std::string host = conf["producer"]["host"];
    int port = conf["producer"]["port"];

    // Preload
    try {
        g_cache = CSVHandler::readCSV("data/students.csv");
        std::cout << "[Producer] Cache Ready. Loaded " << g_cache.size() << " records.\n";
    } catch(...) {}

    net::io_context ioc;
    std::make_shared<WorkerClient>(ioc, host, std::to_string(port))->run();
    ioc.run();
    return 0;
}
