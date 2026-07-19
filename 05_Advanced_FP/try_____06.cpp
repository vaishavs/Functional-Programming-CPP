#include <iostream>
#include <string>
#include <type_traits>

template <typename T>
struct Box {
    T value;
};

// ==========================================
// 1. FUNCTOR: fmap
// Concept: Applies a normal function (A -> B) to a wrapped value (Box<A>),
// returning a newly wrapped value (Box<B>).
// ==========================================
template <typename F, typename T>
auto fmap(F f, Box<T> box) {
    // HINT 1: Look at the return type of `f`. If `f` is `[](int i){ return std::to_string(i); }`, 
    // `f` returns a raw `std::string`. 
    // HINT 2: A Functor must preserve the context (the Box). If we just return `f(...)`, 
    // the value "escapes" the Box. How do we put it back?
    
    // BUG 1 HERE
    return f(box.value);
}

// ==========================================
// 2. APPLICATIVE: pure (or "return")
// Concept: Lifts a raw value (A) into the context (Box<A>).
// (This one is implemented correctly and will be useful for fixing the others!)
// ==========================================
template <typename T>
Box<T> pure(T v) {
    return Box<T>{v};
}

// ==========================================
// 3. APPLICATIVE: apply
// Concept: Applies a wrapped function (Box<A -> B>) to a wrapped value (Box<A>),
// returning a wrapped result (Box<B>).
// ==========================================
template <typename F, typename T>
auto apply(Box<F> box_f, Box<T> box_v) {
    // HINT 1: `box_f` is a `Box`, which is a struct. The compiler will complain:
    // "type 'Box<(lambda)>' does not provide a call operator". 
    // HINT 2: You cannot invoke a Box. Where is the actual function stored inside `box_f`?
    
    // BUG 2 HERE
    return pure(box_f(box_v.value));
}

// ==========================================
// 4. MONAD: bind (flatMap / and_then)
// Concept: Chains operations where the function itself returns a wrapped value.
// It takes a Box<A> and a function (A -> Box<B>), and returns a Box<B>.
// ==========================================
template <typename F, typename T>
auto bind(F f, Box<T> box) {
    // HINT 1: Look closely at the signature of a Monadic function. 
    // `f` already returns a `Box<B>`.
    // HINT 2: What happens if you take a `Box<B>` and pass it into `pure()`? 
    // You get the dreaded anti-pattern: `Box<Box<B>>`. Monads are designed to flatten!
    
    // BUG 3 HERE
    return pure(f(box.value));
}

int main() {
    Box<int> x{5};

    // --- FUNCTOR TEST ---
    auto to_string = [](int i) { return std::to_string(i); };
    auto b1 = fmap(to_string, x);
    static_assert(std::is_same_v<decltype(b1), Box<std::string>>, 
                  "fmap should return a Box<std::string>");

    // --- APPLICATIVE TEST ---
    auto add_two = [](int i) { return i + 2; };
    Box<decltype(add_two)> box_func{add_two};
    auto b2 = apply(box_func, x);
    static_assert(std::is_same_v<decltype(b2), Box<int>>, 
                  "apply should return a Box<int>");

    // --- MONAD TEST ---
    auto make_box = [](int i) { return pure(i * 10); };
    auto b3 = bind(make_box, x);
    static_assert(std::is_same_v<decltype(b3), Box<int>>, 
                  "bind should return a Box<int>, not Box<Box<int>>!");

    std::cout << "All assertions passed! Box model is mathematically sound.\n";
    return 0;
}
