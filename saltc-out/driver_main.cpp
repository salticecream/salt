#include <iostream>
#include <cstdlib>
int gangplanks = 0;
extern "C" {
    int print_gangplank(int times);
}

extern "C" int gangplank() {
    return printf("gangplank %d\n", ++gangplanks);
}

int main(int argc, char** argv) {
    
    if (argc != 2)
        return std::printf("fuck you");

    int x = std::atoi(argv[1]);
    print_gangplank(x);

}