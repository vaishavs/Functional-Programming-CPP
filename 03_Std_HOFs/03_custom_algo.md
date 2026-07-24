# Custom algorithm

Developing a standard-compliant C++20 range algorithm requires utilizing concepts to ensure type safety and proper resolution. The `count_if` algorithm variant can be implemented as a standard constrained function template that operates directly on range elements.

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

#### Step 2. Define and Constrain the Function Template:

Instead of a structural function object, define a standard template function. Applying C++20 concepts guarantees that compilation will fail with clear error messages if incompatible types are provided.

```cpp
template <std::ranges::input_range R, class Pred>
requires std::indirect_unary_predicate<Pred, std::ranges::iterator_t<R>>
constexpr std::ranges::range_difference_t<R>
count_if(R&& r, Pred pred)

```

* `std::ranges::input_range` restricts `R` to types that can be iterated over at least once.
* `std::indirect_unary_predicate` ensures `Pred` can be invoked with the dereferenced iterator of the range and will return a boolean-testable type.
* `std::ranges::range_difference_t<R>` extracts the correct return type to ensure compatibility with standard iterators.

#### Step 3. Implement the Iteration Logic:

Inside the function body, the predicate operates directly on the elements yielded by the range.

```cpp
{
    std::ranges::range_difference_t<R> count = 0;
    
    for (auto&& elem : r) {
        if (pred(elem)) {
            ++count;
        }
    }
    return count;
}

```

#### Step 4. Execute the Algorithm:

The constrained function is now ready for deployment against any valid C++20 range, including standard containers and range adaptors.

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    auto is_even = [](int n) { return n % 2 == 0; };

    // The custom algorithm processes the range directly
    auto even_count = count_if(numbers, is_even);

    std::cout << "Even numbers found: " << even_count << '\n';

    return 0;
}

```

And this is the complete example:

```cpp
#include <ranges>
#include <concepts>
#include <iterator>
#include <iostream>
#include <vector>

// 1 & 2. Define the constrained function template
template <std::ranges::input_range R, class Pred>
requires std::indirect_unary_predicate<Pred, std::ranges::iterator_t<R>>
constexpr std::ranges::range_difference_t<R>
count_if(R&& r, Pred pred) {
    
    // 3. Implement the iteration logic
    std::ranges::range_difference_t<R> count = 0;
    
    for (auto&& elem : r) {
        if (pred(elem)) {
            ++count;
        }
    }
    return count;
}

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    // Define a predicate
    auto is_even = [](int n) { return n % 2 == 0; };

    // 4. Execute the custom algorithm
    auto even_count = count_if(numbers, is_even);

    std::cout << "Even numbers found: " << even_count << '\n';

    return 0;
}

```

Sources:

* https://www.oreilly.com/library/view/c-cookbook/0596007612/ch07s11.html
* https://stackoverflow.com/questions/11993575/how-can-i-write-a-custom-algorithm-that-works-on-either-a-vector-or-map-iterator

