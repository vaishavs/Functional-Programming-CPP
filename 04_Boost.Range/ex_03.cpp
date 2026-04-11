/* Extending Boost.Range for an unmodifiable data structure */
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptors.hpp>
#include <iostream>

// ── Third-party struct we cannot modify ──────────────────────────────────
struct SensorArray {
    float readings[64];
    int   count;
};

// ── ADL hooks — MUST be in the same namespace as SensorArray ─────────────
inline float*       range_begin(      SensorArray& s) { return s.readings; }
inline float*       range_end  (      SensorArray& s) { return s.readings + s.count; }
inline const float* range_begin(const SensorArray& s) { return s.readings; }
inline const float* range_end  (const SensorArray& s) { return s.readings + s.count; }

// ── Register the iterator types with Boost ───────────────────────────────
namespace boost {
    template<> struct range_mutable_iterator<SensorArray> { typedef float* type; };
    template<> struct range_const_iterator  <SensorArray> { typedef const float* type; };
}

// ── Usage ─────────────────────────────────────────────────────────────────
int main() {
    SensorArray sa{};
    sa.count = 4;
    sa.readings[0] = 1.f; sa.readings[1] = 2.f;
    sa.readings[2] = 3.f; sa.readings[3] = 4.f;

    float total = boost::accumulate(sa, 0.f);
    std::cout << "Sum = " << total << '\n';  // Sum = 10

    // Adaptors work identically
    namespace ba = boost::adaptors;
    auto above_two = sa | ba::filtered([](float f){ return f > 2.f; });
    boost::for_each(above_two, [](float f){ std::cout << f << ' '; });
    // Output: 3 4
}
