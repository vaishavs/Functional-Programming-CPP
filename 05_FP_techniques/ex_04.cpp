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

    std::cout << "Identity  : " << combine(add, pure_box(2), pure_box(3)).value << '\n';
    std::cout << "Identity  : " << combine(add, pure_box(std::string("a")),       // SAME combine,
                                                 pure_box(std::string("b"))).value // T = string
              << '\n';

    Val<int> ok  = combine(add, pure_val(2), pure_val(3));
    Val<int> bad = combine(add, fail<int>("name empty"), fail<int>("age negative"));
    std::cout << "Valid ok  : " << ok.value << '\n';
    std::cout << "Valid bad :";
    for (auto& e : bad.errors) std::cout << " [" << e << ']';
    std::cout << '\n';

    std::cout << "List      :";
    for (int x : combine(add, List<int>{1, 2}, List<int>{10, 20})) std::cout << ' ' << x;
    std::cout << '\n';

    Reader<int> ra = [](Env e){ return e + 1; }, rb = [](Env e){ return e * 10; };
    std::cout << "Reader    : " << combine(add, ra, rb)(5) << '\n';

    Writer<int> w = combine(add, Writer<int>{2, "got2; "}, Writer<int>{3, "got3; "});
    std::cout << "Writer    : " << w.value << "  log=\"" << w.log << "\"\n";

    std::cout << "Future    : " << combine(add, pure_fut(2), pure_fut(3)).get() << '\n';
}
