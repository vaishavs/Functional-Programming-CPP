# Bonus: How to extend a Boost.Range iterator
A significant amount of boilerplate code is required to be written when a custom iterator is implemented from scratch in C++. To simplify this process, **`boost::iterator_facade`** is provided by the Boost library, by which all standard iterator operators are generated based on a few core functions provided by the developer. Once the custom iterator is built, it can be easily converted into a Range using **`boost::iterator_range`**, by which it is made fully compatible with Boost.Range algorithms and C++ range-based `for` loops.

A step-by-step guide on how a custom iterator and range are implemented using Boost is provided below.

### Scenario: The "Step" Iterator
A `step_iterator` will be created by which a sequence of integers is iterated over, but incremented by a specific step value (e.g., counted by 2s or 3s).

---

### Step 1: Including the headers
The `iterator_facade` is needed so the iterator can be built, and the `iterator_range` is required so it can be packaged.

```
#include <iostream>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/for_each.hpp> // Optional: for testing
```

### Step 2: Inheriting from `boost::iterator_facade`
The CRTP (Curiously Recurring Template Pattern) is used by Boost so that the required operators can be injected into the class. The custom iterator should inherit `iterator_facade`, and four template parameters must be provided:

1.  **Derived**: The iterator class name.
2.  **Value**: The type of element pointed to by the iterator.
3.  **Category**: The traversal category (e.g., `boost::forward_traversal_tag`).
4.  **Reference**: The return type of the dereference operator (returned by value here).

```
class step_iterator : public boost::iterator_facade<
    step_iterator,                   // 1. Derived class
    int,                             // 2. Value type
    boost::forward_traversal_tag,    // 3. Category (Forward Iterator)
    int                              // 4. Reference type 
> 
{
public:
    // Constructor
    step_iterator(int current, int step = 1) 
        : m_current(current), m_step(step) {}

private:
    // Access is granted to the facade so our private methods can be called
    friend class boost::iterator_core_access;

    // ... Core methods will be placed here ...

    int m_current;
    int m_step;
};
```

### Step 3: Implementing the Core Iterator Mechanics
For a `forward_traversal_tag` iterator to work, three private methods are strictly required to be implemented by `iterator_facade`: `increment()`, `equal()`, and `dereference()`. 

These must be added inside the `private` section of the class:

```
private:
    friend class boost::iterator_core_access;

    // How the iterator is moved forward is defined here (the ++ operator)
    void increment() {
        m_current += m_step;
    }

    // How iterators are compared is defined here (the == and != operators)
    bool equal(step_iterator const& other) const {
        // For this simple range, it is considered equal if the 'end' state is met or exceeded.
        return this->m_current >= other.m_current; 
    }

    // What happens when the iterator is dereferenced is defined here (the * operator)
    int dereference() const {
        return m_current;
    }
```

### Step 4: Creating the Boost Range
A Range in C++ is defined as a concept by which a `begin()` and an `end()` are possessed. `boost::make_iterator_range` is used as the easiest way a Boost.Range can be created out of custom iterators.

This can be wrapped in a clean factory function:

```
// A Boost Range is returned by this helper function
auto make_step_range(int start, int end, int step) {
    // The start iterator is created
    step_iterator begin_iter(start, step);
    
    // The end iterator is created. 
    step_iterator end_iter(end, step);

    return boost::make_iterator_range(begin_iter, end_iter);
}
```

### Step 5: Testing the Custom Range
The custom range can now be used in standard C++11 range-based `for` loops, or it can be passed directly to Boost.Range algorithms.

```
int main() {
    std::cout << "Counting from 0 to 10 by 2s:\n";
    
    // 1. A standard range-based for loop is used
    for (int val : make_step_range(0, 10, 2)) {
        std::cout << val << " "; 
    }
    std::cout << "\n\n";

    std::cout << "Counting from 1 to 15 by 3s (using Boost algorithm):\n";
    
    // 2. Boost.Range algorithms are used directly
    auto my_range = make_step_range(1, 15, 3);
    boost::range::for_each(my_range, [](int val) {
        std::cout << val << " ";
    });
    std::cout << "\n";

    return 0;
}
```

### Summary of Requirements by Iterator Category
Different methods are required to be provided to `iterator_facade` if the iterator is upgraded to a higher category in the future:

* **Forward Iterator:** `increment()`, `equal()`, and `dereference()` are required.
* **Bidirectional Iterator:** Everything above is required, plus `decrement()`.
* **Random Access Iterator:** Everything above is required, plus `advance(n)` and `distance_to(other)`.
