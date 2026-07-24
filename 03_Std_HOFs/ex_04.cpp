// This file defines a custom RANGE ALGORITHM. It eagerly walks a range
// and returns a value, rather than lazily returning a new range.
//
// The `longest_streak_if` algorithm returns the length of the longest
// run of consecutive elements in a range that satisfy a predicate.

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

template <std::ranges::input_range R,
          std::indirect_unary_predicate<std::ranges::iterator_t<R>> Pred>
// This function scans the range once, from left to right, and
// returns the length of the longest streak it finds.
constexpr std::ranges::range_difference_t<R>
longest_streak_if(R&& r, Pred pred) {
    // This variable holds the longest streak found so far.
    std::ranges::range_difference_t<R> longest = 0;
    // This variable holds the length of the streak currently in progress.
    std::ranges::range_difference_t<R> current = 0;

    // Walk every element of the range exactly once, in order.
    for (auto&& elem : r) {
        // Check whether this element satisfies the predicate.
        if (std::invoke(pred, elem)) {
            // Extend the current streak by one element.
            current += 1;
            // Update the longest streak if the current one is now bigger.
            longest = std::max(longest, current);
        } else {
            // The streak is broken here, so reset the current count to zero.
            current = 0;
        }
    }
    // Return the longest streak found across the whole range.
    return longest;
}

// --------------------------------------------------------------------
// Real-world use: longest workout streak from a habit tracker's log.
// --------------------------------------------------------------------

// This struct represents a single day's entry in a habit tracker.
struct DayLog {
    std::string day;      // This field stores the day's short name, e.g. "Mon".
    bool workout_done;    // This field records whether the workout was completed.
};

int main() {
    // This vector holds one week of workout logs, Monday through Sunday.
    std::vector<DayLog> week{
        {"Mon", true}, {"Tue", true}, {"Wed", false}, {"Thu", true},
        {"Fri", true}, {"Sat", true}, {"Sun", true},
    };

    // Call the custom algorithm over the whole week, using a lambda
    // that reads the workout_done field as the predicate.
    auto streak = longest_streak_if(
        week, [](const DayLog& d) { return d.workout_done; });

    // Print the longest streak of completed workout days.
    std::cout << "Longest workout streak: " << streak << " days\n";

    // Call the same algorithm again with the predicate inverted, so it
    // finds the longest run of missed days instead of completed days.
    auto missed_streak = longest_streak_if(
        week, [](const DayLog& d) { return !d.workout_done; });

    // Print the longest run of missed days found across the full week.
    std::cout << "Longest missed streak: " << missed_streak << " days\n";
    
    return 0;
}
