#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <iomanip>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;

// Generic Sync Client interaction
json sendCommand(const std::string& host, int port, const json& cmd) {
    net::io_context ioc;
    tcp::resolver resolver(ioc);
    websocket::stream<beast::tcp_stream> ws(ioc);
    
    auto const results = resolver.resolve(host, std::to_string(port));
    beast::get_lowest_layer(ws).connect(results);
    ws.handshake(host, "/");
    
    ws.write(net::buffer(cmd.dump()));
    
    beast::flat_buffer buffer;
    ws.read(buffer);
    ws.close(websocket::close_code::normal);
    
    std::string s = beast::buffers_to_string(buffer.data());
    try {
        return json::parse(s);
    } catch(...) {
        return {{"raw", s}};
    }
}

void displayTable(const std::string& filename, const json& stats, const std::string& content) {
    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << "                 DOWNLOAD SUMMARY                           \n";
    std::cout << "============================================================\n";
    std::cout << std::left << std::setw(20) << "File Name" << ": " << filename << "\n";
    std::cout << std::left << std::setw(20) << "Records" << ": " << stats.value("record_count", 0) << "\n";
    
    // Check for us vs ms
    if (stats.contains("parse_us")) {
        std::cout << std::left << std::setw(20) << "Parse Time" << ": " << stats.value("parse_us", 0) << " us\n";
        std::cout << std::left << std::setw(20) << "Process Time" << ": " << stats.value("process_us", 0) << " us\n";
    } else {
        std::cout << std::left << std::setw(20) << "Parse Time" << ": " << stats.value("parse_ms", 0) << " ms\n";
        std::cout << std::left << std::setw(20) << "Process Time" << ": " << stats.value("process_ms", 0) << " ms\n";
    }
    
    std::cout << std::left << std::setw(20) << "Memory Usage" << ": " << stats.value("memory_bytes", 0) << " bytes\n";
    std::cout << "============================================================\n";
    if (content.length() > 200) {
        std::cout << "Preview (First 200 chars):\n" << content.substr(0, 200) << "...\n";
    } else {
        std::cout << "Content:\n" << content << "\n";
    }
    std::cout << "\n";
}

#include "../common/Config.h"

int main() {
    json conf = Config::load();
    std::string serverHost = conf["consumer"]["host"];
    int serverPort = conf["consumer"]["port"];
    
    while(true) {
        std::cout << "\n=== CONSUMER CONTROLLER ===\n";
        std::cout << "1. Submit Job (Process Data)\n";
        std::cout << "2. Generate New Dataset\n"; // New option
        std::cout << "3. Review History (Download & View)\n";
        std::cout << "4. Exit\n";
        std::cout << "> ";
        
        int mainChoice;
        std::cin >> mainChoice;
        
        if (mainChoice == 1) {
            std::cout << "\n--- Job Type ---\n";
            std::cout << "1. List All\n";
            std::cout << "2. Search\n";
            std::cout << "3. Sort\n";
            std::cout << "> ";
            int jobType;
            std::cin >> jobType;
            
            json req;
            req["command"] = "submit_job";
            json params;
            
            if (jobType == 1) {
                params["operation"] = "list";
            } 
            else if (jobType == 2) {
                params["operation"] = "search";
                std::cout << "Field (id, name, age, grade): ";
                std::string field; std::cin >> field;
                std::cout << "Value: ";
                std::string val; std::cin >> val;
                params["field"] = field;
                params["value"] = val;
            } 
            else if (jobType == 3) {
                params["operation"] = "sort";
                std::cout << "Field (id, name, age, grade): ";
                std::string field; std::cin >> field;
                std::cout << "Order (asc, desc): ";
                std::string order; std::cin >> order;
                params["field"] = field;
                params["order"] = order;
            } else {
                continue;
            }
            
            req["params"] = params;
            std::cout << "Submitting Job...\n";
            try {
                auto resp = sendCommand(serverHost, serverPort, req);
                std::cout << "Server: " << resp.value("status", "unknown") << " (Job ID: " << resp.value("job_id", "?") << ")\n";
            } catch(std::exception const& e) { std::cerr << e.what() << "\n"; }

        } 
        else if (mainChoice == 2) {
            // GENERATE DATASET
            std::cout << "Enter number of records to generate (e.g. 1000, 10000): ";
            int count;
            std::cin >> count;
            
            json req;
            req["command"] = "submit_job";
            req["params"] = {
                {"operation", "generate"},
                {"count", count}
            };
             std::cout << "Requesting Generation...\n";
            try {
                auto resp = sendCommand(serverHost, serverPort, req);
                std::cout << "Server: " << resp.value("status", "unknown") << " (Job ID: " << resp.value("job_id", "?") << ")\n";
                // Note: This relies on the Producer picking up the job and rewriting the file.
            } catch(std::exception const& e) { std::cerr << e.what() << "\n"; }
        }
        else if (mainChoice == 3) {
            json req;
            req["command"] = "list_history";
            try {
                auto resp = sendCommand(serverHost, serverPort, req);
                if (resp.contains("history")) {
                    std::vector<json> hist = resp["history"];
                    
                    // Sort Recent First
                    std::sort(hist.begin(), hist.end(), [](const json& a, const json& b){
                        return a["timestamp"] > b["timestamp"];
                    });
                    
                    std::cout << "\n--- RECENT HISTORY ---\n";
                    int idx = 0;
                    int maxItems = 10;
                    for(const auto& item : hist) {
                        idx++;
                        if (idx > maxItems) break;
                        std::cout << idx << ". " << item["timestamp"] << " | " << item["filename"] << "\n";
                    }
                    
                    if (hist.empty()) {
                        std::cout << "(No history found)\n";
                        continue;
                    }
                    
                    std::cout << "\nEnter Number to Download (1-" << std::min((int)hist.size(), maxItems) << ") or 0 to back: ";
                    int sel;
                    std::cin >> sel;
                    
                    if (sel > 0 && sel <= idx) {
                        json targetItem = hist[sel-1];
                        std::string ts = targetItem["timestamp"];
                        json targetStats;
                        if(targetItem["stats"].is_string()) targetStats = json::parse(targetItem["stats"].get<std::string>()); 
                        else targetStats = targetItem["stats"];

                        json dlReq;
                        dlReq["command"] = "download";
                        dlReq["timestamp"] = ts;
                        auto dlResp = sendCommand(serverHost, serverPort, dlReq);
                        
                        if (dlResp.contains("file_content") && !dlResp["file_content"].get<std::string>().empty()) {
                            std::string content = dlResp["file_content"];
                            std::ofstream out("data/final_output.csv");
                            out << content;
                            displayTable(targetItem["filename"], targetStats, content);
                        } else {
                            std::cout << "Error Downloading.\n";
                        }
                    }
                }
            } catch(std::exception const& e) { std::cerr << e.what() << "\n"; }
            
        } else {
            break;
        }
    }
    return 0;
}
