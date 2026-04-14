#include <iostream>
#include <vector>
#include <cmath>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/concepts.hpp>

// ── Custom Boost.Range container: ring_buffer ─────────────────────────────
template <typename T>
class ring_buffer {
    // Base iterator from underlying storage
    using base_iter = typename std::vector<T>::const_iterator;

public:
    // ── Custom iterator wrapping base_iter ─────────────────────
    struct iterator
        : boost::iterator_adaptor<
              iterator,                     // CRTP: this iterator
              base_iter,                   // underlying iterator type
              const T,                     // value type
              boost::forward_traversal_tag // traversal category
          >
    {
        iterator() = default;

        // Construct with:
        // - base iterator (not really used for position logic)
        // - pointer to buffer
        // - logical index (0..size)
        iterator(base_iter it, const ring_buffer* b, size_t i)
            : iterator::iterator_adaptor_(it), buf(b), idx(i) {}

    private:
        friend class boost::iterator_core_access;

        // ── Core logic: compute circular index ─────────────────
        const T& dereference() const {
            // Map logical index → actual position in circular buffer
            size_t pos = (buf->head + idx) % buf->cap;
            return buf->data[pos];
        }

        // Move forward (logical index only)
        void increment() { ++idx; }

        // Equality based on logical position
        bool equal(const iterator& o) const { return idx == o.idx; }

        const ring_buffer* buf{}; // pointer to owning buffer
        size_t idx{};             // logical position
    };

    // ── Constructor ───────────────────────────────────────────
    explicit ring_buffer(size_t c) : data(c), cap(c) {}

    // ── Insert element (circular overwrite) ───────────────────
    void push_back(T v) {
        data[(head + sz) % cap] = v;
        if (sz < cap)
            ++sz;                 // grow until full
        else
            head = (head + 1) % cap; // overwrite oldest
    }

    // ── Range interface ───────────────────────────────────────
    iterator begin() const { return {data.begin(), this, 0}; }
    iterator end()   const { return {data.begin(), this, sz}; }

private:
    std::vector<T> data; // storage
    size_t head = 0;     // start of logical range
    size_t sz   = 0;     // current size
    size_t cap;          // capacity
};

// Boost.Range traits (REQUIRED)
// Tell Boost how to treat ring_buffer as a range
namespace boost {
    template <typename T>
    struct range_iterator<ring_buffer<T>> {
        using type = typename ring_buffer<T>::iterator;
    };
    
    template <typename T>
    struct range_iterator<const ring_buffer<T>> {
        using type = typename ring_buffer<T>::iterator;
    };
    
    template <typename T>
    struct range_value<ring_buffer<T>> {
        using type = T;
    };
    
    template <typename T>
    struct range_value<const ring_buffer<T>> {
        using type = T;
    };
}

// ── Custom Boost.Range adaptor: clamped ───────────────────────────────────
namespace adaptors {
    // Iterator that wraps another iterator and clamps values
    template <typename It, typename T>
    struct clamp_it
        : boost::iterator_adaptor<
              clamp_it<It,T>, It, T,
              boost::forward_traversal_tag, T>
    {
        clamp_it() = default;
    
        // Store bounds (lo, hi)
        clamp_it(It it, T lo, T hi)
            : clamp_it::iterator_adaptor_(it), lo(lo), hi(hi) {}
    
    private:
        friend class boost::iterator_core_access;
    
        // Clamp each value on access
        T dereference() const {
            T x = *this->base(); // read from underlying iterator
            return x < lo ? lo : (x > hi ? hi : x);
        }
    
        T lo{}, hi{};
    };
    
    // Holder for pipe syntax
    template <typename T>
    struct holder { T lo, hi; };
    
    // Factory function
    template <typename T>
    holder<T> clamped(T lo, T hi) { return {lo, hi}; }
    
    // Pipe operator: creates a transformed range
    template <typename R, typename T>
    auto operator|(const R& r, const holder<T>& h) {
        using It = decltype(boost::begin(r));
    
        return boost::make_iterator_range(
            clamp_it<It,T>(boost::begin(r), h.lo, h.hi),
            clamp_it<It,T>(boost::end(r),   h.lo, h.hi)
        );
    }
}

// ── Custom Boost.Range algorithm: statistics ───────────────────────────────
template <typename R>
void stats(const R& r)
{
    // Ensure it's a valid forward range
    BOOST_CONCEPT_ASSERT((boost::ForwardRangeConcept<const R>));

    auto it = boost::begin(r), end = boost::end(r);

    double min = *it, max = *it, sum = 0, sq = 0;
    size_t n = 0;

    for (; it != end; ++it) {
        double x = *it;

        if (x < min) min = x;
        if (x > max) max = x;

        sum += x;
        sq  += x * x;
        ++n;
    }

    double mean = sum / n;
    double std  = std::sqrt(sq / n - mean * mean);

    std::cout << "count=" << n << "\nmin=" << min << "\nmax=" << max
              << "\nmean=" << mean << "\nstd=" << std << std::endl;
}

// ── Demo ─────────────────────────────────────────────────────────────────
int main() {
    ring_buffer<double> rb(5);

    // Insert values (circular overwrite happens)
    for (double v : {3, -2, 10, 25, 7, 8, -5})
        rb.push_back(v);

    // Pipeline:
    // 1. clamp values to [0, 15]
    // 2. keep only positive values
    auto pipe = rb
        | adaptors::clamped(0.0, 15.0)
        | boost::adaptors::filtered([](double x){ return x > 0; });

    // Print pipeline output
    for (double x : pipe)
        std::cout << x << " ";
    std::cout << "\n";

    // Compute statistics on pipeline
    stats(pipe);
}
