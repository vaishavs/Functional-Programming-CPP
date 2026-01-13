#include <iostream>
#include <functional>
#include <string>

// Target function: increments a value and prints a message
void processor(int& val, const std::string& msg) {
    val++;
    std::cout << msg << ": " << val << std::endl;
}

int main() {
    int count = 10;
    std::string text = "Processing";

    // BUG 1: Unexpected Logic (Value vs Reference)
    // Goal: Create a function that increments the original 'count'.
    auto task1 = std::bind(processor, count, text);
    task1(); 
    std::cout << "Count after task1: " << count << "\n"; // Why is it still 10?

    // BUG 2: Compilation Error (Const Correctness)
    // Goal: Pass 'text' as a reference to save memory, but keep it read-only.
    // auto task2 = std::bind(processor, std::cref(count), std::cref(text));
    // task2(); // ERROR: Why won't this line compile?

    // BUG 3: Dangling Reference (Lifetime)
    // Goal: Delay execution of a task referencing a local variable.
    std::function<void()> task3;
    {
        int temp_val = 50;
        task3 = std::bind(processor, std::ref(temp_val), "Temp");
    } 
    // task3(); // DANGER: What happens if we call this now?

    // BUG 4: Argument Type (R-values)
    // Goal: Bind a constant literal value as a reference.
    // auto task4 = std::bind(processor, std::ref(100), "Literal"); // ERROR
    
    return 0;
}
