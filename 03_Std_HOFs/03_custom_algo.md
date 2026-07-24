# Custom algorithm
Developing a standard-compliant C++20 range algorithm requires utilizing concepts and function objects to ensure type safety and proper resolution. The following procedure outlines the creation of a `count_if` algorithm variant that operates directly on range elements without implementing standard library projection mechanics.

#### Step 1. Include the Required Headers:

Specific standard library headers provide the necessary infrastructure for ranges and concepts.

```cpp
#include <ranges>
#include <concepts>
#include <iterator>

```

* `<ranges>` supplies core range utilities and types.
* `<concepts>` provides foundational compile-time constraints.
* `<iterator>` furnishes iterator-specific constraints like `std::indirect_unary_predicate`.


#### Step 2. Define a Function Object Structure:

Standard range algorithms are typically implemented as function objects (often referred to as Niebloids) rather than plain template functions. This structure prevents Argument-Dependent Lookup (ADL) interference and allows the algorithm to be passed as an argument to other functions.

```cpp
namespace custom_algo {
    struct count_if_fn {
        // Template implementation will go here
    };
}

```


#### Step 3. Constrain Template Parameters with Concepts:
The call operator must accept a range and a predicate. Applying C++20 concepts guarantees that compilation will fail with clear error messages if incompatible types are provided.

```cpp
template <std::ranges::input_range R, class Pred>
requires std::indirect_unary_predicate<Pred, std::ranges::iterator_t<R>>

```

* `std::ranges::input_range` restricts `R` to types that can be iterated over at least once.
* `std::indirect_unary_predicate` ensures `Pred` can be invoked with the dereferenced iterator of the range and will return a boolean-testable type.


#### Step 4. Implement the Iteration Logic:
Define the `operator()` utilizing the constrained template parameters. Extract the correct return type using `std::ranges::range_difference_t<R>` to ensure compatibility with standard iterators. Because projection support is explicitly omitted, the predicate operates directly on the elements yielded by the range.

```cpp
constexpr std::ranges::range_difference_t<R>
operator()(R&& r, Pred pred) const {
    std::ranges::range_difference_t<R> count = 0;
    
    for (auto&& elem : r) {
        if (pred(elem)) {
            ++count;
        }
    }
    return count;
}

```


#### Step 5. Instantiate the Customization Point Object:

Define an inline `constexpr` instance of the struct within the namespace. This provides the global entry point for invoking the algorithm.

```cpp
namespace custom_algo {
    struct count_if_fn {
        // ... (implementation from above) ...
    };

    inline constexpr count_if_fn count_if{};
}

```


#### Step 6. Execute the Algorithm:

The algorithm is now ready for deployment against any valid C++20 range, including standard containers and range adaptors.

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    auto is_even = [](int n) { return n % 2 == 0; };

    // The custom algorithm processes the range directly
    auto even_count = custom_algo::count_if(numbers, is_even);

    std::cout << "Even numbers found: " << even_count << '\n';

    return 0;
}

```

And this is the complete example:
```
#include <ranges>
#include <concepts>
#include <iterator>
#include <iostream>
#include <vector>

namespace custom_algo {

    // 1. Define the function object (Niebloid)
    struct count_if_fn {
        
        // 2. Constrain template parameters
        template <std::ranges::input_range R, class Pred>
        requires std::indirect_unary_predicate<Pred, std::ranges::iterator_t<R>>
        
        // 3. Implement the iteration logic
        constexpr std::ranges::range_difference_t<R>
        operator()(R&& r, Pred pred) const {
            std::ranges::range_difference_t<R> count = 0;
            
            for (auto&& elem : r) {
                if (pred(elem)) {
                    ++count;
                }
            }
            return count;
        }
    };

    // 4. Instantiate the Customization Point Object
    inline constexpr count_if_fn count_if{};

} // namespace custom_algo

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    // Define a predicate
    auto is_even = [](int n) { return n % 2 == 0; };

    // 5. Execute the custom algorithm
    auto even_count = custom_algo::count_if(numbers, is_even);

    std::cout << "Even numbers found: " << even_count << '\n';

    return 0;
}
```

Sources:

* https://www.oreilly.com/library/view/c-cookbook/0596007612/ch07s11.html
* https://stackoverflow.com/questions/11993575/how-can-i-write-a-custom-algorithm-that-works-on-either-a-vector-or-map-iterator

