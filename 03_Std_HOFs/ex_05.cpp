// This file defines a custom RANGE ADAPTOR using sensor data logging.
// It lazily returns a new range.

#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>

// =============================================================================
// 1. THE CUSTOM VIEW CLASS
// =============================================================================
// std::ranges::view_interface automatically provides standard view features
// like .empty(), .size(), .front(), and container-like conversions for free!
template <std::ranges::view V>
class clamp_view : public std::ranges::view_interface<clamp_view<V>> {
private:
    V base_ = V();          // Stores the wrapped underlying view/range
    double min_val_{0.0};   // Lower clamping boundary
    double max_val_{0.0};   // Upper clamping boundary

public:
    // Default constructor is required by C++20 std::ranges::view concept
    clamp_view() = default;

    // Constructor that stores the input range and clamping limits
    clamp_view(V base, double min_val, double max_val)
        : base_(std::move(base)), min_val_(min_val), max_val_(max_val) {}

    // -------------------------------------------------------------------------
    // 2. CUSTOM ITERATOR
    // -------------------------------------------------------------------------
    // C++20 requires far less boilerplate for iterators than C++11/14.
    // We only need two type aliases and four core operator overloads.
    class iterator {
    private:
        std::ranges::iterator_t<V> current_{}; // Underlying iterator from the base range
        double min_val_{0.0};                  // Min limit passed to std::clamp
        double max_val_{0.0};                  // Max limit passed to std::clamp

    public:
        // Required by C++20 range concepts to inspect element and distance types
        using value_type      = double;
        using difference_type = std::ptrdiff_t;

        // Default constructor (required by std::input_or_output_iterator concept)
        iterator() = default;

        // Custom constructor to bind iterator position with current clamp bounds
        iterator(std::ranges::iterator_t<V> current, double min_val, double max_val)
            : current_(current), min_val_(min_val), max_val_(max_val) {}

        // --- OPERATOR 1: Dereference (*it) ---
        // THE LAZY MAGIC HAPPENS HERE: We don't change original vector data.
        // The value is read from the underlying iterator and clamped ON-THE-FLY
        // only when the caller asks for it!
        double operator*() const {
            return std::clamp(static_cast<double>(*current_), min_val_, max_val_);
        }

        // --- OPERATOR 2: Prefix Increment (++it) ---
        // Advances the position of the underlying wrapped iterator
        iterator& operator++() {
            ++current_;
            return *this;
        }

        // --- OPERATOR 3: Postfix Increment (it++) ---
        // C++20 allows postfix increment to return void for simple input iterators!
        void operator++(int) {
            ++current_;
        }

        // --- OPERATOR 4: Equality Check (it == end_it) ---
        // Determines loop completion by comparing the underlying iterator position.
        // C++20 automatically synthesizes operator!= from this single operator!
        bool operator==(const iterator& other) const {
            return current_ == other.current_;
        }
    };

    // Range begin: returns our simplified iterator initialized at start of range
    auto begin() { 
        return iterator(std::ranges::begin(base_), min_val_, max_val_); 
    }

    // Range end: returns our simplified iterator initialized at end of range
    auto end() { 
        return iterator(std::ranges::end(base_), min_val_, max_val_); 
    }
};

// DEDUCTION GUIDE:
// Tells the compiler how to convert raw containers (like std::vector) into 
// a valid view (std::views::all_t) when constructing clamp_view directly.
template <class R>
clamp_view(R&&, double, double) -> clamp_view<std::views::all_t<R>>;


// =============================================================================
// 3. PIPELINE ADAPTOR SETUP (Enables: range | custom_views::clamp(min, max))
// =============================================================================
namespace custom_views {

    // A closure struct that stores parameters until the pipe operator '|' is used
    struct clamp_adaptor_closure {
        double min_val;
        double max_val;

        // Friend operator| overload:
        // Triggers when evaluating: range | clamp_adaptor_closure
        // Left side ('r') = input range, Right side ('closure') = saved parameters
        template <std::ranges::viewable_range R>
        friend auto operator|(R&& r, const clamp_adaptor_closure& closure) {
            return clamp_view(std::forward<R>(r), closure.min_val, closure.max_val);
        }
    };

    // Public factory function that users actually call in code.
    // Calling clamp(0.0, 5.0) returns the closure holding those numbers.
    inline auto clamp(double min_val, double max_val) {
        return clamp_adaptor_closure{min_val, max_val};
    }
}


// =============================================================================
// 4. MAIN EXECUTION
// =============================================================================
int main() {
    // Original noisy sensor data
    std::vector<double> voltage_readings = {-1.5, 0.2, 3.5, 6.2, 2.0, -0.4, 5.0};

    std::cout << "Original Readings:\n";
    for (double v : voltage_readings) {
        std::cout << v << "V ";
    }
    std::cout << "\n\n";

    // Combine custom pipe view with std::views
    auto safe_readings = voltage_readings 
                       | custom_views::clamp(0.0, 5.0)                       // Our custom view
                       | std::views::filter([](double v) { return v > 0.0; }); // Standard C++20 view

    std::cout << "Clamped & Filtered Safe Readings (0.0V to 5.0V):\n";
    for (double v : safe_readings) {
        std::cout << v << "V "; // Values strictly evaluated here on iteration!
    }
    std::cout << "\n";

    return 0;
}
