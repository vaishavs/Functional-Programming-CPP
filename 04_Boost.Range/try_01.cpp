/*
Exercise: The Filtered Transformer

The Goal:
The task is to process the given list of integers to find specific numbers, modify them, and print the results — 
all without writing a single manual for loop or using std::begin/std::end pairs.

The Requirements
1.  Filter: Keep only the even numbers.
2.  Transform: Square each of those even numbers.
3.  Reverse: Reverse the resulting sequence.
4.  Output: Print the final sequence to the console.

Expected output ->
Processed Numbers: 100 64 36 16 4
*/

#include <iostream>
#include <vector>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <iterator>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // The task: Create a range pipeline using | 
    // 1. Filter evens: n % 2 == 0
    // 2. Transform: n * n
    // 3. Reverse the order
    
    auto result = numbers 
        /* TODO: Add adaptors here */ ;

    // Use boost::copy to print the result
    std::cout << "Processed Numbers: ";
    boost::copy(result, std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;

    return 0;
}
