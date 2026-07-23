#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>

// =============================================================================
// 1. CUSTOM VIEW ADAPTOR (Lazy Execution)
// =============================================================================
// Views are LAZY. Computation is deferred until iteration actually occurs.
// The following view applies a grading curve (adds bonus points) and caps 
// scores at 100.
template <std::ranges::view V>
class curve_view : public std::ranges::view_interface<curve_view<V>> {
private:
    V base_ = V();          // The underlying range/view
    double bonus_{0.0};     // The curve points to add

public:
    curve_view() = default;
    curve_view(V base, double bonus) : base_(std::move(base)), bonus_(bonus) {}

    // SIMPLE FORWARD ITERATOR
    class iterator {
    private:
        std::ranges::iterator_t<V> current_{};
        double bonus_{0.0};

    public:
        // C++20 concepts require specific tags to define iterator capabilities
        using iterator_concept = std::forward_iterator_tag; 
        using value_type       = double;
        using difference_type  = std::ptrdiff_t;

        iterator() = default;
        iterator(std::ranges::iterator_t<V> current, double bonus)
            : current_(current), bonus_(bonus) {}

        // --- DEREFERENCE (The Lazy Calculation) ---
        // Adds the bonus and caps at 100.0 ONLY when a specific element is read.
        double operator*() const {
            double curved_score = static_cast<double>(*current_) + bonus_;
            return std::min(100.0, curved_score); 
        }

        // --- PREFIX INCREMENT (++it) ---
        iterator& operator++() {
            ++current_;
            return *this;
        }

        // --- POSTFIX INCREMENT (it++) ---
        // Must return a copy of the old state to satisfy forward_iterator_tag
        iterator operator++(int) {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        // --- EQUALITY (it == end) ---
        // C++20 auto-generates operator!= from a single equality function
        bool operator==(const iterator& other) const {
            return current_ == other.current_;
        }
    };

    // Range interface hooks
    auto begin() { return iterator(std::ranges::begin(base_), bonus_); }
    auto end()   { return iterator(std::ranges::end(base_), bonus_); }
};

// Deduction guide: Allows standard containers to be wrapped into the view directly
template <class R>
curve_view(R&&, double) -> curve_view<std::views::all_t<R>>;

// Pipeline Adaptor for pipe syntax ( range | views::curve(10) )
namespace custom_views {
    struct curve_closure {
        double bonus;
        
        template <std::ranges::viewable_range R>
        friend auto operator|(R&& r, const curve_closure& closure) {
            return curve_view(std::forward<R>(r), closure.bonus);
        }
    };

    inline auto curve(double bonus) { return curve_closure{bonus}; }
}


// =============================================================================
// 2. CUSTOM RANGE ALGORITHM (Eager Execution)
// =============================================================================
// Algorithms are EAGER. Execution happens immediately, iterating through the 
// provided range to compute a result or mutate data in place.
namespace custom_algos {
    
    // std::ranges::input_range concept ensures the function accepts ANY valid 
    // range, view, or container (vector, list, the custom view, etc.)
    template <std::ranges::input_range R>
    double pass_rate(R&& range, double pass_threshold) {
        int total_students = 0;
        int passed_students = 0;

        // Eagerly consumes the range immediately when called
        for (double score : range) {
            total_students++;
            if (score >= pass_threshold) {
                passed_students++;
            }
        }

        if (total_students == 0) return 0.0;
        return (static_cast<double>(passed_students) / total_students) * 100.0;
    }
}


// =============================================================================
// 3. MAIN EXECUTION
// =============================================================================
int main() {
    // 1. Raw Data (Original exam scores)
    std::vector<double> exam_scores = {45.0, 82.5, 95.0, 58.0, 49.5, 99.0};
    double passing_grade = 60.0;

    std::cout << "--- RAW SCORES ---\n";
    for (double score : exam_scores) std::cout << score << " ";
    std::cout << "\nRaw Pass Rate: " 
              << custom_algos::pass_rate(exam_scores, passing_grade) << "%\n\n";

    // 2. Build the Lazy View Pipeline
    // Notice: Nothing is actually computed on the following line!
    auto final_grades_view = exam_scores 
                           | custom_views::curve(12.0); // +12 point curve

    // 3. Combine View and Algorithm
    // The algorithm eagerly pulls data from the lazy view. 
    // The view applies the +12 curve *as* the algorithm asks for each element.
    std::cout << "--- CURVED SCORES ---\n";
    for (double score : final_grades_view) std::cout << score << " ";
    
    double new_pass_rate = custom_algos::pass_rate(final_grades_view, passing_grade);
    std::cout << "\nCurved Pass Rate: " << new_pass_rate << "%\n";

    return 0;
}
