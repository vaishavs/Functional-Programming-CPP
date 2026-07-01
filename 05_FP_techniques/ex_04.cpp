// One applicative box per family, generic over the value type.
// Build & run:  g++ -std=c++17 -pthread applicative_tmpl.cpp -o app && ./app
//
// Every box gives the same two things, generic:
//   pure_*<T>     — drop a plain value of ANY type T into the box
//   combine(f,..) — merge TWO boxes with a binary function f   (overloaded per box)
// `auto` return + one `decltype` keeps each combine a single clean line.

#include <functional>
#include <future>
#include <iostream>
#include <string>
#include <vector>

// 1. IDENTITY — one value, no extra structure.
template <class T> struct Box { T value; };
template <class T> Box<T> pure_box(T x) { return { x }; }
template <class F, class A, class B>
auto combine(F f, Box<A> a, Box<B> b) {
    return Box<decltype(f(a.value, b.value))>{ f(a.value, b.value) };
}

// 2. FAILURE/ERROR — Validation: a value, or errors that ACCUMULATE.
template <class T> struct Val {
    std::vector<std::string> errors; T value;
    bool ok() const { return errors.empty(); }
};
template <class T> Val<T> pure_val(T x)     { return { {}, x }; }
template <class T> Val<T> fail(std::string m) { return { { std::move(m) }, T{} }; }
template <class F, class A, class B>
auto combine(F f, Val<A> a, Val<B> b) {
    using R = decltype(f(a.value, b.value));
    std::vector<std::string> all = a.errors;                  // keep BOTH error lists
    all.insert(all.end(), b.errors.begin(), b.errors.end());
    if (all.empty()) return Val<R>{ {}, f(a.value, b.value) };
    return Val<R>{ all, R{} };
}

// 3. SHAPE — List (cartesian): every element against every element.
template <class T> using List = std::vector<T>;
template <class T> List<T> pure_list(T x) { return { x }; }
template <class F, class A, class B>
auto combine(F f, List<A> a, List<B> b) {
    std::vector<decltype(f(a.front(), b.front()))> out;
    for (auto& x : a) for (auto& y : b) out.push_back(f(x, y));
    return out;
}

// 4. FUNCTION — Reader: a value you get once you supply an env.
using Env = int;
template <class T> using Reader = std::function<T(Env)>;
template <class T> Reader<T> pure_reader(T x) { return [x](Env){ return x; }; }
template <class F, class A, class B>
auto combine(F f, Reader<A> a, Reader<B> b) {
    return [f, a, b](Env e){ return f(a(e), b(e)); };          // same env to both
}

// 5. MONOID — Writer: a value plus an accumulating log.
template <class T> struct Writer { T value; std::string log; };
template <class T> Writer<T> pure_writer(T x) { return { x, "" }; }
template <class F, class A, class B>
auto combine(F f, Writer<A> a, Writer<B> b) {
    return Writer<decltype(f(a.value, b.value))>{ f(a.value, b.value), a.log + b.log };
}

// 6. ASYNC — Future: a value that arrives later.
template <class T> using Fut = std::future<T>;
template <class T> Fut<T> pure_fut(T x) { std::promise<T> p; p.set_value(x); return p.get_future(); }
template <class F, class A, class B>
auto combine(F f, Fut<A> a, Fut<B> b) {
    return std::async(std::launch::deferred, [f, a = std::move(a), b = std::move(b)]() mutable { return f(a.get(), b.get()); });
}

int main() {
    auto add = [](auto x, auto y){ return x + y; };           // generic binary function

    // ---------------- 1. IDENTITY (Box) ----------------
    std::cout << "=== Identity (Box) ===\n";
    Box<int> b1 = pure_box(2), b2 = pure_box(3);
    std::cout << "Before: box1=" << b1.value << ", box2=" << b2.value << '\n';
    std::cout << "After : " << combine(add, b1, b2).value << '\n';

    Box<std::string> s1 = pure_box(std::string("a"));
    Box<std::string> s2 = pure_box(std::string("b"));
    std::cout << "Before: box1=\"" << s1.value << "\", box2=\"" << s2.value << "\"\n";
    std::cout << "After : \"" << combine(add, s1, s2).value << "\"\n\n";

    // ---------------- 2. VALIDATION (Val) ----------------
    std::cout << "=== Validation (Val) ===\n";
    Val<int> v1 = pure_val(2), v2 = pure_val(3);
    std::cout << "Before: val1=" << v1.value << " (ok=" << v1.ok()
               << "), val2=" << v2.value << " (ok=" << v2.ok() << ")\n";
    Val<int> ok = combine(add, v1, v2);
    std::cout << "After : value=" << ok.value << ", ok=" << ok.ok() << "\n\n";

    Val<int> f1 = fail<int>("name empty"), f2 = fail<int>("age negative");
    std::cout << "Before: val1=<error: " << f1.errors[0]
               << ">, val2=<error: " << f2.errors[0] << ">\n";
    Val<int> bad = combine(add, f1, f2);
    std::cout << "After : errors=";
    for (auto& e : bad.errors) std::cout << " [" << e << ']';
    std::cout << "\n\n";

    // ---------------- 3. LIST (cartesian) ----------------
    std::cout << "=== List (cartesian) ===\n";
    List<int> l1{1, 2}, l2{10, 20};
    std::cout << "Before: list1=[1 2], list2=[10 20]\n";
    std::cout << "After :";
    for (int x : combine(add, l1, l2)) std::cout << ' ' << x;
    std::cout << "\n\n";

    // ---------------- 4. READER (function of Env) ----------------
    std::cout << "=== Reader (function) ===\n";
    Reader<int> ra = [](Env e){ return e + 1; }, rb = [](Env e){ return e * 10; };
    Env env = 5;
    std::cout << "Before: ra(5)=" << ra(env) << ", rb(5)=" << rb(env) << " (env=" << env << ")\n";
    std::cout << "After : " << combine(add, ra, rb)(env) << "\n\n";

    // ---------------- 5. WRITER (value + log) ----------------
    std::cout << "=== Writer (value + log) ===\n";
    Writer<int> w1{2, "got2; "}, w2{3, "got3; "};
    std::cout << "Before: w1=(value=" << w1.value << ", log=\"" << w1.log
               << "\"), w2=(value=" << w2.value << ", log=\"" << w2.log << "\")\n";
    Writer<int> w = combine(add, w1, w2);
    std::cout << "After : value=" << w.value << ", log=\"" << w.log << "\"\n\n";

    // ---------------- 6. FUTURE (async) ----------------
    std::cout << "=== Future (async) ===\n";
    std::cout << "Before: fut1=<pending 2>, fut2=<pending 3>\n";
    auto result = combine(add, pure_fut(2), pure_fut(3)).get();
    std::cout << "After : " << result << "\n";
}
