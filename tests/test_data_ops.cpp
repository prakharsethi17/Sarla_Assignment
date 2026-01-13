#include <gtest/gtest.h>
#include "DataOperations.h"
#include "Student.h"
#include <vector>

// Helper to create dummy data
std::vector<Student> createDummyStudents() {
    return {
        {1, "Alice", 20, 85},
        {2, "Bob", 22, 90},
        {3, "Charlie", 20, 70}, 
        {4, "Dave", 19, 95},
        {5, "Eve", 20, 85} // Duplicate Age/Grade for testing
    };
}

// --- SORT TESTS ---
TEST(DataOperationsTest, SortByAgeAscending) {
    auto students = createDummyStudents();
    DataOperations::sortStudents(students, SortField::AGE, SortOrder::ASC);
    
    // Order: Dave(19), Alice(20), Charlie(20), Eve(20), Bob(22)
    EXPECT_EQ(students[0].name, "Dave");
    EXPECT_EQ(students[1].age, 20);
    EXPECT_EQ(students.back().name, "Bob");
}

TEST(DataOperationsTest, SortByGradeDescending) {
    auto students = createDummyStudents();
    DataOperations::sortStudents(students, SortField::GRADE, SortOrder::DESC);
    
    // Order: Dave(95), Bob(90), Alice(85), Eve(85), Charlie(70)
    EXPECT_EQ(students[0].name, "Dave");
    EXPECT_EQ(students[1].name, "Bob");
    EXPECT_EQ(students.back().name, "Charlie");
}

// --- SEARCH TESTS ---
TEST(DataOperationsTest, SearchById) {
    auto students = createDummyStudents();
    auto results = DataOperations::searchById(students, 3);
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].name, "Charlie");
}

TEST(DataOperationsTest, SearchByName) {
    auto students = createDummyStudents();
    auto results = DataOperations::searchByName(students, "Bob");
    
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].id, 2);
}

TEST(DataOperationsTest, SearchByAge) {
    auto students = createDummyStudents();
    // Alice(20), Charlie(20), Eve(20)
    auto results = DataOperations::searchByAge(students, 20);
    
    ASSERT_EQ(results.size(), 3);
}

TEST(DataOperationsTest, SearchByGrade) {
    auto students = createDummyStudents();
    // Alice(85), Eve(85)
    auto results = DataOperations::searchByGrade(students, 85);
    
    ASSERT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].grade, 85);
    EXPECT_EQ(results[1].grade, 85);
}

// --- LIST TEST ---
// listStudents prints to stdout. We just verify it doesn't crash or modify const data.
TEST(DataOperationsTest, ListStudentsSmokeTest) {
    auto students = createDummyStudents();
    testing::internal::CaptureStdout(); // Capture output to keep console clean
    DataOperations::listStudents(students);
    std::string output = testing::internal::GetCapturedStdout();
    
    // Simple check that it printed something
    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("ID"), std::string::npos);
    EXPECT_NE(output.find("Alice"), std::string::npos);
}
