/* Jump table, aka array of function pointers */
#include <iostream>

void op1() { std::cout << "Op 1" << std::endl; }
void op2() { std::cout << "Op 2" << std::endl; }

// Array of function pointers
void (*ops[2])() = { op1, op2 };

int main() {
    ops[0](); // Calls op1
    ops[1](); // Calls op2
    return 0;
}
