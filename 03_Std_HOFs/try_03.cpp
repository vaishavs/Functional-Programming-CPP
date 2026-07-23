#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <stack>
#include <functional>
#include <cassert>

// ============================================================================
// Exercise 1: Iterator Adaptors & Transformation
// Concept: Applying an operation to a range and safely inserting into a new one.
// Standard Tools: std::transform, std::back_inserter
// ============================================================================
std::vector<int> double_values(const std::vector<int>& input) {
    std::vector<int> result;
    
    // TODO: Multiply each element in 'input' by 2 and store it in 'result'.
    // HINT 1: Use std::transform(begin, end, destination, operation).
    // HINT 2: Because 'result' is empty, you cannot write directly to result.begin(). 
    //         Use std::back_inserter(result) as the destination.
    // HINT 3: Use a lambda [](int x) { return x * 2; } for the operation.
    
    return result;
}

// ============================================================================
// Exercise 2: The Erase-Remove Idiom
// Concept: Filtering elements out of a container in-place.
// Standard Tools: std::remove_if, std::vector::erase (or std::erase_if in C++20)
// ============================================================================
void remove_negatives(std::vector<int>& numbers) {
    // TODO: Remove all negative numbers from the 'numbers' vector in-place.
    // HINT 1: std::remove_if(begin, end, condition) moves items to be deleted 
    //         to the end and returns an iterator to the new logical end.
    // HINT 2: Call numbers.erase(new_end, numbers.end()) to actually resize it.
    // HINT 3: The lambda condition should be [](int x) { return x < 0; }
    // (Note: If using C++20, you can achieve this in one step with std::erase_if).

}

// ============================================================================
// Exercise 3: Numeric Algorithms
// Concept: Folding/reducing a range down to a single value.
// Standard Tools: std::accumulate (from <numeric>)
// ============================================================================
int calculate_product(const std::vector<int>& numbers) {
    // TODO: Calculate the product (multiplication) of all elements.
    // HINT 1: Use std::accumulate(begin, end, initial_value, operation).
    // HINT 2: Watch out for the initial value! What should you multiply by first 
    //         so you don't end up with 0?
    // HINT 3: For the operation, you can use a lambda [](int a, int b){ return a * b; } 
    //         or the built-in std::multiplies<int>().
    
    return 0; // Replace this with your std::accumulate result
}

// ============================================================================
// Exercise 4: Container Adaptors
// Concept: Using a LIFO (Last-In, First-Out) data structure to reverse data.
// Standard Tools: std::stack
// ============================================================================
std::string reverse_string(const std::string& text) {
    std::stack<char> char_stack;
    std::string reversed_text;

    // TODO: Push all characters from 'text' onto 'char_stack'.
    // HINT: Use a range-based for loop and char_stack.push().
    
    // TODO: Pop characters from 'char_stack' and append them to 'reversed_text'.
    // HINT 1: Loop while !char_stack.empty().
    // HINT 2: Grab the top element using char_stack.top().
    // HINT 3: Append it to 'reversed_text' (e.g., using +=).
    // HINT 4: Don't forget to call char_stack.pop() to remove the element!

    return reversed_text;
}


// ============================================================================
// Verification Tests - Do not modify below this line
// ============================================================================
int main() {
    std::cout << "Running standard library tests...\n\n";

    // Test 1
    std::vector<int> input1 = {1, 2, 3, 4};
    std::vector<int> expected1 = {2, 4, 6, 8};
    assert(double_values(input1) == expected1);
    std::cout << "[✓] Exercise 1 (std::transform & back_inserter) Passed!\n";

    // Test 2
    std::vector<int> input2 = {5, -2, 9, -8, -7, 1};
    std::vector<int> expected2 = {5, 9, 1};
    remove_negatives(input2);
    assert(input2 == expected2);
    std::cout << "[✓] Exercise 2 (Erase-Remove Idiom) Passed!\n";

    // Test 3
    std::vector<int> input3 = {2, 3, 4}; 
    assert(calculate_product(input3) == 24);
    std::cout << "[✓] Exercise 3 (std::accumulate) Passed!\n";

    // Test 4
    std::string input4 = "srotpada & smhtirogla";
    assert(reverse_string(input4) == "algorithms & adaptors");
    std::cout << "[✓] Exercise 4 (std::stack) Passed!\n";

    std::cout << "\n🎉 All exercises completed successfully!\n";
    return 0;
}
