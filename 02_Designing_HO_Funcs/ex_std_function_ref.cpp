/* Since C++26 */
#include <iostream>
#include <functional> // std::function_ref (C++26)
#include <vector>

// Accepts any callable that takes an int and returns void
// It does NOT take ownership; it only refers to the callable during execution.
void process(const std::vector<int>& data, std::function_ref<void(int)> action) {
    for (int x : data) {
        action(x);
    }
}

// DANGEROUS: The lambda is destroyed by the time this function returns.
// Bad because std::function_ref does not make an internal copy of the function assigned to it like std::function does
// std::function_ref<int(int)> get_bad_ref() {
//     return [](int x) { return x * 2; }; // BUG: returns reference to temporary
// }


int main() {
    std::vector<int> nums = {1, 2, 3};
    int total = 0;

    // A stateful lambda capturing 'total' by reference
    auto addToTotal = [&](int n) { total += n; };

    // Pass the lambda directly; function_ref refers to 'addToTotal'
    process(nums, addToTotal);

    std::cout << "Total: " << total << std::endl; // Output: Total: 6
    return 0;
}
