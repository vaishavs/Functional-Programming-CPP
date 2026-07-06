// Exercise 3: Applicative — combining two Many<T> boxes (a Vector, not a Maybe)
//
// GOAL:
// This exercise uses a completely different shape of box than a
// present/absent Maybe: Many<T> holds a *list* of possible values (built
// on std::vector), including possibly none, possibly several. There is
// nothing to check for "is it there" — the applicative combine here is
// the classic list-applicative operation: the CARTESIAN PRODUCT.
//
// Given Many<T> holding values [x1, x2, ...] and Many<U> holding values
// [y1, y2, ...], combining them with a function f produces every possible
// f(xi, yj) pairing, collected into one new Many<V>.
//
// TASK:
// Complete apply2() so it computes the cartesian product: for every value
// in `a`, and for every value in `b`, compute f(x, y) and collect all of
// the results into a single Many<V>.

#include <iostream>
#include <string>
#include <vector>

// Many<T>: holds zero or more possible values.
template <typename T>
struct Many {
    std::vector<T> values;

    static Many<T> from(std::vector<T> v) { return Many<T>{std::move(v)}; }
};

template <typename F, typename T, typename U>
auto apply2(F f, const Many<T>& a, const Many<U>& b)
    -> Many<decltype(f(a.values[0], b.values[0]))>
{
    // TODO: implement this function.
    //
    // HINT: use two nested range-based for loops — outer loop over
    //       `a.values`, inner loop over `b.values`.
    // HINT: for each pair (x, y), push_back(f(x, y)) onto a local
    //       std::vector, then wrap the finished vector with
    //       Many<ResultType>::from(...).
    // HINT: `decltype(f(a.values[0], b.values[0]))` never actually
    //       evaluates `a.values[0]` — decltype only inspects the type,
    //       so this is safe even though it looks like it might read past
    //       the end of an empty vector.

    using ResultType = decltype(f(a.values[0], b.values[0]));
    return Many<ResultType>::from({}); // <-- placeholder (always empty), replace this
}

int main() {
    Many<int> xs = Many<int>::from({1, 2});
    Many<int> ys = Many<int>::from({10, 20});

    auto sums = apply2([](int x, int y) { return x + y; }, xs, ys);

    // Expected output:
    // 11 21 12 22

    for (int v : sums.values) {
        std::cout << v << " ";
    }
    std::cout << "\n";
}
