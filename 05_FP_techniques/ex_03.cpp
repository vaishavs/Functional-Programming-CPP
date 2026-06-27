// box_functor.cpp
//
// Build & run (needs C++20 for concepts):
//   g++ -std=c++20 -O2 box_functor.cpp -o box_functor && ./box_functor
//
// Eight "boxes," one shared idea: each has a member `transform` that maps the
// contents (T -> U) while leaving the box's *shape* untouched. A single free
// helper `map_over` works over all of them via one concept, and a single
// `operator<<` per box keeps the demo uniform (and lets boxes nest/print).

#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <concepts>
#include <functional>
#include <type_traits>

// The inner type produced by applying F to a (const) T. Used by every
// transform below, so the long std::invoke_result_t spelling appears once.
template <typename F, typename T>
using mapped_t = std::invoke_result_t<F, const T&>;

// =====================================================================
// 1. Identity — exactly one value.            flavor: none (pure value)
// =====================================================================
template <typename T>
struct Box {
    T value;

    template <typename F>
    auto transform(F f) const -> Box<mapped_t<F, T>> {
        return { f(value) };                 // new box, same shape, new contents
    }
    bool operator==(const Box&) const = default;
};

// =====================================================================
// 2. Maybe — one value OR nothing.            flavor: presence / absence
// Shape preserved = whether the box is empty; f runs zero or one time.
// =====================================================================
template <typename T>
struct Maybe {
    bool full;
    T    value;                              // only meaningful when full == true

    static Maybe some(T v) { return { true, std::move(v) }; }
    static Maybe none()    { return { false, T{} }; }

    template <typename F>
    auto transform(F f) const -> Maybe<mapped_t<F, T>> {
        using U = mapped_t<F, T>;
        if (full) return Maybe<U>::some(f(value));   // transform the value
        return Maybe<U>::none();                     // empty stays empty
    }
    bool operator==(const Maybe&) const = default;
};

// =====================================================================
// 3. Many — a list of values.                 flavor: multiplicity
// Shape preserved = length and order; every element is rewritten.
// =====================================================================
template <typename T>
struct Many {
    std::vector<T> values;

    template <typename F>
    auto transform(F f) const -> Many<mapped_t<F, T>> {
        Many<mapped_t<F, T>> result;
        result.values.reserve(values.size());
        for (const auto& v : values)
            result.values.push_back(f(v));   // f re-applied per element
        return result;
    }
    bool operator==(const Many&) const = default;
};

// =====================================================================
// 4. Both — two values of the same type.      flavor: fixed-arity product
// Shape preserved = "two slots"; both contents are transformed.
// =====================================================================
template <typename T>
struct Both {
    T first, second;

    template <typename F>
    auto transform(F f) const -> Both<mapped_t<F, T>> {
        return { f(first), f(second) };      // both slots mapped
    }
    bool operator==(const Both&) const = default;
};

// =====================================================================
// 5. Tagged — a value carrying an untouched tag alongside it.
//                                             flavor: keyed context (Writer seed)
// Shape preserved = the tag; only the value channel is mapped.
// =====================================================================
template <typename C, typename T>
struct Tagged {
    C tag;
    T value;

    template <typename F>
    auto transform(F f) const -> Tagged<C, mapped_t<F, T>> {
        return { tag, f(value) };            // tag preserved, value mapped
    }
    bool operator==(const Tagged&) const = default;
};

// =====================================================================
// 6. Tree — an n-ary tree of values.          flavor: recursive structure
// Shape preserved = the topology; every node value is rewritten.
// =====================================================================
template <typename T>
struct Tree {
    T value;
    std::vector<Tree<T>> children;

    template <typename F>
    auto transform(F f) const -> Tree<mapped_t<F, T>> {
        Tree<mapped_t<F, T>> out{ f(value), {} };
        out.children.reserve(children.size());
        for (const auto& c : children)
            out.children.push_back(c.transform(f));   // recurse, same shape
        return out;
    }
    bool operator==(const Tree&) const = default;
};

// =====================================================================
// 7. Lazy — a value computed only when forced. flavor: deferral / effect
// Mapping schedules f to run later, at the moment you force the box.
// =====================================================================
template <typename T>
struct Lazy {
    std::function<T()> thunk;
    T force() const { return thunk(); }

    template <typename F>
    auto transform(F f) const -> Lazy<mapped_t<F, T>> {
        auto inner = thunk;                           // capture current thunk
        return { [inner, f] { return f(inner()); } }; // runs only when forced
    }
};

// =====================================================================
// 8. Reader — a value computed on demand from an input R.
//                                             flavor: dependency on an input
// The box is itself R -> T; mapping composes f after it, giving R -> U.
// =====================================================================
template <typename R, typename T>
struct Reader {
    std::function<T(R)> run;
    T operator()(const R& r) const { return run(r); }

    template <typename F>
    auto transform(F f) const -> Reader<R, mapped_t<F, T>> {
        auto h = run;                                       // current R -> T
        return { [h, f](const R& r) { return f(h(r)); } };  // f after h
    }
};

// =====================================================================
// The pattern is per-type: there is no shared base every box derives from.
// A C++20 concept *describes* the common surface so one helper covers
// anything with a member transform that compiles.
// =====================================================================
template <typename Fa, typename Func>
concept Transformable = requires(Fa fa, Func f) {
    { fa.transform(f) };
};

