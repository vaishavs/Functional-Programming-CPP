// ============================================================================
//  Currying & Partial Application in C++ — Solutions & Commentary
// ============================================================================
//
//  Build:
//      g++ -std=c++20 -Wall -Wextra -O0 -g curry_partial_solutions.cpp -o sol
//      ./sol            (every line should end in [OK])
//
//  The scorecard:
//
//   #  Bug                                  Root cause        Rule of thumb
//  --- ------------------------------------ ----------------- ----------------------------------------
//   1  f(b, a) in the innermost closure     logic             Test curried code with NON-commutative
//                                                             ops; + and * prove nothing about order.
//   2  [&x] on a by-value parameter         lifetime / UB     Every stage of a curried chain must OWN
//                                                             its argument: capture by value, or
//                                                             [x = std::move(x)] for heavy types.
//   3  std::bind decay-copies bound args    binding time      Decide PER ARGUMENT: snapshot (value)
//                                                             vs live view (std::ref) — and make the
//                                                             choice visible in the code.
//   4  std::move out of captures + mutable  move semantics    Move INTO closures at construction.
//                                                             Never move OUT at invocation if the
//                                                             callable promises reuse.
//   5  `auto` params + naive re-captures    value category    auto&& parameters, std::forward into
//                                                             init-captures, forward the final arg
//                                                             straight through. Re-captures that
//                                                             remain are the price of reusability.
// ============================================================================

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

namespace check {
template <class T>
void line(const char* tag, const T& got, const T& expected) {
    std::cout << tag << " got: " << got
              << (got == expected ? "   [OK]" : "   [STILL BROKEN]") << '\n';
}

// Doubles get an epsilon: 100.0 * 1.10 is not exactly 110.0, and a checker
// that forgets that would fail its own Exercise 3.
inline void line(const char* tag, double got, double expected) {
    std::cout << tag << " got: " << got
              << (std::abs(got - expected) < 1e-9 ? "   [OK]" : "   [STILL BROKEN]")
              << '\n';
}
} // namespace check

// ----------------------------------------------------------------------------
//  Exercise 1 — The Order of Things
// ----------------------------------------------------------------------------
//  Bug:  the innermost lambda called f(b, a).
//  Fix:  call f(a, b). One transposition.
//
//  The buggy version wasn't nonsense — it was `flip` (Haskell's flip, i.e.
//  the C combinator) wearing curry2's name tag. Argument-order combinators
//  are useful; MISLABELED ones are landmines, and commutative test data
//  (operator+) will never step on them. Always include a non-commutative
//  operation (sub, div, string concat) in tests for anything that shuffles
//  arguments.
// ----------------------------------------------------------------------------
namespace ex1 {

auto curry2 = [](auto f) {
    return [f](auto a) {
        return [f, a](auto b) {
            return f(a, b);              // FIX: was f(b, a)
        };
    };
};

// Bonus: the bug, promoted to a first-class citizen with an honest name.
auto flip = [](auto f) {
    return [f](auto a, auto b) { return f(b, a); };
};

void run() {
    auto sub = [](int a, int b) { return a - b; };
    check::line("[ex1] 10 - 4      ", curry2(sub)(10)(4), 6);

    auto cat = [](std::string a, std::string b) { return a + b; };
    check::line("[ex1] foo:: + bar ",
                curry2(cat)(std::string("foo::"))(std::string("bar")),
                std::string("foo::bar"));

    check::line("[ex1] flip(sub)   ", flip(sub)(4, 10), 6);
}

} // namespace ex1

// ----------------------------------------------------------------------------
//  Exercise 2 — The Vanishing Captive
// ----------------------------------------------------------------------------
//  Bug:  [&x] captured a reference to make_adder's by-value parameter. That
//        parameter dies at make_adder's closing brace; the closure walks out
//        holding a pointer into a dead stack frame. Every later call reads
//        whatever now lives at that address — which is why both adders
//        "agreed" on one garbage value: successive calls at the same depth
//        reuse the same frame, so both closures pointed at the SAME slot.
//
//  Fix:  capture by value. One deleted character. The closure now owns its
//        stage's argument, which is the invariant every curried chain must
//        maintain: a stage can outlive everything that built it.
//
//  Notes:
//  * For move-only or heavy types, the ownership transfer is spelled
//    [x = std::move(x)]. Same invariant, explicit about the cost.
//  * Neither gcc 13 nor clang reliably diagnoses this capture pattern with
//    -Wall -Wextra. ASan with detect_stack_use_after_return=1 nails it at
//    the exact variable and line. Budget a sanitizer stage for any code
//    that returns closures.
//  * "It printed 8 on my machine" is not evidence of correctness; it is
//    evidence that the corpse hadn't been disturbed yet.
// ----------------------------------------------------------------------------
namespace ex2 {

auto make_adder(int x) {
    return [x](int y) {                  // FIX: was [&x]
        return x + y;
    };
}

void run() {
    auto add5 = make_adder(5);
    auto add7 = make_adder(7);
    check::line("[ex2] add5(3)     ", add5(3), 8);
    check::line("[ex2] add7(3)     ", add7(3), 10);
}

} // namespace ex2

