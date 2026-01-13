#ifndef CSVHANDLER_H
#define CSVHANDLER_H

#include "Student.h"
#include <vector>
#include <string>
#include <stdexcept>

/**
 * CSVHandler - Static utility class for CSV file I/O operations
 * Handles reading and writing student records in CSV format
 */
class CSVHandler {
public:
    /**
     * Read student records from a CSV file
     * Format: id,name,age,grade
     * @param filepath Path to the CSV file
     * @param hasHeader Whether the file has a header row (default: true)
     * @return Vector of Student objects
     * @throws std::runtime_error if file cannot be opened or data is malformed
     */
    static std::vector<Student> readCSV(const std::string& filepath, bool hasHeader = true);

    /**
     * Write student records to a CSV file
     * @param filepath Path to the output CSV file
     * @param students Vector of Student objects to write
     * @param writeHeader Whether to write a header row (default: true)
     * @throws std::runtime_error if file cannot be created or written
     */
    static void writeCSV(const std::string& filepath, 
                        const std::vector<Student>& students, 
                        bool writeHeader = true);

private:
    // Helper function to trim whitespace from strings
    static std::string trim(const std::string& str);
    
    // Helper function to split CSV line
    static std::vector<std::string> splitCSV(const std::string& line);
};

#endif // CSVHANDLER_H