template <typename Fa, typename Func>
    requires Transformable<Fa, Func>
auto map_over(Fa&& fa, Func&& f) {
    return std::forward<Fa>(fa).transform(std::forward<Func>(f));
}

// ---------- one operator<< per box, so every box prints the same way ----------
template <typename T>
std::ostream& operator<<(std::ostream& os, const Box<T>& b) {
    return os << "Box(" << b.value << ")";
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const Maybe<T>& m) {
    return m.full ? os << "Maybe(" << m.value << ")" : os << "Maybe(empty)";
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const Many<T>& m) {
    os << "Many[";
    for (std::size_t i = 0; i < m.values.size(); ++i)
        os << m.values[i] << (i + 1 < m.values.size() ? ", " : "");
    return os << "]";
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const Both<T>& b) {
    return os << "Both(" << b.first << ", " << b.second << ")";
}
template <typename C, typename T>
std::ostream& operator<<(std::ostream& os, const Tagged<C, T>& t) {
    return os << "Tagged{" << t.tag << " = " << t.value << "}";
}
template <typename T>
void stream_tree(std::ostream& os, const Tree<T>& t) {   // bare topology, recursive
    os << t.value;
    if (!t.children.empty()) {
        os << "(";
        for (std::size_t i = 0; i < t.children.size(); ++i) {
            stream_tree(os, t.children[i]);
            if (i + 1 < t.children.size()) os << " ";
        }
        os << ")";
    }
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const Tree<T>& t) {
    os << "Tree ";
    stream_tree(os, t);
    return os;
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const Lazy<T>& l) {
    return os << "Lazy(forced -> " << l.force() << ")";
}

// One printer for anything streamable (every box above qualifies, and they nest).
template <typename T>
void print(const T& x) { std::cout << x << '\n'; }

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
    Lazy<int>                z { [] { return 21; } };
    Reader<std::string, int> len { [](const std::string& s) { return (int)s.size(); } };

    std::cout << "-- one map_over + one function, every box keeps its shape --\n";
    print(map_over(a, times_ten));   // Box(50)
    print(map_over(b, times_ten));   // Maybe(70)
    print(map_over(c, times_ten));   // Maybe(empty)          <- emptiness preserved
    print(map_over(d, times_ten));   // Many[10, 20, 30]
    print(map_over(e, times_ten));   // Both(40, 90)
    print(map_over(g, times_ten));   // Tagged{celsius = 1000}     <- tag untouched
    print(map_over(t, times_ten));   // Tree 10(20(40) 30)         <- topology preserved
    print(map_over(z, times_ten));   // Lazy(forced -> 210)        <- runs at force time
    std::cout << "Reader(\"hello\" -> " << map_over(len, times_ten)("hello") << ")\n"; // 50

    // transform may also CHANGE the inner type (int -> string),
    // while the box kind / structure stays exactly the same.
    auto to_label = [](int x) { return std::string("#") + std::to_string(x); };

    std::cout << "\n-- contents change type, box kind does not --\n";
    print(map_over(e, to_label));    // Both(#4, #9)
    print(map_over(g, to_label));    // Tagged{celsius = #100}
    print(map_over(t, to_label));    // Tree #1(#2(#4) #3)

    // ---------------- the two functor laws ----------------
    auto id        = [](auto x) { return x; };
    auto add_one   = [](int x) { return x + 1; };
    auto times_two = [](int x) { return x * 2; };
    auto fused     = [&](int x) { return times_two(add_one(x)); };  // times_two ∘ add_one

    std::cout << std::boolalpha;

    // Law 1: transform(id) leaves the box unchanged.
    auto identity_law = [&](const char* name, const auto& box) {
        std::cout << "  " << name << " : " << (map_over(box, id) == box) << "\n";
    };
    std::cout << "\n-- Law 1, identity: transform(id) == id --\n";
    identity_law("Box   ", a);
    identity_law("Maybe ", b);
    identity_law("Many  ", d);
    identity_law("Both  ", e);
    identity_law("Tagged", g);
    identity_law("Tree  ", t);

    // Law 2: mapping f then g equals mapping (g ∘ f) in a single pass.
    auto compose_law = [&](const char* name, const auto& box) {
        std::cout << "  " << name << " : "
                  << (map_over(map_over(box, add_one), times_two) == map_over(box, fused)) << "\n";
    };
    std::cout << "\n-- Law 2, composition: transform(f) then transform(g) == transform(g.f) --\n";
    compose_law("Box   ", a);
    compose_law("Maybe ", b);
    compose_law("Many  ", d);
    compose_law("Both  ", e);
    compose_law("Tagged", g);
    compose_law("Tree  ", t);

    // Lazy and Reader wrap callables, so they aren't equality-comparable;
    // observe the laws by forcing / applying both sides instead.
    std::cout << "\n-- same laws, observed for the function-style boxes --\n"
              << "  Lazy   : "
              << (map_over(map_over(z, add_one), times_two).force() == map_over(z, fused).force()) << "\n"
              << "  Reader : "
              << (map_over(map_over(len, add_one), times_two)("hello") == map_over(len, fused)("hello")) << "\n";

    return 0;
}
