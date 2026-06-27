// box_functor.cpp
//
// Build & run (needs C++20 for concepts):
//   g++ -std=c++20 -O2 box_functor.cpp -o box_functor && ./box_functor

#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <concepts>
#include <functional>
#include <type_traits>

// =====================================================================
// 1. Identity — exactly one value.            flavor: none (pure value)
// Styled after the lesson's identity functor.
// =====================================================================
template <typename T>
struct Box {
    T value;

    // transform: (T -> U)  on  Box<T>  yields  Box<U>
    template <typename F>
    auto transform(F&& f) const -> Box<std::invoke_result_t<F, const T&>> {
        return { std::forward<F>(f)(value) };   // new box, same shape, new contents
    }
    bool operator==(const Box&) const = default;
};

// =====================================================================
// 2. Maybe — one value OR nothing.            flavor: presence / absence
// A hand-written stand-in for optional. Assumes T is default-constructible.
// Preserved shape = whether the box is empty; f runs zero or one time.
// =====================================================================
template <typename T>
struct Maybe {
    bool full;
    T    value;   // only meaningful when full == true

    static Maybe<T> some(T v) { return { true, std::move(v) }; }
    static Maybe<T> none()    { return { false, T{} }; }

    template <typename F>
    auto transform(F&& f) const -> Maybe<std::invoke_result_t<F, const T&>> {
        using U = std::invoke_result_t<F, const T&>;
        if (full) return Maybe<U>::some(std::forward<F>(f)(value)); // transform value
        return Maybe<U>::none();                                    // empty stays empty
    }
    bool operator==(const Maybe&) const = default;
};

// =====================================================================
// 3. Many — a list of values.                 flavor: multiplicity
// Preserved shape = length and ordering; every element is rewritten,
// but the count and order never change.
// =====================================================================
template <typename T>
struct Many {
    std::vector<T> values;

    template <typename F>
    auto transform(F&& f) const -> Many<std::invoke_result_t<F, const T&>> {
        Many<std::invoke_result_t<F, const T&>> result;
        result.values.reserve(values.size());
        for (const auto& v : values)
            result.values.push_back(f(v));   // f re-applied per element
        return result;
    }
    bool operator==(const Many&) const = default;
};

// =====================================================================
// 4. Both — exactly two values of the same type, both mapped.
//                                             flavor: fixed-arity product
// Preserved shape = "two slots." Both contents are transformed.
// =====================================================================
template <typename T>
struct Both {
    T first, second;

    template <typename F>
    auto transform(F&& f) const -> Both<std::invoke_result_t<F, const T&>> {
        return { f(first), f(second) };   // both slots mapped
    }
    bool operator==(const Both&) const = default;
};

// =====================================================================
// 5. Tagged — a value carrying an untouched tag/context alongside it.
//                                             flavor: keyed context (Writer seed)
// Preserved shape = the tag. Only the value channel is mapped — this is the
// lesson's std::pair<A, T> functor, where the first slot rides along.
// =====================================================================
template <typename C, typename T>
struct Tagged {
    C tag;
    T value;

    template <typename F>
    auto transform(F&& f) const -> Tagged<C, std::invoke_result_t<F, const T&>> {
        return { tag, std::forward<F>(f)(value) };   // tag preserved, value mapped
    }
    bool operator==(const Tagged&) const = default;
};

// =====================================================================
// 6. Tree — an n-ary tree of values.          flavor: recursive structure
// Preserved shape = the tree topology. Every node value is rewritten, but
// the branching never changes. Shows functors are not only linear containers.
// =====================================================================
template <typename T>
struct Tree {
    T value;
    std::vector<Tree<T>> children;

    template <typename F>
    auto transform(F&& f) const -> Tree<std::invoke_result_t<F, const T&>> {
        using U = std::invoke_result_t<F, const T&>;
        Tree<U> out{ f(value), {} };
        out.children.reserve(children.size());
        for (const auto& c : children)
            out.children.push_back(c.transform(f));   // recurse, same shape
        return out;
    }
    bool operator==(const Tree&) const = default;
};

