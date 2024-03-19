#include <iostream>
#include <cstdlib>

extern "C" {
    int cool_exp(int base, int exponent);
}

int main(int argc, char** argv) {
    
    if (argc != 3)
        return std::printf("fuck you");

    int x = std::atoi(argv[1]);
    int y = std::atoi(argv[2]);

    std::cout << cool_exp(x, y);
    
}