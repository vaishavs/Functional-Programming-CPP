# Custom adaptor
Creating a custom view adaptor in modern C++ involves defining a lazily evaluated view class and a corresponding range adaptor object to support the pipeline syntax (`operator|`). The following procedure outlines the comprehensive construction of a `scale` adaptor, which lazily multiplies elements in a range by a scalar value, utilizing a streamlined C++20 iterator wrapper and omitting projection mechanics.

#### Step 1. Include Necessary Headers

Standard library headers provide the foundational concepts, traits, and view interfaces required for custom range implementations. `<type_traits>` is necessary for simplifying iterator type definitions.

```cpp
#include <ranges>
#include <concepts>
#include <iterator>
#include <utility>
#include <type_traits>

```


#### Step 2. Define the Custom View Class

A standard-compliant view must inherit from `std::ranges::view_interface` and store the underlying view alongside any required state. Template parameters must be constrained to ensure the base type models a view.

```cpp
namespace custom_views {
    template <std::ranges::input_range V, typename T>
    requires std::ranges::view<V>
    class scale_view : public std::ranges::view_interface<scale_view<V, T>> {
    private:
        V base_ = V();
        T factor_ = T();
        
        // Iterator definition will go here
    public:
        scale_view() = default;
        constexpr scale_view(V base, T factor) 
            : base_(std::move(base)), factor_(factor) {}
    };
    
    // Deduction guide for the view
    template <class R, class T>
    scale_view(R&&, T) -> scale_view<std::views::all_t<R>, T>;
}

```


#### Step 3. Implement the Iterator Wrapper
Views operate lazily through iterators. The custom iterator wraps the base view's iterator and applies the transformation logic within the dereference operator (`operator*`). C++20 features reduce manual boilerplate:

* **Defaulted Equality Operator:** `operator== = default;` automatically synthesizes the comparison logic.
* **Return Type Deduction using `auto`:** Removes the explicit trailing return type from `operator*()`.
* **Refined Type Aliases:** `std::remove_cvref_t` ensures `value_type` is always a clean, unqualified type.

```cpp
struct iterator {
    using base_iterator = std::ranges::iterator_t<V>;
    using difference_type = std::ranges::range_difference_t<V>;
    using value_type = std::remove_cvref_t<decltype(std::declval<std::ranges::range_reference_t<V>>() * std::declval<T>())>;

    base_iterator it_ = base_iterator();
    T factor_ = T();

    iterator() = default;
    constexpr iterator(base_iterator it, T factor) : it_(std::move(it)), factor_(factor) {}

    // Auto return type deduction
    constexpr auto operator*() const { return *it_ * factor_; }
    constexpr iterator& operator++() { ++it_; return *this; }
    constexpr iterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
    
    // C++20 defaulted comparison
    constexpr bool operator==(const iterator&) const = default;
    constexpr bool operator==(const std::ranges::sentinel_t<V>& s) const { return it_ == s; }
};

```


#### Step 4. Implement Range Accessors

The view must provide `begin()` and `end()` methods. `begin()` returns the custom iterator, while `end()` returns the base view's sentinel.

```cpp
constexpr auto begin() { 
    return iterator(std::ranges::begin(base_), factor_); 
}
constexpr auto end() { 
    return std::ranges::end(base_); 
}

```


#### Step 5. Define the Adaptor Closure

To enable the lazy pipeline syntax (`range | adaptor`), an intermediate closure object must capture the configuration arguments. This struct stores the arguments and provides a call operator that constructs the view when a range is eventually supplied.

```cpp
template <typename T>
struct scale_adaptor_closure {
    T factor_;
    
    constexpr scale_adaptor_closure(T factor) : factor_(factor) {}

    template <std::ranges::viewable_range R>
    constexpr auto operator()(R&& r) const {
        return scale_view(std::views::all(std::forward<R>(r)), factor_);
    }
};

```


#### Step 6. Overload the Pipeline Operator
The bitwise OR operator (`operator|`) serves as the bridge between the input range and the closure. This overload ensures that whenever a range appears on the left and the specific closure appears on the right, the closure is invoked with the provided range.

```cpp
template <std::ranges::viewable_range R, typename T>
constexpr auto operator|(R&& r, const scale_adaptor_closure<T>& closure) {
    return closure(std::forward<R>(r));
}

```