// =====================================================================
// 7. Lazy — a value computed only when forced. flavor: deferral / effect
// A self-contained cousin of std::future / lazy views: mapping schedules f
// to run later, the moment you force the box. Preserved shape = "deferred."
// =====================================================================
template <typename T>
struct Lazy {
    std::function<T()> thunk;
    T force() const { return thunk(); }

    template <typename F>
    auto transform(F f) const -> Lazy<std::invoke_result_t<F, const T&>> {
        using U = std::invoke_result_t<F, const T&>;
        auto inner = thunk;                                   // capture current thunk
        return Lazy<U>{ [inner, f]{ return f(inner()); } };   // runs only when forced
    }
};

// =====================================================================
// 8. Reader — a value computed on demand from an input R.
//                                             flavor: dependency on an input
// The "box" is itself a computation R -> T. Mapping composes f after it,
// yielding R -> U. Preserved shape = "still needs an R." (lesson's function box)
// =====================================================================
template <typename R, typename T>
struct Reader {
    std::function<T(R)> run;
    T operator()(const R& r) const { return run(r); }

    template <typename F>
    auto transform(F f) const -> Reader<R, std::invoke_result_t<F, const T&>> {
        using U = std::invoke_result_t<F, const T&>;
        auto h = run;                                              // current R -> T
        return Reader<R, U>{ [h, f](const R& r){ return f(h(r)); } };  // f after h
    }
};

// =====================================================================
// The box-functor pattern is ad hoc and per-type: there is no single
// interface every box implements. A C++20 concept lets us *describe* the
// shared surface and write ONE helper over anything with a member transform.
// =====================================================================
template <typename Fa, typename Func>
concept Transformable = requires(Fa fa, Func f) {
    { fa.transform(f) };   // member transform that compiles
};

template <typename Func, typename Fa>
    requires Transformable<Fa, Func>
auto map_over(Fa&& fa, Func&& f) {
    return std::forward<Fa>(fa).transform(std::forward<Func>(f));
}

// ---------------- tiny printers, just for the demo ----------------
template <typename T>
void print(const Box<T>& b) { std::cout << "Box(" << b.value << ")\n"; }

template <typename T>
void print(const Maybe<T>& b) {
    if (b.full) std::cout << "Maybe(" << b.value << ")\n";
    else        std::cout << "Maybe(empty)\n";
}

template <typename T>
void print(const Many<T>& b) {
    std::cout << "Many[";
    for (std::size_t i = 0; i < b.values.size(); ++i)
        std::cout << b.values[i] << (i + 1 < b.values.size() ? ", " : "");
    std::cout << "]\n";
}

template <typename T>
void print(const Both<T>& b) { std::cout << "Both(" << b.first << ", " << b.second << ")\n"; }

template <typename C, typename T>
void print(const Tagged<C, T>& t) { std::cout << "Tagged{" << t.tag << " = " << t.value << "}\n"; }

template <typename T>
void print_tree(const Tree<T>& t) {
    std::cout << t.value;
    if (!t.children.empty()) {
        std::cout << "(";
        for (std::size_t i = 0; i < t.children.size(); ++i) {
            print_tree(t.children[i]);
            if (i + 1 < t.children.size()) std::cout << " ";
        }
        std::cout << ")";
    }
}
template <typename T>
void print(const Tree<T>& t) { std::cout << "Tree "; print_tree(t); std::cout << "\n"; }

template <typename T>
void print(const Lazy<T>& l) { std::cout << "Lazy(forced -> " << l.force() << ")\n"; }