// ----------------------------------------------------------------------------
//  Exercise 3 — Frozen in Time
// ----------------------------------------------------------------------------
//  Bug:  std::bind decay-copies every bound argument into the call wrapper
//        AT BIND TIME. The wrapper's tax_rate and run()'s tax_rate were two
//        unrelated doubles from that moment on; reassigning one could never
//        reach the other.
//
//  Fix:  std::ref(tax_rate) stores a reference_wrapper instead, making the
//        partial application a live view.
//
//  Notes:
//  * This is the ONE decision that defines a partial application's contract:
//    snapshot or view. Both are legitimate; the bug was wanting a view and
//    silently writing a snapshot. Whatever you choose, make it visible.
//  * The lambda spelling states the choice louder than bind does:
//        snapshot:  [rate = tax_rate](double p) { return apply_rate(p, rate); }
//        view:      [&tax_rate](double p) { return apply_rate(p, tax_rate); }
//    ...and the view variant immediately raises Exercise 2's question:
//    does the closure outlive tax_rate? std::ref has the exact same dangling
//    hazard — reference semantics never come for free.
//  * In modern code, prefer the lambda; bind survives mostly in legacy
//    call sites and in this exercise.
// ----------------------------------------------------------------------------
namespace ex3 {

double apply_rate(double price, double rate) { return price * (1.0 + rate); }

void run() {
    double tax_rate = 0.10;

    auto taxed = std::bind(apply_rate, std::placeholders::_1,
                           std::ref(tax_rate));          // FIX: was tax_rate

    check::line("[ex3] rate=0.10   ", taxed(100.0), 110.0);
    tax_rate = 0.20;
    check::line("[ex3] rate=0.20   ", taxed(100.0), 120.0);
}

} // namespace ex3

// ----------------------------------------------------------------------------
//  Exercise 4 — The One-Shot Wonder
// ----------------------------------------------------------------------------
//  Bug:  invoke-time std::move(bound)... — plus the `mutable` that made it
//        real. Without `mutable`, captures are const members, so
//        std::move(bound) yields const T&&, which binds to const T& and
//        quietly COPIES: wrong intent, correct behavior. Adding `mutable`
//        made the moves genuine, and join's by-value parameters stole the
//        string buffers on call #1. Call #2 ran on the moved-from husks.
//
//  Fix:  keep the CONSTRUCTION-time moves (the init-captures — those are
//        correct and free), delete the INVOKE-time moves and the `mutable`.
//        A reusable callable may lend its captures on every call; it may
//        never give them away.
//
//  The two moves, side by side:
//        [f = std::move(f), ...bound = std::move(bound)]   // into the closure: GOOD
//        std::invoke(std::move(f), std::move(bound)...)    // out, every call: SINGLE-SHOT
//
//  C++23 bonus — have both. Deducing `this` threads the closure's OWN value
//  category through to the captures, so an lvalue invocation copies and an
//  explicit rvalue invocation moves (needs gcc >= 14 / clang >= 18):
//
//      return [f = std::move(f), ...bound = std::move(bound)]
//             (this auto&& self, auto&&... rest) {
//          return std::invoke(
//              std::forward_like<decltype(self)>(f),
//              std::forward_like<decltype(self)>(bound)...,
//              std::forward<decltype(rest)>(rest)...);
//      };
//
//      path_join("/");             // lvalue call:  copies out, reusable
//      std::move(path_join)("/");  // rvalue call:  moves out, deliberate one-shot
// ----------------------------------------------------------------------------
namespace ex4 {

template <class F, class... Bound>
auto partial(F f, Bound... bound) {
    return [f = std::move(f), ...bound = std::move(bound)](auto&&... rest) {
        return std::invoke(f, bound...,                  // FIX: no moves, no mutable
                           std::forward<decltype(rest)>(rest)...);
    };
}

std::string join(std::string a, std::string b, const std::string& sep) {
    return a + sep + b;
}

void run() {
    auto path_join = partial(join,
                             std::string("/opt/vendor/toolchain-x86_64"),
                             std::string("release-candidate-builds"));

    const std::string expected =
        "/opt/vendor/toolchain-x86_64/release-candidate-builds";

    check::line("[ex4] call #1     ", path_join("/"), expected);
    check::line("[ex4] call #2     ", path_join("/"), expected);
    check::line("[ex4] call #3     ", path_join("/"), expected);
}

} // namespace ex4

