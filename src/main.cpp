#include <iostream>
#include "headers/Vector2.h"

int main() {

    std::cout << Vector2{3,4}.length() << "\n";            // 5
    std::cout << Vector2{3,4}.normalise().length() << "\n"; // 1
    Vector2 a{7,-2};
    std::cout << (a-a).x << " " << (a-a).y << "\n";        // 0 0
    
}