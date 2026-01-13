cmake_minimum_required(VERSION 3.15)
project(StudentSystem VERSION 2.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(_WIN32_WINNT=0x0601)

# Find Packages (provided by vcpkg)
find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json REQUIRED)

# Include Directories
include_directories(include)

# --- 1. Generator ---
add_executable(generate_students src/generator/generate_students.cpp)
# Header-only lib, vcpkg adds include path automatically

# --- 2. Central Server ---
add_executable(server src/server/central_server.cpp src/common/DataOperations.cpp src/common/Student.cpp src/common/CSVHandler.cpp)
target_link_libraries(server PRIVATE 
    Boost::system 
    nlohmann_json::nlohmann_json 
    ws2_32
)

# --- 3. Producer Client ---
add_executable(producer src/client/producer_node.cpp src/common/DataOperations.cpp src/common/Student.cpp src/common/CSVHandler.cpp)
target_link_libraries(producer PRIVATE 
    Boost::system 
    nlohmann_json::nlohmann_json 
    ws2_32
    psapi
)

# --- 4. Consumer Client ---
add_executable(consumer src/client/consumer_node.cpp src/common/DataOperations.cpp src/common/Student.cpp src/common/CSVHandler.cpp)
target_link_libraries(consumer PRIVATE 
    Boost::system 
    nlohmann_json::nlohmann_json 
    ws2_32
)

# --- 5. Unit Tests ---
enable_testing()
find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests 
    tests/test_main.cpp 
    tests/test_data_ops.cpp
    src/common/DataOperations.cpp 
    src/common/Student.cpp
)
target_link_libraries(unit_tests PRIVATE GTest::gtest GTest::gtest_main)
target_include_directories(unit_tests PRIVATE include)

add_test(NAME CoreLogicTests COMMAND unit_tests)
