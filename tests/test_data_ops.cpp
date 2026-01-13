#include <gtest/gtest.h>
#include "DataOperations.h"
#include "Student.h"
#include <vector>

// Helper to create dummy data
std::vector<Student> createDummyStudents() {
    return {
        {1, "Alice", 20, 85},
        {2, "Bob", 22, 90},
        {3, "Charlie", 20, 70}, // Same age as Alice
        {4, "Dave", 19, 95}
    };
}

TEST(DataOperationsTest, SortByAgeAscending) {
    auto students = createDummyStudents();
    DataOperations::sortStudents(students, SortField::AGE, SortOrder::ASC);
    
    // Check order: Dave(19), Alice(20), Charlie(20), Bob(22)
    EXPECT_EQ(students[0].name, "Dave");
    EXPECT_EQ(students[1].age, 20);
    EXPECT_EQ(students[2].age, 20);
    EXPECT_EQ(students[3].name, "Bob");
}

TEST(DataOperationsTest, SortByGradeDescending) {
    auto students = createDummyStudents();
    DataOperations::sortStudents(students, SortField::GRADE, SortOrder::DESC);
    
    // Check order: Dave(95.0), Bob(90.0), Alice(85.5), Charlie(70.0)
    EXPECT_EQ(students[0].name, "Dave");
    EXPECT_EQ(students[1].name, "Bob");
    EXPECT_EQ(students[2].name, "Alice");
    EXPECT_EQ(students[3].name, "Charlie");
}
