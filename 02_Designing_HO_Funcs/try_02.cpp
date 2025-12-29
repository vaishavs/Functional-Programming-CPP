// Fix the bug in the faulty_incrementor function by ensuring the lambda captures the increment value correctly.
// This will allow the returned function to behave as expected. Make sure your code outputs 15, which is the correct answer.

#include <iostream>
#include <functional>

std::function<int(int)> faulty_incrementor() {
    int local_increment = 5;
    return [&local_increment](int x) {
        return x + local_increment;  
    };
}

int main() {
    auto inc = faulty_incrementor();
    std::cout << "Increment 5 by 10: " << inc(10) << '\n';  // Undefined behavior
    return 0;
}
