/* A simple Boost.Range example */
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <vector>
#include <iostream>

int main() {
    std::vector<int> nums = {1, 2, 3, 4, 5, 6};

    using namespace boost::adaptors;

    std::vector<int> result;

    boost::copy(
        nums
            | filtered([](int x) { return x % 2 == 0; })   // keep even numbers
            | transformed([](int x) { return x * x; }),    // square them
        std::back_inserter(result)
    );

    // Print result
    for (int x : result) {
        std::cout << x << " ";
    }
}
