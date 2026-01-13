#ifndef DATAOPERATIONS_H
#define DATAOPERATIONS_H

#include "Student.h"
#include <vector>
#include <string>

/**
 * Enums for sorting operations
 */
enum class SortField {
    ID,
    NAME,
    AGE,
    GRADE
};

enum class SortOrder {
    ASC,
    DESC
};

/**
 * DataOperations - Provides core data manipulation operations
 * Implements list, search, and sort functionality for student records
 */
class DataOperations {
public:
    /**
     * Display all student records in a formatted table
     */
    static void listStudents(const std::vector<Student>& students);

    /**
     * Search for students by ID
     * @param students Vector of students to search
     * @param id Student ID to search for
     * @return Vector of matching students (empty if not found)
     */
    static std::vector<Student> searchById(const std::vector<Student>& students, int id);

    /**
     * Search for students by name (exact match)
     * @param students Vector of students to search
     * @param name Student name to search for
     * @return Vector of matching students (empty if not found)
     */
    static std::vector<Student> searchByName(const std::vector<Student>& students, 
                                            const std::string& name);

    static std::vector<Student> searchByAge(const std::vector<Student>& students, int age);
    
    static std::vector<Student> searchByGrade(const std::vector<Student>& students, int grade);

    /**
     * Sort students in-place by specified field and order
     * @param students Vector of students to sort (modified in-place)
     * @param field Field to sort by
     * @param order Sort order (ASC or DESC)
     */
    static void sortStudents(std::vector<Student>& students, 
                           SortField field, 
                           SortOrder order);

    /**
     * Get string representation of sort field
     */
    static std::string sortFieldToString(SortField field);
};

#endif // DATAOPERATIONS_H
