## Extending an algorithm
The construction of a custom algorithm within the Boost.Range ecosystem is achieved by moving beyond simple iterator pairs and embracing the library's trait-based architecture. It is through this architecture that the algorithm is ensured to work not just with standard containers, but also with adapted ranges, filtered views, and even raw arrays.

The following guide details the construction of a `find_last` algorithm—by which the last occurrence of a value in a range is located—using the professional conventions found within the Boost source tree.

#### Step 1: Defining the Range Traits
The determination of the correct types for iterators and values, without the assumption that the input is a specific container like `std::vector`, is the first requirement for a robust range algorithm. These pieces of information metadata are extracted through header-only traits provided by Boost. Through the use of `boost::range_iterator<Range>::type`, the algorithm is automatically adjusted according to whether a const or non-const range is being processed.
```
#include <boost/range/functions.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/traversal.hpp>

template<typename ForwardRange, typename Value>
inline typename boost::range_iterator<ForwardRange>::type
find_last(ForwardRange& rng, const Value& val)
{
    // Step 2 & 3 logic goes here
}
```

#### Step 2: Concept Assertion
Requirements are enforced at compile time before any logic is executed. If the ability to step backward is required by the algorithm (e.g., for performance), an assertion must be made that the `BidirectionalRangeConcept` is satisfied by the range. Cryptic template error messages deep in the compilation stack are prevented by this practice.

`BOOST_CONCEPT_ASSERT((boost::BidirectionalRangeConcept<ForwardRange>));`

#### Step 3: Implementation via Range Accessors
The standalone `boost::begin(rng)` and `boost::end(rng)` functions are used instead of calling `rng.begin()`. This distinction is considered critical; these functions are overloaded so that C-style arrays and pointer pairs, which do not possess member functions, can be handled.

```
template<typename ForwardRange, typename Value>
inline typename boost::range_iterator<ForwardRange>::type
find_last(ForwardRange& rng, const Value& val)
{
    using iter_t = typename boost::range_iterator<ForwardRange>::type;
    
    iter_t result = boost::end(rng);
    iter_t it = boost::begin(rng);
    iter_t last = boost::end(rng);

    for (; it != last; ++it) {
        if (*it == val) {
            result = it;
        }
    }
    return result;
}
```

#### Step 4: Supporting Range Composition (The Pipe Operator)
To allow the algorithm to be used in a functional pipeline (e.g., `my_range | find_last(x)`), a "Range Adaptor" must be implemented. This is accomplished through the creation of a function object where the arguments are stored, and the overloading of operator `|`.
```
namespace detail {
    template<typename T>
    struct find_last_forwarder {
        T value;
        find_last_forwarder(T val) : value(val) {}
    };
}

// The pipe operator overload
template<typename ForwardRange, typename T>
inline typename boost::range_iterator<ForwardRange>::type
operator|(ForwardRange& rng, const detail::find_last_forwarder<T>& f)
{
    return find_last(rng, f.value);
}

// Helper function to create the forwarder
template<typename T>
inline detail::find_last_forwarder<T> find_last(T val)
{
    return detail::find_last_forwarder<T>(val);
}
```

#### Step 5: Verifying the implementation
With this structure, the custom algorithm is rendered indistinguishable from built-in Boost tools. Type deduction is handled, const-correctness is respected, and the declarative syntax preferred in modern C++ development is supported.
```
#include <vector>
#include <iostream>

int main() {
    std::vector<int> data = {1, 2, 3, 2, 1};
    
    // Usage 1: Direct Call
    auto it1 = find_last(data, 2);
    
    // Usage 2: Pipe Syntax (Functional style)
    auto it2 = data | find_last(2);

    if (it2 != boost::end(data)) {
        std::cout << "Found at index: " << std::distance(boost::begin(data), it2);
    }
}
```
The decoupling of code from specific data structures is ensured by this tiered approach, while the full optimization benefits of the Boost.Range iterator abstractions are gained.

The complete implementation looks like this:
```
#include <iostream>
#include <vector>
#include <string>
#include <boost/range/functions.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/traversal.hpp>

// =========================================================
// 1. ALGORITHM DEFINITION
// =========================================================

/**
 * find_last: Locates the last occurrence of a value in a range.
 * This implementation follows Boost.Range conventions by using
 * range_iterator traits and boost::begin/end accessors.
 */
template<typename ForwardRange, typename Value>
inline typename boost::range_iterator<ForwardRange>::type
find_last(ForwardRange& rng, const Value& val)
{
    // Step 2: Concept Assertion
    // Ensures the range provides at least forward traversal at compile-time
    BOOST_CONCEPT_ASSERT((boost::ForwardRangeConcept<ForwardRange>));

    // Step 1 & 3: Range Traits and Accessors
    using iter_t = typename boost::range_iterator<ForwardRange>::type;
    
    iter_t result = boost::end(rng);
    iter_t it = boost::begin(rng);
    iter_t last = boost::end(rng);

    for (; it != last; ++it) {
        if (*it == val) {
            result = it;
        }
    }
    return result;
}

// =========================================================
// 2. PIPE OPERATOR SUPPORT (ADAPTOR)
// =========================================================

namespace detail {
    // Holder struct to store the value between the function call and the pipe
    template<typename T>
    struct find_last_forwarder {
        T value;
        find_last_forwarder(T val) : value(val) {}
    };
}

// Overload operator| to enable: range | find_last(val)
template<typename ForwardRange, typename T>
inline typename boost::range_iterator<ForwardRange>::type
operator|(ForwardRange& rng, const detail::find_last_forwarder<T>& f)
{
    return find_last(rng, f.value);
}

// Helper function that creates the forwarder object
template<typename T>
inline detail::find_last_forwarder<T> find_last(T val)
{
    return detail::find_last_forwarder<T>(val);
}

// =========================================================
// 3. EXECUTION
// =========================================================

int main() {
    // Works with standard containers
    std::vector<int> numbers = {10, 20, 30, 20, 10, 50};
    
    // Usage 1: Direct Call 
    // Added [[maybe_unused]] to tell the compiler this is intentional
    [[maybe_unused]] auto it1 = find_last(numbers, 20);
    
    // Usage 2: Pipe Syntax
    auto it2 = numbers | find_last(20);

    if (it2 != boost::end(numbers)) {
        std::cout << "Vector: Last 20 found at index " 
                  << std::distance(boost::begin(numbers), it2) << "\n";
    }

    // Works with C-style arrays due to boost::begin/end accessors
    int arr[] = {5, 1, 5, 2};
    auto it3 = arr | find_last(5);
    
    if (it3 != boost::end(arr)) {
        std::cout << "Array: Last 5 found at index " 
                  << std::distance(boost::begin(arr), it3) << "\n";
    }

    return 0;
}
```

Sources:
* [Extending Boost.Range](https://www.boost.org/doc/libs/latest/libs/range/doc/html/range/reference/extending.html)
* [Using boost::range](https://www.boost.org/doc/libs/latest/libs/numeric/odeint/doc/html/boost_numeric_odeint/odeint_in_detail/using_boost__range.html)
* [Boost Range for humans](https://www.caichinger.com/boost-range/index.html)