// ----------------------------------------------------------------------------
//  Exercise 5 — The Copy Avalanche
// ----------------------------------------------------------------------------
//  The six copies in the buggy version, attributed:
//
//      1. stage A init-capture [f, a]  <- copies parameter a      AVOIDABLE
//      2. stage B init-capture of b    <- copies parameter b      AVOIDABLE
//      3. stage B re-capture of a      <- a must survive in B     floor
//      4. final call: a into weigh     <- by-value callee         floor
//      5. final call: b into weigh     <- by-value callee         floor
//      6. final call: c into weigh     <- copies parameter c      AVOIDABLE
//
//  Root cause of 1, 2, 6: a parameter declared `auto x` has already
//  flattened the caller's rvalue into a plain lvalue — the value category is
//  gone before you can use it. `auto&& x` preserves it; then
//  x = std::forward<decltype(x)>(x) MOVES rvalues into the init-capture and
//  copies lvalues, exactly once each. And c never needed capturing at all:
//  the innermost stage forwards it straight into f.
//
//  Why 3, 4, 5 are the floor for THIS design: reusable stages. stage_a may
//  be invoked again, so invoking it cannot strip its captured a — the next
//  closure gets a copy (#3). Likewise the final call cannot move a and b
//  into weigh's by-value parameters, because the innermost closure might be
//  called again (#4, #5). A stage that can be re-invoked can lend, never
//  give.
//
//  Moving the floor means changing the deal:
//    * weigh(const Payload&, ...)  -> #4/#5 vanish; floor drops to 1.
//    * C++23 deducing `this` (as in Exercise 4) -> std::move(stage)(x)
//      becomes an opt-in one-shot that really moves; lvalue calls stay safe.
//    * A single-closure design (accumulate arguments into a std::tuple,
//      std::apply at the end) trades the nested re-captures for one tuple —
//      same floor analysis, different geometry.
// ----------------------------------------------------------------------------
namespace ex5 {

struct Payload {
    static inline int copies = 0;
    static inline int moves  = 0;

    int id = 0;
    explicit Payload(int i) : id(i) {}
    Payload(const Payload& o) : id(o.id) { ++copies; }
    Payload(Payload&& o) noexcept : id(o.id) { ++moves; }
    Payload& operator=(const Payload& o) { id = o.id; ++copies; return *this; }
    Payload& operator=(Payload&& o) noexcept { id = o.id; ++moves; return *this; }
};

auto curry3 = [](auto f) {
    return [f = std::move(f)](auto&& a) {                       // FIX: auto&&
        return [f, a = std::forward<decltype(a)>(a)](auto&& b) {          // FIX
            return [f, a, b = std::forward<decltype(b)>(b)](auto&& c) {   // FIX
                return f(a, b, std::forward<decltype(c)>(c));   // FIX: c not captured
            };
        };
    };
};

int weigh(Payload a, Payload b, Payload c) { return a.id + b.id + c.id; }

void run() {
    Payload::copies = 0;
    Payload::moves  = 0;

    auto curried = curry3(weigh);
    auto stage_a = curried(Payload{100});   // move into capture
    auto stage_b = stage_a(Payload{20});    // copy a (reuse), move b into capture
    const int r  = stage_b(Payload{3});     // copy a, b into weigh; forward c

    check::line("[ex5] result      ", r, 123);
    check::line("[ex5] copies      ", Payload::copies, 3);
    check::line("[ex5] moves       ", Payload::moves, 3);

    // Reusability held: the stages still work, same answer, no fresh UB.
    check::line("[ex5] stage reuse ", stage_b(Payload{3}), 123);
}

} // namespace ex5

// ----------------------------------------------------------------------------
int main() {
    ex1::run();
    ex2::run();
    ex3::run();
    ex4::run();
    ex5::run();
}
