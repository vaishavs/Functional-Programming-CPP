#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5};

    // Bug 1 & 2: Goal is to double only odd numbers and store them back
    // Hint: Check the return value and the predicate logic
    std::remove_if(nums.begin(), nums.end(), [](int n) {
        return n % 2 == 0; 
    });

    // Bug 3: Goal is to double the remaining numbers (1, 3, 5 -> 2, 6, 10)
    // Hint: std::transform requires a specific output destination
    std::transform(nums.begin(), nums.end(), [](int n) {
        return n * 2;
    });

    // Bug 4: Calculate the final sum
    // Hint: std::accumulate is sensitive to the type of its third argument
    double total = std::accumulate(nums.begin(), nums.end(), 0.0);

    std::cout << "Final Total: " << total << std::endl;
    return 0;
}
