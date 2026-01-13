#include "DataOperations.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

void DataOperations::listStudents(const std::vector<Student>& students) {
    if (students.empty()) {
        std::cout << "[Info] No student records available.\n\n";
        return;
    }

    // Table Header
    std::cout << std::left 
              << std::setw(10) << "ID" 
              << std::setw(25) << "Name" 
              << std::setw(10) << "Age" 
              << std::setw(10) << "Grade" 
              << "\n";
    std::cout << std::string(55, '-') << "\n";

    // Table Rows
    for (const auto& s : students) {
        std::cout << std::left 
                  << std::setw(10) << s.id 
                  << std::setw(25) << s.name 
                  << std::setw(10) << s.age 
                  << std::setw(10) << s.grade 
                  << "\n";
    }
    std::cout << "\n";
}

std::vector<Student> DataOperations::searchById(const std::vector<Student>& students, int id) {
    std::vector<Student> results;
    for (const auto& s : students) {
        if (s.id == id) {
            results.push_back(s);
        }
    }
    return results;
}

std::vector<Student> DataOperations::searchByName(const std::vector<Student>& students, 
                                                  const std::string& name) {
    std::vector<Student> results;
    for (const auto& s : students) {
        if (s.name == name) {
            results.push_back(s);
        }
    }
    return results;
}

std::vector<Student> DataOperations::searchByAge(const std::vector<Student>& students, int age) {
    std::vector<Student> results;
    for (const auto& s : students) {
        if (s.age == age) {
            results.push_back(s);
        }
    }
    return results;
}

std::vector<Student> DataOperations::searchByGrade(const std::vector<Student>& students, int grade) {
    std::vector<Student> results;
    for (const auto& s : students) {
        if (s.grade == grade) {
            results.push_back(s);
        }
    }
    return results;
}

void DataOperations::sortStudents(std::vector<Student>& students, 
                                 SortField field, 
                                 SortOrder order) {
    std::sort(students.begin(), students.end(), [field, order](const Student& a, const Student& b) {
        // Generic comparison helper
        auto compareVal = [order](const auto& v1, const auto& v2) {
            if (order == SortOrder::ASC) {
                return v1 < v2;
            } else {
                return v2 < v1;
            }
        };

        switch (field) {
            case SortField::ID:    return compareVal(a.id, b.id);
            case SortField::NAME:  return compareVal(a.name, b.name);
            case SortField::AGE:   return compareVal(a.age, b.age);
            case SortField::GRADE: return compareVal(a.grade, b.grade);
        }
        return false;
    });
}

std::string DataOperations::sortFieldToString(SortField field) {
    switch (field) {
        case SortField::ID:    return "ID";
        case SortField::NAME:  return "Name";
        case SortField::AGE:   return "Age";
        case SortField::GRADE: return "Grade";
    }
    return "Unknown";
}
