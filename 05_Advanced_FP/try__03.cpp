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
// FIXME EXERCISES - FIX THE BUGGY CODE
// =============================================================================

int main() {
    std::cout << "--- TASK 1: PARTIAL APPLICATION (FIXME) ---\n";
    
    // We want a partially applied function for "TechMart" that always applies 
    // an 8% tax rate (0.08), leaving ONLY 'base_amount' to be provided later.
    //
    // FIXME: The lambda below requires 3 arguments when called, defeating the 
    //        purpose of partial application! Modify its signature and body so 
    //        it only requires 'base_amount'.
    
    auto techmart_receipt = [](const std::string& store, double tax, double base_amount) {
        print_receipt(store, tax, base_amount); 
    };

    // UNCOMMENT TO TEST TASK 1:
    // techmart_receipt(100.0); // Expected Output: [TechMart] Total: $108
    // techmart_receipt(50.0);  // Expected Output: [TechMart] Total: $54


    std::cout << "\n--- TASK 2: CURRYING (FIXME) ---\n";

    // Currying transforms f(a, b, c) into f(a)(b)(c).
    //
    // FIXME: The nested lambdas below won't compile! In C++, inner lambdas 
    //        do not automatically have access to outer parameters. Fix the 
    //        capture lists so the inner lambdas can see 'length' and 'width'.

    auto curried_volume = [](int length) {
        return [](int width) { // FIXME: Something is missing in this capture list
            return [](int height) { // FIXME: Something is missing in this capture list
                return calculate_volume(length, width, height);
            };
        };
    };

    // UNCOMMENT TO TEST TASK 2:
    // int vol = curried_volume(2)(5)(10);
    // std::cout << "Curried Volume: " << vol << "\n"; // Expected Output: Curried Volume: 100


    std::cout << "\n--- BONUS TASK: STD::BIND (FIXME) ---\n";
    
    // FIXME: This std::bind call is broken. It is missing a placeholder for 
    //        the argument that will be passed in dynamically (base_amount).
    //        Add the correct std::placeholders value.

    auto legacy_techmart_receipt = std::bind(print_receipt, "TechMart", 0.08);
    
    // UNCOMMENT TO TEST BONUS TASK:
    // legacy_techmart_receipt(200.0); // Expected Output: [TechMart] Total: $216

    return 0;
}
