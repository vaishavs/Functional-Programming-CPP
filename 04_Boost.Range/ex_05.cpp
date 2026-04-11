#include <iostream>
#include <vector>
#include <string>
#include <iterator>

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
// 2. Custom Iterator
// (wraps underlying iterator)
// =========================
#include <iterator>

template <typename Iter>
class RedactIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename std::iterator_traits<Iter>::value_type;
    using difference_type = typename std::iterator_traits<Iter>::difference_type;

    using reference = typename std::iterator_traits<Iter>::reference;
    using pointer   = typename std::iterator_traits<Iter>::pointer;

    RedactIterator() = default;
    explicit RedactIterator(Iter it) : current(it) {}

    reference operator*() const {
        return *current;
    }

    pointer operator->() const {
        return current.operator->();
    }

    RedactIterator& operator++() {
        ++current;
        return *this;
    }

    RedactIterator operator++(int) {
        RedactIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool operator==(const RedactIterator& other) const {
        return current == other.current;
    }

    bool operator!=(const RedactIterator& other) const {
        return !(*this == other);
    }

private:
    Iter current;
};

// =========================
// 3. Custom Range
// =========================
template <typename Container>
class RedactRange {
public:
    using iterator = RedactIterator<typename Container::iterator>;
    using const_iterator = RedactIterator<typename Container::const_iterator>;

    explicit RedactRange(Container& c) : container(c) {}

    iterator begin() {
        return iterator(container.begin());
    }

    iterator end() {
        return iterator(container.end());
    }

    const_iterator begin() const {
        return const_iterator(container.begin());
    }

    const_iterator end() const {
        return const_iterator(container.end());
    }

private:
    Container& container;
};

// =========================
// 4. Custom Adaptor
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
// 5. Custom Algorithm
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
// 6. MAIN PIPELINE
// =========================
int main() {
    std::vector<Log> logs = {
        {"INFO", "System started"},
        {"ERROR", "Disk failure"},
        {"WARNING", "Low memory"},
        {"ERROR", "Crash detected"},
        {"INFO", "Recovered"}
    };

    RedactRange<std::vector<Log>> range(logs);

    // Pipeline:
    // custom range → custom adaptor → boost transform
    auto processed =
        range
        | only_errors()
        | boost::adaptors::transformed([](const Log& log) {
              return log.message;
          });

    std::cout << "Error messages:\n";
    for (const auto& msg : processed) {
        std::cout << msg << std::endl;
    }

    // Custom algorithm on pipeline
    int count = count_if_range(
        range | only_errors(),
        [](const Log&) { return true; }
    );

    std::cout << "\nTotal ERROR logs: " << count << std::endl;

    return 0;
}
