#pragma once
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <chrono>

class DataGenerator {
public:
    static void generate(int count, const std::string& filename = "data/students.csv") {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return;
        }

        // Write header
        file << "id,name,age,grade\n";

        // Use a random seed based on time for variety, or fixed if needed. 
        // User requested dynamic generation, so maybe time-based is better now? 
        // Or keep 42 for reproducibility? Let's use 42 for consistency unless asked.
        std::mt19937 rng(42); 
        std::uniform_int_distribution<int> age_dist(10, 18);
        std::uniform_int_distribution<int> grade_dist(0, 100);

        for (int i = 1; i <= count; ++i) {
            file << i << ","
                 << "Student_" << i << ","
                 << age_dist(rng) << ","
                 << grade_dist(rng) << "\n";
        }
        
        std::cout << "[Generator] Created " << count << " records in " << filename << "\n";
    }
};
