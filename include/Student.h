#ifndef STUDENT_H
#define STUDENT_H

#include <string>
#include <iostream>
#include <iomanip>

/**
 * Student class - Represents a student record
 * Uses POD (Plain Old Data) structure for simplicity
 * Includes methods for JSON serialization/deserialization
 */
struct Student {
    int id;
    std::string name;
    int age;
    int grade;

    // Default constructor
    Student() : id(0), name(""), age(0), grade(0) {}

    // Parameterized constructor
    Student(int id, const std::string& name, int age, int grade)
        : id(id), name(name), age(age), grade(grade) {}

    // Convert to JSON string
    std::string toJSON() const;

    // Parse from JSON string
    static Student fromJSON(const std::string& json);

    // Comparison operators for sorting
    bool operator<(const Student& other) const {
        return id < other.id;
    }

    bool operator==(const Student& other) const {
        return id == other.id && name == other.name && 
               age == other.age && grade == other.grade;
    }

    bool operator!=(const Student& other) const {
        return !(*this == other);
    }

    // Stream output for debugging
    friend std::ostream& operator<<(std::ostream& os, const Student& s) {
        os << std::setw(10) << s.id << " | "
           << std::setw(20) << s.name << " | "
           << std::setw(5) << s.age << " | "
           << std::setw(8) << s.grade;
        return os;
    }
};

#endif // STUDENT_H
