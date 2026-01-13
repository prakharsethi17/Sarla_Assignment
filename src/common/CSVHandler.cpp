#include "CSVHandler.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>



// Include the fast-csv-parser
#include <fast-cpp-csv-parser/csv.h>

std::vector<Student> CSVHandler::readCSV(const std::string& filepath, bool hasHeader) {
    std::vector<Student> students;
    
    try {
        io::CSVReader<4> in(filepath);
        if (hasHeader) {
            in.read_header(io::ignore_extra_column, "id", "name", "age", "grade");
        }
        
        int id;
        std::string name;
        int age;
        int grade;
        
        while (in.read_row(id, name, age, grade)) {
             Student s;
             s.id = id;
             s.name = name;
             s.age = age;
             s.grade = grade;
             students.push_back(s);
        }
    } catch (const io::error::can_not_open_file& e) {
        throw std::runtime_error(std::string("Cannot open file: ") + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("CSV Parse Error: ") + e.what());
    }
    
    std::cout << "Loaded " << students.size() << " valid records using fast-cpp-csv-parser.\n";
    return students;
}

void CSVHandler::writeCSV(const std::string& filepath, 
                         const std::vector<Student>& students, 
                         bool writeHeader) {
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filepath);
    }
    
    // Write header
    if (writeHeader) {
        file << "id,name,age,grade\n";
    }
    
    // Write student records
    for (const auto& s : students) {
        file << s.id << ","
             << s.name << ","
             << s.age << ","
             << s.grade << "\n";
    }
    
    file.close();
    
    if (!file.good()) {
        throw std::runtime_error("Error writing to file: " + filepath);
    }
}
