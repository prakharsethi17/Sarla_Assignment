#include "../../include/DataGenerator.h"
#include <string>

int main(int argc, char* argv[]) {
    int count = 1000;
    std::string filename = "data/students.csv";

    if (argc > 1) count = std::stoi(argv[1]);
    if (argc > 2) filename = argv[2];

    DataGenerator::generate(count, filename);
    return 0;
}
