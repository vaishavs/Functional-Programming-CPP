#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <iomanip>

int main() {
    // Student data structure: name, {age, grade}
    std::vector<std::pair<std::string, std::pair<int, double>>> students = {
        {"Alice", {20, 85.5}},
        {"bob", {17, 92.0}},
        {"Amy", {22, 78.2}},
        {"Charlie", {19, 88.1}},
        {"Anna", {21, 95.3}},
        {"Diana", {16, 76.4}}
    };
    
    // Expected: Average of Alice(85.5), Amy(78.2), Anna(95.3) = 86.3
    std::vector<double> grades;  // BUG #1: Empty output vector!
                                  // HINT: grades.begin() == grades.end() on empty vector!
    
    // BUG #2: Wrong copy_if signature (takes 4 args in C++20, not 5)
    // BUG #3: std::string::startsWith doesn't exist (use starts_with C++20 or find())
    // BUG #4: Extra transform lambda - copy_if doesn't transform!
    std::copy_if(students.begin(), students.end(), grades.begin(), 
        [](const auto& s) { 
            // HINT: Check compiler error: "no member named 'startsWith'"
            return s.first.startsWith("A");  
        },                                 
        [](const auto& s) { 
            // HINT: copy_if copies elements AS-IS, doesn't transform!
            return s.second.second;          
        });
    
    // CRASH #1 COMPLETE: Dereferencing empty vector's begin() iterator
    
    // BUG #5: grades.end() from failed copy_if operation (still empty!)
    auto invalid_it = grades.end();
    
    // BUG #6: Lambda missing return statement = UNDEFINED BEHAVIOR!
    // HINT: Lambdas must explicitly return unless void
    std::transform(students.begin(), students.end(), invalid_it,
        [](const auto& s) { 
            // HINT: Check compiler warning: "not all control paths return value"
            s.second.second * 2;  // No return statement!
        });
    
    // BUG #7: sort() with invalid iterator from different container
    // HINT: grades.begin() and invalid_it don't belong to same container
    std::sort(grades.begin(), invalid_it);
    
    // BUG #8: accumulate on empty vector returns 0.0 safely, but...
    // BUG #9: Why multiply grades by 1.1? (curve grading gone wrong)
    double sum = std::accumulate(grades.begin(), grades.end(), 
        0.0, [](double acc, double g) { 
            // HINT: Grade inflation? Should be acc + g
            return acc + g * 1.1;  
        });
    
    // BUG #10: Divide by ALL students (6) instead of filtered ones (3)
    // HINT: grades.size() == 0 still, but even if fixed: wrong denominator!
    double avg = sum / students.size();
    
    // BUG #11: for_each first arg must be iterator range, not ostream!
    // BUG #12: for_each returns void - can't use in expression!
    std::for_each(std::cout, [](double x) { 
        // HINT: for_each([first, last), func) - not for_each(output, func)!
        std::cout << avg << std::endl;       
    });  // This line doesn't even compile!
    
    std::cout << "Result: " << avg << std::endl;  // Garbage value!
    return 0;
}
