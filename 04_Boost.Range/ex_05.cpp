#include <iostream>
#include <vector>
#include <cmath>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/concepts.hpp>

// ── Custom Boost.Range container: ring_buffer ───────────────────────────────────────────────────────
template <typename T>
class ring_buffer {
public:
    struct iterator
        : boost::iterator_facade<iterator, const T, boost::forward_traversal_tag>
    {
        iterator() = default;
        iterator(const ring_buffer* b, size_t i) : buf(b), idx(i) {}

    private:
        friend class boost::iterator_core_access;
        const T& dereference() const {
            return buf->data[(buf->head + idx) % buf->cap];
        }
        bool equal(const iterator& o) const { return idx == o.idx; }
        void increment() { ++idx; }

        const ring_buffer* buf{};
        size_t idx{};
    };

    explicit ring_buffer(size_t c) : data(c), cap(c) {}

    void push_back(T v) {
        data[(head + sz) % cap] = v;
        if (sz < cap) ++sz; else head = (head + 1) % cap;
    }

    iterator begin() const { return {this, 0}; }
    iterator end()   const { return {this, sz}; }

private:
    std::vector<T> data;
    size_t head = 0, sz = 0, cap;
};

// Boost traits
namespace boost {
    template <typename T> 
    struct range_iterator<ring_buffer<T>> {
        using type = typename ring_buffer<T>::iterator;
    };

    template <typename T> struct range_iterator<const ring_buffer<T>> {
        using type = typename ring_buffer<T>::iterator;
    };

    template <typename T> struct range_value<ring_buffer<T>> {
        using type = T;
    };

    template <typename T> struct range_value<const ring_buffer<T>> {
        using type = T;
    };
}

// ── Custom Boost.Range adaptor: clamped adaptor ───────────────────────────────────────────────────────
namespace adaptors {

template <typename It, typename T>
struct clamp_it : boost::iterator_adaptor<clamp_it<It,T>, It, T, boost::forward_traversal_tag, T>
{
    clamp_it() = default;
    clamp_it(It it, T lo, T hi) : clamp_it::iterator_adaptor_(it), lo(lo), hi(hi) {}

private:
    friend class boost::iterator_core_access;
    T dereference() const {
        T x = *this->base();
        return x < lo ? lo : (x > hi ? hi : x);
    }
    T lo{}, hi{};
};

template <typename T> struct holder { T lo, hi; };
template <typename T> holder<T> clamped(T lo, T hi) { return {lo, hi}; }

template <typename R, typename T>
auto operator|(const R& r, const holder<T>& h) {
    using It = decltype(boost::begin(r));
    return boost::make_iterator_range(
        clamp_it<It,T>(boost::begin(r), h.lo, h.hi),
        clamp_it<It,T>(boost::end(r),   h.lo, h.hi)
    );
}

}

// ── Custom Boost.Range adaptor: statistics ────────────────────────────────────────────────────────────
template <typename R>
void stats(const R& r) {
    BOOST_CONCEPT_ASSERT((boost::ForwardRangeConcept<const R>));

    auto it = boost::begin(r), end = boost::end(r);
    double min = *it, max = *it, sum = 0, sq = 0; size_t n = 0;

    for (; it != end; ++it) {
        double x = *it;
        if (x < min) min = x;
        if (x > max) max = x;
        sum += x; sq += x*x; ++n;
    }

    double mean = sum/n, std = std::sqrt(sq/n - mean*mean);

    std::cout << "count="<<n<<"\nmin="<<min<<"\nmax="<<max
              <<"\nmean="<<mean<<"\nstd="<<std<< std::endl;
}

// ── Demo ──────────────────────────────────────────────────────────────────────────────────────────
int main() {
    ring_buffer<double> rb(5);
    for (double v : {3, -2, 10, 25, 7, 8, -5}) rb.push_back(v);

    auto pipe = rb
        | adaptors::clamped(0.0, 15.0)
        | boost::adaptors::filtered([](double x){ return x > 0; });

    for (double x : pipe) std::cout << x << " ";
    std::cout << "\n";

    stats(pipe);
}