#### Step 7. Create the Adaptor Factory
A factory struct provides the primary interface. It must support two invocation styles: returning a closure when only the arguments are provided, and returning the fully constructed view when both the range and arguments are supplied simultaneously.

```cpp
struct scale_adaptor {
    // Supports: range | scale(factor)
    template <typename T>
    constexpr auto operator()(T factor) const {
        return scale_adaptor_closure<T>{factor};
    }
    
    // Supports: scale(range, factor)
    template <std::ranges::viewable_range R, typename T>
    constexpr auto operator()(R&& r, T factor) const {
        return scale_view(std::views::all(std::forward<R>(r)), factor);
    }
};

```


#### Step 8. Instantiate the Customization Point Object

An inline `constexpr` instance of the factory struct acts as the global entry point for the adaptor, matching standard library conventions found in `<ranges>`.

```cpp
inline constexpr scale_adaptor scale{};

```




## Final C++20 Implementation

```cpp
#include <ranges>
#include <concepts>
#include <iterator>
#include <utility>
#include <type_traits>
#include <iostream>
#include <vector>

namespace custom_views {

    // Step 2: Define the Custom View Class
    template <std::ranges::input_range V, typename T>
    requires std::ranges::view<V>
    class scale_view : public std::ranges::view_interface<scale_view<V, T>> {
    private:
        V base_ = V();
        T factor_ = T();

        // Step 3: Implement the Simplified Iterator Wrapper
        struct iterator {
            using base_iterator = std::ranges::iterator_t<V>;
            using difference_type = std::ranges::range_difference_t<V>;
            using value_type = std::remove_cvref_t<decltype(std::declval<std::ranges::range_reference_t<V>>() * std::declval<T>())>;

            base_iterator it_ = base_iterator();
            T factor_ = T();

            iterator() = default;
            constexpr iterator(base_iterator it, T factor) : it_(std::move(it)), factor_(factor) {}

            constexpr auto operator*() const { return *it_ * factor_; }
            
            constexpr iterator& operator++() { ++it_; return *this; }
            constexpr iterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
            
            constexpr bool operator==(const iterator&) const = default;
            constexpr bool operator==(const std::ranges::sentinel_t<V>& s) const { return it_ == s; }
        };

    public:
        scale_view() = default;
        constexpr scale_view(V base, T factor) 
            : base_(std::move(base)), factor_(factor) {}

        // Step 4: Implement Range Accessors
        constexpr auto begin() { return iterator(std::ranges::begin(base_), factor_); }
        constexpr auto end() { return std::ranges::end(base_); }
    };

    template <class R, class T>
    scale_view(R&&, T) -> scale_view<std::views::all_t<R>, T>;

    // Step 5: Define the Adaptor Closure
    template <typename T>
    struct scale_adaptor_closure {
        T factor_;
        constexpr scale_adaptor_closure(T factor) : factor_(factor) {}

        template <std::ranges::viewable_range R>
        constexpr auto operator()(R&& r) const {
            return scale_view(std::views::all(std::forward<R>(r)), factor_);
        }
    };

    // Step 6: Overload the Pipeline Operator
    template <std::ranges::viewable_range R, typename T>
    constexpr auto operator|(R&& r, const scale_adaptor_closure<T>& closure) {
        return closure(std::forward<R>(r));
    }

    // Step 7: Create the Adaptor Factory
    struct scale_adaptor {
        template <typename T>
        constexpr auto operator()(T factor) const {
            return scale_adaptor_closure<T>{factor};
        }
        
        template <std::ranges::viewable_range R, typename T>
        constexpr auto operator()(R&& r, T factor) const {
            return scale_view(std::views::all(std::forward<R>(r)), factor);
        }
    };

    // Step 8: Instantiate the Customization Point Object
    inline constexpr scale_adaptor scale{};

} // namespace custom_views

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    // Execute the custom view adaptor via pipeline syntax
    auto scaled_numbers = numbers | custom_views::scale(10);

    std::cout << "Scaled elements: ";
    for (int n : scaled_numbers) {
        std::cout << n << " ";
    }
    std::cout << '\n';

    return 0;
}

```
