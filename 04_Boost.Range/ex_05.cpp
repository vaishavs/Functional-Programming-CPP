#include <iostream>
#include <vector>
#include <string>
#include <iterator>

#include <boost/iterator/iterator_facade.hpp>
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
// 2. Iterator
// =========================
template <typename Iter>
class RedactIterator
    : public boost::iterator_facade<
          RedactIterator<Iter>,
          typename std::iterator_traits<Iter>::value_type,
          boost::forward_traversal_tag,
          typename std::iterator_traits<Iter>::reference,
          typename std::iterator_traits<Iter>::difference_type
      >
{
public:
    RedactIterator() = default;
    explicit RedactIterator(Iter it) : current(it) {}

private:
    friend class boost::iterator_core_access;

    void increment() { ++current; }

    bool equal(const RedactIterator& other) const {
        return current == other.current;
    }

    typename std::iterator_traits<Iter>::reference
    dereference() const {
        return *current;
    }

    Iter current;
};

// =========================
// 3. Range
// =========================
template <typename Container>
class RedactRange {
public:
    using iterator = RedactIterator<typename Container::iterator>;
    using const_iterator = RedactIterator<typename Container::const_iterator>;

    explicit RedactRange(Container& c) : container(c) {}

    iterator begin() { return iterator(container.begin()); }
    iterator end()   { return iterator(container.end()); }

    const_iterator begin() const { return const_iterator(container.begin()); }
    const_iterator end()   const { return const_iterator(container.end()); }

private:
    Container& container;
};

// =========================
// 4. Adaptor
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
// 5. Algorithm
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
// 6. MAIN
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

    int count = count_if_range(
        range | only_errors(),
        [](const Log&) { return true; }
    );

    std::cout << "\nTotal ERROR logs: " << count << std::endl;

    return 0;
}
