#include "Student.h"
#include <sstream>
#include <algorithm>

// Simple JSON serialization (manual, no library needed for basic format)
std::string Student::toJSON() const {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << id << ","
        << "\"name\":\"" << name << "\","
        << "\"age\":" << age << ","
        << "\"grade\":" << grade
        << "}";
    return oss.str();
}

// Simple JSON deserialization (manual parsing)
Student Student::fromJSON(const std::string& json) {
    Student s;
    
    // Find and extract each field
    auto findValue = [&json](const std::string& key) -> std::string {
        size_t pos = json.find("\"" + key + "\":");
        if (pos == std::string::npos) return "";
        
        pos += key.length() + 3; // Skip past "key":
        
        // Skip whitespace
        while (pos < json.length() && std::isspace(json[pos])) pos++;
        
        if (pos >= json.length()) return "";
        
        // If it's a string (starts with ")
        if (json[pos] == '"') {
            size_t start = ++pos;
            size_t end = json.find('"', start);
            if (end == std::string::npos) return "";
            return json.substr(start, end - start);
        }
        
        // Otherwise it's a number
        size_t start = pos;
        while (pos < json.length() && 
               (std::isdigit(json[pos]) || json[pos] == '.' || json[pos] == '-')) {
            pos++;
        }
        return json.substr(start, pos - start);
    };
    
    try {
        std::string idStr = findValue("id");
        std::string nameStr = findValue("name");
        std::string ageStr = findValue("age");
        std::string gradeStr = findValue("grade");
        
        if (!idStr.empty()) s.id = std::stoi(idStr);
        s.name = nameStr;
        if (!ageStr.empty()) s.age = std::stoi(ageStr);
        if (!gradeStr.empty()) s.grade = std::stoi(gradeStr);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
    
    return s;
}
