#include <iostream>
#include <vector>
#include <functional>

// Reducing function
int reduce(const std::vector<int>& vec, std::function<int(int, int)> reduceFunc) {
    if (vec.empty()) {
        throw std::invalid_argument("Vector must not be empty");
    }
    // TODO: initialize result with the reduceFunc(vec[0], vec[1])
    // TODO: iterate other elements and combine them with result using reduceFunc
    return result;
}

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    // Sum of elements
    auto sumFunc = [](int a, int b) { return a + b; };
    int sum = reduce(numbers, sumFunc);
    std::cout << "Sum: " << sum << std::endl; // Output: Sum: 15

    // Product of elements
    auto productFunc = [](int a, int b) { return a * b; };
    int product = reduce(numbers, productFunc);
    std::cout << "Product: " << product << std::endl; // Output: Product: 120

    return 0;
}
