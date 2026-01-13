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
    std::cout << std::left << std::setw(20) << "Parse Time" << ": " << stats.value("parse_ms", 0) << " ms\n";
    std::cout << std::left << std::setw(20) << "Process Time" << ": " << stats.value("process_ms", 0) << " ms\n";
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
        std::cout << "1. Submit Job\n";
        std::cout << "2. Review History (Download & View)\n";
        std::cout << "3. Exit\n";
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

        } else if (mainChoice == 2) {
            json req;
            req["command"] = "list_history";
            try {
                auto resp = sendCommand(serverHost, serverPort, req);
                if (resp.contains("history")) {
                    std::vector<json> hist = resp["history"];
                    
                    // Sort Descending by latest (Timestamp string comparison works for YYYYMMDD_HHMMSS)
                    std::sort(hist.begin(), hist.end(), [](const json& a, const json& b){
                        return a["timestamp"] > b["timestamp"];
                    });
                    
                    std::cout << "\n--- HISTORY (Latest First) ---\n";
                    for(const auto& item : hist) {
                        std::cout << " [" << item["timestamp"] << "] " << item["filename"] << "\n";
                    }
                    
                    std::cout << "\nEnter Timestamp to Download (or '0' to back): ";
                    std::string ts;
                    std::cin >> ts;
                    
                    if (ts != "0") {
                        // Find stats for this TS to display table
                        json targetStats;
                        std::string targetFilename;
                        for(const auto& item : hist) {
                            if (item["timestamp"] == ts) {
                                targetFilename = item["filename"];
                                if(item["stats"].is_string()) targetStats = json::parse(item["stats"].get<std::string>()); 
                                else targetStats = item["stats"];
                                break;
                            }
                        }

                        json dlReq;
                        dlReq["command"] = "download";
                        dlReq["timestamp"] = ts;
                        auto dlResp = sendCommand(serverHost, serverPort, dlReq);
                        
                        if (dlResp.contains("file_content") && !dlResp["file_content"].get<std::string>().empty()) {
                            std::string content = dlResp["file_content"];
                            std::ofstream out("data/final_output.csv");
                            out << content;
                            displayTable(targetFilename, targetStats, content);
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
