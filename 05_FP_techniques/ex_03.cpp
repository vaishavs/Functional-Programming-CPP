// Build & run (needs C++20 for concepts):
//   g++ -std=c++20 -O2 box_functor.cpp -o box_functor && ./box_functor
//
// Eight "boxes," one shared idea: each has a member `transform` that maps the contents (T -> U) while
// leaving the box's *shape* untouched. A free helper `traced` maps + prints over all of them via one concept.

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <concepts>
#include <functional>

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

    friend std::ostream& operator<<(std::ostream& os, const Box& b) {
        return os << "Box(" << b.value << ")";
    }
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

    friend std::ostream& operator<<(std::ostream& os, const Maybe& m) {
        return m.full ? os << "Maybe(" << m.value << ")" : os << "Maybe(empty)";
    }
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

    friend std::ostream& operator<<(std::ostream& os, const Many& m) {
        os << "Many[";
        for (std::size_t i = 0; i < m.values.size(); ++i)
            os << m.values[i] << (i + 1 < m.values.size() ? ", " : "");
        return os << "]";
    }
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

    friend std::ostream& operator<<(std::ostream& os, const Both& b) {
        return os << "Both(" << b.first << ", " << b.second << ")";
    }
};

// =====================================================================
// 5. Tagged — a value with an untouched tag. flavor: keyed context (Writer seed)
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

    friend std::ostream& operator<<(std::ostream& os, const Tagged& t) {
        return os << "Tagged{" << t.tag << " = " << t.value << "}";
    }
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

    friend std::ostream& operator<<(std::ostream& os, const Tree& t) {
        os << "Tree ";
        t.stream_into(os);                   // label once; topology printed below
        return os;
    }

private:
    // Bare topology, recursive — no "Tree " prefix, so nested nodes stay clean.
    void stream_into(std::ostream& os) const {
        os << value;
        if (!children.empty()) {
            os << "(";
            for (std::size_t i = 0; i < children.size(); ++i) {
                children[i].stream_into(os);
                if (i + 1 < children.size()) os << " ";
            }
            os << ")";
        }
    }
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

    friend std::ostream& operator<<(std::ostream& os, const Lazy& l) {
        return os << "Lazy(forced -> " << l.force() << ")";
    }
};

// =====================================================================
// 8. Reader — a value computed on demand from an input R. flavor: input dependency
// The box is itself R -> T; mapping composes f after it, giving R -> U.
// (No operator<< — a Reader needs an input before it has anything to show.)
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
// A C++20 concept *describes* the common surface so the free helper below can
// accept any box with a member transform. (Per-type pattern: no shared base.)
// =====================================================================
template <typename Fa, typename Func>
concept Transformable = requires(Fa fa, Func f) {
    fa.transform(f);                         // simple-requirement: must compile
};

// map_over + trace, in one: apply the box's transform, print "before  ->  after",
// and return the result so calls chain. (Streamable boxes only; see Reader / the laws,
// which call .transform directly.)
template <typename Fa, typename Func> requires Transformable<Fa, Func>
auto traced(const Fa& fa, const Func& f) {
    auto out = fa.transform(f);
    std::cout << fa << "  ->  " << out << '\n';
    return out;
}

// Render any streamable box to text via operator<<.
// The functor laws are checked by comparing these renderings.
template <typename T>
std::string show(const T& x) {
    std::ostringstream os;
    os << x;
    return os.str();
}

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

    std::cout << "-- before  ->  after : one helper + one function, every box keeps its shape --\n";
    traced(a, times_ten);   // Box(5)  ->  Box(50)
    traced(b, times_ten);
    traced(c, times_ten);   // empty stays empty
    traced(d, times_ten);
    traced(e, times_ten);
    traced(g, times_ten);   // tag untouched
    traced(t, times_ten);   // topology preserved
    traced(z, times_ten);   // both sides forced at print time
    std::cout << "Reader(\"hello\")  " << len("hello") << "  ->  " << len.transform(times_ten)("hello") << '\n';

    // transform may also CHANGE the inner type (int -> string),
    // while the box kind / structure stays exactly the same.
    auto to_label = [](int x) { return std::string("#") + std::to_string(x); };

    std::cout << "\n-- contents change type, box kind does not --\n";
    traced(e, to_label);    // Both(4, 9)  ->  Both(#4, #9)
    traced(g, to_label);
    traced(t, to_label);

    auto id        = [](auto x) { return x; };
    auto add_one   = [](int x) { return x + 1; };
    auto times_two = [](int x) { return x * 2; };
    auto fused     = [&](int x) { return times_two(add_one(x)); };  // times_two ∘ add_one

    std::cout << std::boolalpha;

    // Apply a given law to every box once, so the box list lives in a single place.
    auto for_each_box = [&](auto law) {
        law("Box   ", a);
        law("Maybe ", b);
        law("Many  ", d);
        law("Both  ", e);
        law("Tagged", g);
        law("Tree  ", t);
    };

    // Law 1: transform(id) leaves the box unchanged (silent — calls .transform directly).
    auto identity_law = [&](const char* name, const auto& box) {
        std::cout << "  " << name << " : " << (show(box.transform(id)) == show(box)) << "\n";
    };
    std::cout << "\n-- Law 1, identity: transform(id) == id --\n";
    for_each_box(identity_law);

    // Law 2: mapping f then g equals mapping (g ∘ f) in a single pass.
    auto compose_law = [&](const char* name, const auto& box) {
        std::cout << "  " << name << " : "
                  << (show(box.transform(add_one).transform(times_two)) == show(box.transform(fused))) << "\n";
    };
    std::cout << "\n-- Law 2, composition: transform(f) then transform(g) == transform(g.f) --\n";
    for_each_box(compose_law);

    // Lazy and Reader hold deferred computations; run them (force / apply) and compare the resulting values directly.
    std::cout << "\n-- same laws, observed for the function-style boxes --\n"
              << "  Lazy   : "
              << (z.transform(add_one).transform(times_two).force() == z.transform(fused).force()) << "\n"
              << "  Reader : "
              << (len.transform(add_one).transform(times_two)("hello") == len.transform(fused)("hello")) << "\n";

    return 0;
}
