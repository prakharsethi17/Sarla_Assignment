#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include <iomanip>

struct Student {
    int id;
    std::string name;
    int age;
    int grade;
};

int main(int argc, char* argv[]) {
    int count = 1000;
    std::string filename = "data/students.csv";

    if (argc > 1) count = std::stoi(argv[1]);
    if (argc > 2) filename = argv[2];

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    // Write header
    file << "id,name,age,grade\n";

    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> age_dist(10, 18);
    std::uniform_int_distribution<int> grade_dist(0, 100);

    for (int i = 1; i <= count; ++i) {
        file << i << ","
             << "Student_" << i << ","
             << age_dist(rng) << ","
             << grade_dist(rng) << "\n";
    }

    std::cout << "Generated " << count << " records to " << filename << std::endl;
    return 0;
}
