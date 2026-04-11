#include <iostream>
#include <vector>
#include <string>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

// =========================
// 1. Data Model
// =========================
struct Log {
    std::string level;
    std::string message;
};

// =========================
// 2. Adaptor
// =========================
struct only_errors_holder {};

template <typename Range>
auto operator|(Range&& r, only_errors_holder) {
    return r | boost::adaptors::filtered([](const Log& log) {
        return log.level == "ERROR";
    });
}

only_errors_holder only_errors() {
    return {};
}

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

    // Directly use vector (already random access)
    auto processed =
        logs
        | only_errors()
        | boost::adaptors::transformed([](const Log& log) {
              return log.message;
          });

    std::cout << "Error messages:\n";
    for (const auto& msg : processed) {
        std::cout << msg << std::endl;
    }

    int count = count_if_range(
        logs | only_errors(),
        [](const Log&) { return true; }
    );

    std::cout << "\nTotal ERROR logs: " << count << std::endl;

    return 0;
}
