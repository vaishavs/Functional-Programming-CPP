#include <iostream>
#include <functional>
#include <string>

// =============================================================================
// HELPER FUNCTIONS (Do not modify these)
// =============================================================================

// Calculates the final price of an item including tax.
void print_receipt(const std::string& store_name, double tax_rate, double base_amount) {
    double final_price = base_amount * (1.0 + tax_rate);
    std::cout << "[" << store_name << "] Total: $" << final_price << "\n";
}

// Calculates the volume of a 3D rectangular prism.
int calculate_volume(int length, int width, int height) {
    return length * width * height;
}

// =============================================================================
// TODO EXERCISES - FILL IN THE BLANKS
// =============================================================================

int main() {
    std::cout << "--- TASK 1: PARTIAL APPLICATION (TODO) ---\n";
    
    // We want to create a partially applied function for a specific store 
    // ("TechMart") that always applies an 8% tax rate (0.08). 
    // The resulting function should ONLY take the 'base_amount' as an argument.
    //
    // TODO: Implement the lambda from scratch so it takes just the base_amount,
    //       and calls print_receipt internally with the hardcoded values.
    
    auto techmart_receipt = /* TODO: Implement your lambda here */;

    // UNCOMMENT TO TEST TASK 1:
    // techmart_receipt(100.0); // Expected Output: [TechMart] Total: $108
    // techmart_receipt(50.0);  // Expected Output: [TechMart] Total: $54


    std::cout << "\n--- TASK 2: CURRYING (TODO) ---\n";

    // Currying transforms a function taking multiple arguments into a chain
    // of nested single-argument functions. 
    // Example: f(a, b, c) becomes f(a)(b)(c)
    //
    // TODO: Implement the rest of the curried function chain below. 
    //       It should take length, return a lambda taking width, which returns
    //       a lambda taking height, which finally calls calculate_volume.
    //       Don't forget to capture the outer variables!

    auto curried_volume = [](int length) {
        // TODO: Complete the inner lambdas here
    };

    // UNCOMMENT TO TEST TASK 2:
    // int vol = curried_volume(2)(5)(10);
    // std::cout << "Curried Volume: " << vol << "\n"; // Expected Output: Curried Volume: 100


    std::cout << "\n--- BONUS TASK: STD::BIND (TODO) ---\n";
    
    // Before lambdas became powerful, C++11 used std::bind for partial application.
    //
    // TODO: Use std::bind to create 'legacy_techmart_receipt' similar to Task 1.
    //       Remember to use std::placeholders::_1 for the missing argument.

    // auto legacy_techmart_receipt = /* TODO: Use std::bind here */;
    
    // UNCOMMENT TO TEST BONUS TASK:
    // legacy_techmart_receipt(200.0); // Expected Output: [TechMart] Total: $216

    return 0;
}
