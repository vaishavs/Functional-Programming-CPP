/* Custom data structure as a Boost.Range, with a custom adaptor and a custom algorithm */
#include <iostream>
#include <vector>
#include <string>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

// =========================
// 1. Data Model (Log AS RANGE)
// =========================
struct Log {
    std::string level;
    std::string message;

    // Make Log behave like a range over its message
    auto begin() { return message.begin(); }
    auto end()   { return message.end(); }

    auto begin() const { return message.begin(); }
    auto end()   const { return message.end(); }
};

// =========================
// 2. Adaptor
// =========================
struct only_errors_fn {
    template <typename Range>
    auto operator()(Range&& r) const {
        return std::forward<Range>(r)
            | boost::adaptors::filtered([](const Log& log) {
                  return log.level == "ERROR";
              });
    }
};

template <typename Range>
auto operator|(Range&& r, const only_errors_fn& fn) {
    return fn(std::forward<Range>(r));
}

constexpr only_errors_fn only_errors{};

// =========================
// 3. Algorithm
// =========================
template <typename Range, typename Predicate>
int count_if_range(const Range& r, Predicate pred) {
    int count = 0;
    for (auto it = boost::begin(r); it != boost::end(r); ++it) {
        if (pred(*it)) {
            ++count;
        }
    }
    return count;
}

// =========================
// 4. MAIN
// =========================
int main() {
    std::vector<Log> logs = {
        {"INFO", "System started"},
        {"ERROR", "Disk failure"},
        {"WARNING", "Low memory"},
        {"ERROR", "Crash detected"},
        {"INFO", "Recovered"}
    };

    // Pipeline still works
    auto processed =
        logs
        | only_errors
        | boost::adaptors::transformed([](const Log& log) {
              return log.message;
          });

    std::cout << "Error messages:\n";
    for (const auto& msg : processed) {
        std::cout << msg << std::endl;
    }

    // Demonstrating Log as a range
    std::cout << "\nCharacters of first ERROR log:\n";
    for (const auto& ch : *(boost::begin(logs | only_errors))) {
        std::cout << ch << ' ';
    }

    int count = count_if_range(
        logs | only_errors,
        [](const Log&) { return true; }
    );

    std::cout << "\n\nTotal ERROR logs: " << count << std::endl;

    return 0;
}