int main() {
    // One function, reused across every kind of box.
    auto times_ten = [](int x) { return x * 10; };

    Box<int>                 a { 5 };
    Maybe<int>               b = Maybe<int>::some(7);
    Maybe<int>               c = Maybe<int>::none();
    Many<int>                d { {1, 2, 3} };
    Both<int>                e { 4, 9 };
    Tagged<std::string, int> g { "celsius", 100 };
    Tree<int>                t { 1, { Tree<int>{2, { Tree<int>{4, {}} }}, Tree<int>{3, {}} } };
    Lazy<int>                z { []{ return 21; } };
    Reader<std::string, int> len { [](const std::string& s){ return (int)s.size(); } };

    std::cout << "-- one map_over + one function, every box keeps its shape --\n";
    print(map_over(a, times_ten));   // Box(50)
    print(map_over(b, times_ten));   // Maybe(70)
    print(map_over(c, times_ten));   // Maybe(empty)        <- emptiness preserved
    print(map_over(d, times_ten));   // Many[10, 20, 30]
    print(map_over(e, times_ten));   // Both(40, 90)
    print(map_over(g, times_ten));   // Tagged{celsius = 1000}   <- tag untouched
    print(map_over(t, times_ten));   // Tree 10(20(40) 30)       <- topology preserved
    print(map_over(z, times_ten));   // Lazy(forced -> 210)      <- runs at force time
    std::cout << "Reader(\"hello\" -> " << map_over(len, times_ten)("hello") << ")\n"; // 50

    // transform may also CHANGE the inner type (int -> string),
    // while the box kind / structure stays exactly the same.
    auto to_label = [](int x) { return std::string("#") + std::to_string(x); };

    std::cout << "\n-- contents change type, box kind does not --\n";
    print(map_over(e, to_label));    // Both(#4, #9)
    print(map_over(g, to_label));    // Tagged{celsius = #100}
    print(map_over(t, to_label));    // Tree #1(#2(#4) #3)

    // ---------------- the two functor laws ----------------
    auto id        = [](auto x){ return x; };
    auto add_one   = [](int x){ return x + 1; };
    auto times_two = [](int x){ return x * 2; };
    auto fused     = [&](int x){ return times_two(add_one(x)); };  // times_two ∘ add_one

    std::cout << "\n-- Law 1, identity: transform(id) == id --\n"
              << std::boolalpha
              << "  Box    : " << (map_over(a, id) == a) << "\n"
              << "  Maybe  : " << (map_over(b, id) == b) << "\n"
              << "  Many   : " << (map_over(d, id) == d) << "\n"
              << "  Both   : " << (map_over(e, id) == e) << "\n"
              << "  Tagged : " << (map_over(g, id) == g) << "\n"
              << "  Tree   : " << (map_over(t, id) == t) << "\n";

    std::cout << "\n-- Law 2, composition: transform(f) then transform(g) == transform(g.f) --\n"
              << "  Box    : " << (map_over(map_over(a, add_one), times_two) == map_over(a, fused)) << "\n"
              << "  Maybe  : " << (map_over(map_over(b, add_one), times_two) == map_over(b, fused)) << "\n"
              << "  Many   : " << (map_over(map_over(d, add_one), times_two) == map_over(d, fused)) << "\n"
              << "  Both   : " << (map_over(map_over(e, add_one), times_two) == map_over(e, fused)) << "\n"
              << "  Tagged : " << (map_over(map_over(g, add_one), times_two) == map_over(g, fused)) << "\n"
              << "  Tree   : " << (map_over(map_over(t, add_one), times_two) == map_over(t, fused)) << "\n";

    // Lazy and Reader wrap callables, so they aren't equality-comparable; we
    // observe the laws instead by forcing / applying both sides.
    std::cout << "\n-- same laws, observed for the function-style boxes --\n"
              << "  Lazy   : "
              << (map_over(map_over(z, add_one), times_two).force() == map_over(z, fused).force()) << "\n"
              << "  Reader : "
              << (map_over(map_over(len, add_one), times_two)("hello") == map_over(len, fused)("hello")) << "\n";

    return 0;
}
