/* Extending Boost Library Demo */

#include <algorithm>   // std::clamp, std::min, std::max
#include <cmath>       // std::sqrt
#include <cstddef>
#include <iostream>
#include <vector>

#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/iterator_range.hpp>

// ── ring_buffer, iterator via iterator_adaptor ────────────────────────────
//
// The thing being iterated is the logical index, so the base is
// counting_iterator<size_t>. Only dereference is overridden; traversal is
// inherited from the base, so the iterator is random-access.
template <typename T>
class ring_buffer {
public:
    class iterator : public boost::iterator_adaptor<
              iterator,
              boost::counting_iterator<std::size_t>,  // base = logical index
              const T>                                // Reference defaults to const T&
    {
    public:
        iterator() = default;
        iterator(const ring_buffer* b, std::size_t i)
            : iterator::iterator_adaptor_(boost::counting_iterator<std::size_t>(i)),
              buf(b) {}

    private:
        friend class boost::iterator_core_access;

        const T& dereference() const {
            return buf->data[(buf->head + *this->base()) % buf->cap];
        }

        const ring_buffer* buf = nullptr;
    };

    using const_iterator = iterator;

    explicit ring_buffer(std::size_t c) : data(c), cap(c) {}

    void push_back(T v) {
        data[(head + sz) % cap] = std::move(v);
        if (sz < cap) ++sz;                     // grow until full
        else          head = (head + 1) % cap;  // overwrite oldest
    }

    iterator begin() const { return {this, 0}; }
    iterator end()   const { return {this, sz}; }

private:
    std::vector<T> data;
    std::size_t    head = 0, sz = 0, cap;
};

// ── Custom adaptor: clamped (hand-rolled) ─────────────────────────────────
//
// The three classic pieces: an adapting iterator, a bounds holder, and
// operator| for pipe syntax. The iterator is the legitimate use of
// iterator_adaptor — a real base iterator supplies every traversal
// operation (inherited via use_default, nothing demoted to forward);
// only dereference is overridden. Reference is by-value T because the
// clamped value is computed on access, exactly as transform_iterator
// does it.
namespace adaptors {

template <typename It, typename T>
class clamp_iterator
    : public boost::iterator_adaptor<
          clamp_iterator<It, T>,
          It,                   // base: the adapted iterator
          T,                    // Value
          boost::use_default,   // traversal: inherited from It
          T>                    // Reference: by value (computed)
{
public:
    clamp_iterator() = default;
    clamp_iterator(It it, T lo, T hi)
        : clamp_iterator::iterator_adaptor_(it), lo(lo), hi(hi) {}

private:
    friend class boost::iterator_core_access;

    T dereference() const { return std::clamp<T>(*this->base(), lo, hi); }

    T lo{}, hi{};
};

// Bounds captured for deferred application through the pipe.
template <typename T>
struct clamp_holder { T lo, hi; };

template <typename T>
clamp_holder<T> clamped(T lo, T hi) { return {lo, hi}; }

// Found by ADL through clamp_holder; no collision with Boost's pipes.
template <typename Range, typename T>
auto operator|(const Range& r, const clamp_holder<T>& h) {
    using It = decltype(boost::begin(r));
    return boost::make_iterator_range(
        clamp_iterator<It, T>(boost::begin(r), h.lo, h.hi),
        clamp_iterator<It, T>(boost::end(r),   h.lo, h.hi));
}

} // namespace adaptors

// ── Custom algorithm: stats ───────────────────────────────────────────────
template <typename Range>
void stats(const Range& r)
{
    double      min = 0, max = 0, sum = 0, sq = 0;
    std::size_t n = 0;

    for (double x : r) {
        if (n == 0) min = max = x;
        min  = std::min(min, x);
        max  = std::max(max, x);
        sum += x;
        sq  += x * x;
        ++n;
    }

    const double mean = sum / n;
    const double sd   = std::sqrt(sq / n - mean * mean);

    std::cout << "count=" << n << "\nmin=" << min << "\nmax=" << max
              << "\nmean=" << mean << "\nstd=" << sd << '\n';
}

// ── Demo ──────────────────────────────────────────────────────────────────
int main() {
    ring_buffer<double> rb(5);

    for (double v : {3, -2, 10, 25, 7, 8, -5})
        rb.push_back(v);

    auto cl   = rb | adaptors::clamped(0.0, 15.0);
    auto pipe = cl | boost::adaptors::filtered([](double x) { return x > 0; });

    for (double x : pipe)
        std::cout << x << ' ';
    std::cout << '\n';

    stats(pipe);

    // Random access came free from the counting_iterator base.
    auto it = rb.begin();
    std::cout << "it[2]=" << it[2]
              << " *(it+4)=" << *(it + 4)
              << " end-begin=" << (rb.end() - rb.begin()) << '\n';

    // The custom adaptor inherits traversal instead of demoting it: the
    // clamped view of a random-access range is itself random-access.
    std::cout << "*(clamped.begin()+1)=" << *(boost::begin(cl) + 1) << '\n';
}
