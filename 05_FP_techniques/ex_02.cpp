#include <iostream>

// A minimal container that always holds exactly one value.
template <typename T>
class Box {
    T value_;
public:
    explicit Box(T value) : value_(value) {}
    const T& get() const { return value_; }

    // FUNCTOR: run a plain function over the value.
    template <typename F>
    auto map(F f) const -> Box<decltype(f(value_))> {
        return Box<decltype(f(value_))>(f(value_));
    }

    // APPLICATIVE: this box holds a function; apply it to another box's value.
    // (Only compiles when this box's contents are actually callable.)
    template <typename U>
    auto ap(const Box<U>& other) const -> Box<decltype(value_(other.get()))> {
        return Box<decltype(value_(other.get()))>(value_(other.get()));
    }

    // MONAD: run a function that returns a new Box, then flatten.
    template <typename F>
    auto bind(F f) const -> decltype(f(value_)) {
        return f(value_);
    }
};

int main() {
    Box<int> x(5);

    // Functor: function goes in, box comes out
    std::cout << "map:  " << x.map([](int n){ return n * 2; }).get() << "\n";   // 10

    // Applicative: the function itself lives in a box
    Box<int(*)(int)> f([](int n){ return n + 100; });
    std::cout << "ap:   " << f.ap(x).get() << "\n";                             // 105

    // Monad: the function returns a box, bind flattens it
    std::cout << "bind: " << x.bind([](int n){ return Box<int>(n * n); }).get() << "\n"; // 25

    return 0;
}
