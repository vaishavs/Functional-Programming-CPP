#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <list>

using namespace std;

// HINT: Does this reduce function handle empty containers correctly?
// Check the initial value and operation signature.
template<typename T, typename BinaryOp>
T reduce(const vector<T>& data, T init, BinaryOp op) {
    T result = init;
    // BUG: Wrong algorithm! for_each doesn't return accumulated value
    for_each(data.begin(), data.end(), [&](T x) { 
        result = op(result, x);  // This works but for_each is wrong choice
    });
    return result;
}

// HINT: Check std::function signature - does it match lambda return type?
void apply_to_each(list<int>& data, function<void(int&)> mutator) {
    // BUG: Missing call to higher-order function! Direct loop instead
    for (auto it = data.begin(); it != data.end(); ++it) {
        *it = mutator(*it);  // WRONG: mutator returns value but takes ref
    }
}

// HINT: Predicate lambda - check capture and return type consistency
auto is_even = [](int x) -> bool {
    return x % 2 == 0;
    // BUG: Unreachable code after return - compiler warning!
    return false;
};

int main() {
    vector<int> nums = {1, 2, 3, 4, 5, 6};
    list<int> lst = {10, 20, 30, 40};
    
    cout << "Original vector: ";
    for (int n : nums) cout << n << " ";
    cout << endl;
    
    // Task 1: Fix reduce to sum all elements (expected: 21)
    // HINT: accumulate is better, but fix current implementation too
    cout << "Sum: " << reduce(nums, 0, plus<int>()) << endl;
    
    // Task 2: Fix apply_to_each to double list elements using lambda
    cout << "Before doubling list: ";
    for (int n : lst) cout << n << " ";
    cout << endl;
    
    // BUG: Won't compile - wrong function signature
    apply_to_each(lst, [](int x) { return x * 2; });
    
    cout << "After doubling list: ";
    for (int n : lst) cout << n << " ";
    cout << endl;
    
    // Task 3: Count evens using count_if (expected: 3)
    // HINT: count_if returns iterator distance, not count directly
    auto even_count = count_if(nums.begin(), nums.end(), is_even);
    cout << "Even count: " << distance(nums.begin(), even_count) << endl;
    
    return 0;
}
