#include <iostream>
#include <expected>
#include <string>

// Function that might fail
std::expected<int, std::string> validate_positive(int n) {
    if (n > 0) return n;
    return std::unexpected("Not a positive number.");
}

// Another failing step (requires and_then)
std::expected<int, std::string> divide_by_two(int n) {
    if (n == 0) return std::unexpected("Cannot divide zero");
    return n / 2;
}

// Error recovery (requires or_else)
std::expected<int, std::string> handle_error(const std::string& err) {
    std::cout << "Log: " << err << std::endl;
    return std::unexpected{"Error after handling"};
}

int main() {
    auto result = validate_positive(-7)     // Fails: -7 is negative
                .and_then(divide_by_two)    // Skipped due to previous error
                .or_else(handle_error);     // Recovers the error

    if (result) {
        std::cout << "Final Result: " << *result << "\n"; // Output: 1
    }
}
