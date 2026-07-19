// ===========================================================================
// Currying & Partial Application in C++ — TODO exercise (detailed hints)
//
// Fill in the five TODOs, one at a time. After finishing a step, flip its
// STEPn_READY macro to 1 and rebuild: the tests for that step come alive.
// The file compiles and runs as shipped (all steps report TODO).
//
//   Build : g++ -std=c++20 -Wall -Wextra -o curry currying_partial_todo.cpp
//   Run   : ./curry
//
// Ground rules
//   * Standard library only. No std::bind, no std::bind_front — building
//     the machinery by hand is the exercise.
//   * Value semantics: every intermediate stage is an independent,
//     reusable value (steps 1–4). Step 5 deliberately relaxes this.
//   * Everything must compile clean under -Wall -Wextra.
//
// How the hints are layered
//   Each step carries three levels of help. Read in order, stopping as
//   early as possible:
//     SPEC   — the contract: what must hold, pinned down by the tests.
//     HINTS  — the mechanics: which language rules do the work, and the
//              traps between them.
//     SHAPE  — the skeleton, with the load-bearing expressions elided as
//              <...>. Peeking at it costs most of the fun.
// ===========================================================================

#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#define STEP1_READY 0
#define STEP2_READY 0
#define STEP3_READY 0
#define STEP4_READY 0
#define STEP5_READY 0  // bonus

// ---------------------------------------------------------------------------
// STEP 1 — curry2: the two-argument warm-up
//
// SPEC
//   curry2(f)(a)(b) == f(a, b)
//   * Nested generic lambdas; everything captured by value.
//   * Every intermediate stage is an independent, reusable value:
//       auto inc = curry2(add)(1);
//     inc(41) and inc(1) both work, long after the temporary returned by
//     curry2(add) has been destroyed.
//
// HINTS
//   * Structure: return a unary generic lambda; its body returns another
//     unary generic lambda; the innermost body finally applies f.
//   * The parameter f belongs to this function — move it into the outer
//     closure: f = std::move(f).
//   * The inner closure must COPY both f and a. Capturing by reference
//     would leave it holding dangling references into a stack frame (the
//     outer closure's operator()) that has long returned by the time
//     inc(41) runs.
//   * Const mechanics you will keep meeting: a non-mutable lambda's
//     operator() is const, so its by-value captures are const inside the
//     body. Calling the captured f therefore requires a const-invocable
//     callable — true for ordinary lambdas and function pointers, and a
//     restriction this whole exercise accepts on purpose.
//   * Lambda PARAMETERS are unaffected by mutability — a is an ordinary
//     mutable local of the outer operator(), so capturing it into the
//     inner closure may move it (a = std::move(a)) or simply copy it
//     ([f, a]). Note that f itself is const at that point, so "moving" f
//     there would silently copy anyway.
//
// SHAPE
//     return [ <...> ](auto a) {
//         return [ <...> ](auto b) { return <...>; };
//     };
// ---------------------------------------------------------------------------
template <class F>
auto curry2(F f) {
    // TODO(1)
}

// ---------------------------------------------------------------------------
// STEP 2 — partial: fix the leading arguments of any callable
//
// SPEC
//   partial(f, bound...)(rest...) == std::invoke(f, bound..., rest...)
//   * bound... are copied (rvalues: moved) at bind time; mutating the
//     originals afterwards must not leak into the partial application.
//   * rest... are perfectly forwarded at call time.
//   * The return type preserves references: a callable returning int&
//     must still return int& through partial.
//   * Pointers to member functions work as the callable.
//
// HINTS
//   * Bind time — C++20 pack expansion in an init-capture:
//       ...bound = std::forward<Bound>(bound)
//     Each init-capture deduces its type like auto, i.e. it DECAYS: even
//     when Bound deduced as std::string& (an lvalue argument), the closure
//     stores an independent std::string. That one rule delivers "copied at
//     bind time" (the greeting test), and forwarding into the capture
//     means rvalue arguments are moved in rather than copied.
//   * f arrives as a forwarding reference, so capture it the same way:
//       f = std::forward<F>(f)
//     (again decaying to a stored value).
//   * Call time — take auto&&... rest and forward with
//       std::forward<decltype(rest)>(rest)...
//     In a generic lambda, decltype(rest) names the deduced reference type
//     (T& for lvalue arguments, T&& for rvalues), so
//     forward<decltype(rest)> is the generic-lambda spelling of
//     forward<Rest> from a template function.
//   * Return type — declare the lambda -> decltype(auto). A plain deduced
//     auto return DECAYS, turning int& into int; the reference test then
//     fails to compile, because
//       partial(pick, true)(x, y) = 99
//     would be assigning to an int prvalue.
//   * Why std::invoke rather than f(bound..., rest...)? Pointers to
//     members are not callable with call syntax; invoke implements the
//     full INVOKE protocol (member pointer + object first). The Account
//     test exists precisely to punish a plain call.
//   * Const mechanics again: the returned closure is non-mutable, so f and
//     bound... are const inside — invoke sees const F& and const Bound&.
//     That keeps the partial application freely reusable, at the price of
//     supporting only const-invocable callables.
//
// SHAPE
//     return [ <capture f>, <pack init-capture bound...> ]
//            (auto&&... rest) -> <...> {
//         return std::invoke( <...> );
//     };
// ---------------------------------------------------------------------------
template <class F, class... Bound>
auto partial(F&& f, Bound&&... bound) {
    // TODO(2)
}

// ---------------------------------------------------------------------------
// STEP 3 — curry: arbitrary fixed arity, one argument per application
//
// SPEC
//   curry(f)(a1)(a2)...(an) == f(a1, ..., an)
//   * Arity >= 1. Resolution is EAGER: the first prefix of arguments with
//     which f is invocable gets invoked (so curry of a unary f applies
//     immediately — the neg test).
//   * No tuples in this step. Recursion on a wrapped callable.
//   * Every stage remains a reusable value.
//
// HINTS
//   1. curry(f) returns a unary lambda taking auto a.
//   2. Inside, branch at compile time with if constexpr on
//        std::is_invocable_v<F const&, decltype(a)&>
//      Why each piece is spelled that way:
//        * F const&      — the capture is const inside the non-mutable
//                          closure, so that is exactly how f(a) will
//                          invoke it.
//        * decltype(a)&  — a is used as a (non-const) lvalue in f(a);
//                          bare decltype(a) would model a value-typed a as
//                          an rvalue instead. Make the trait mirror the
//                          expression actually written — a habit step 4
//                          depends on.
//   3. Base case: return f(a);
//   4. Recursive case: build a wrapper w with  w(rest...) == f(a, rest...)
//      and return curry(w). Re-currying the wrapper — not returning it —
//      is what lets f1(10)(20) keep descending one level per argument.
//      Each wrap absorbs one argument by value ([f, a]; both are const
//      here, so these are plain copies), preserving the reusability of
//      every stage.
//   5. THE hint — the step lives or dies on this.
//      std::is_invocable_v<W, X> asks whether
//        std::declval<W>()(std::declval<X>())
//      is well-formed IN THE IMMEDIATE CONTEXT. For a lambda whose return
//      type is deduced (auto / decltype(auto) / omitted), computing the
//      result type of a call requires instantiating the BODY — and errors
//      there are hard errors, not substitution failures. One recursion
//      level down, the trait would explode with a compile error instead
//      of answering "no".
//      Fix: give the wrapper an explicit trailing return type,
//        -> decltype(f(a, std::forward<decltype(rest)>(rest)...))
//      Now the invocation expression is part of the DECLARATION; when it
//      is ill-formed for a given rest..., substitution into operator()'s
//      signature fails softly, and is_invocable correctly reports false.
//      (Expression SFINAE — the same mechanism underneath
//      std::invoke_result.)
//   6. Termination and a wart worth sitting with: nothing recurses at
//      definition time — each APPLIED argument instantiates exactly one
//      more level, and every level either invokes or wraps. But supplying
//      an argument of the wrong type never errors: is_invocable just
//      keeps saying "not yet", and the chain keeps absorbing garbage —
//        curry(add3)(1)(2)("x")
//      compiles and hands back yet another stage. Eager currying by
//      invocability cannot distinguish "not enough arguments" from
//      "wrong arguments"; telling them apart needs arity introspection.
//
// SHAPE
//     return [f = std::move(f)](auto a) {
//         if constexpr ( <invocable with just a?> ) {
//             return <...>;
//         } else {
//             return curry([f, a](auto&&... rest) -> <trailing type> {
//                 return <...>;
//             });
//         }
//     };
// ---------------------------------------------------------------------------
template <class F>
auto curry(F f) {
    // TODO(3)
}

// ---------------------------------------------------------------------------
// STEP 4 — Curried<F, Bound...>: apply argument GROUPS
//
// SPEC
//   auto cv = curried(volume);            // volume: int(int, int, int)
//   cv(2)(3)(4) == cv(2, 3)(4) == cv(2)(3, 4) == cv(2, 3, 4) == 24
//   * On each call: if f is invocable with (bound..., args...), invoke;
//     otherwise return a NEW Curried with the args appended (decayed) to
//     the bound tuple.
//   * const-callable and reusable: base(4) and then base(10) both work.
//
// HINTS
//   1. The trait:
//        std::is_invocable_v<F const&, Bound const&..., Args...>
//      * F const&        — f_ inside a const member function is const.
//      * Bound const&... — the stored values will be handed over as const
//                          lvalues (route A below).
//      * Args...         — deduced from Args&&, so Args is T& for lvalue
//                          arguments and T for rvalues; is_invocable
//                          treats a non-reference type as an rvalue, so
//                          the bare pack already carries the caller's
//                          value categories.
//   2. Invoke branch — two routes; route A is the one to build:
//      * Route A: std::apply a small lambda over bound_ that receives the
//        elements as auto const&... b and performs
//          std::invoke(f_, b..., std::forward<Args>(args)...)
//        The bound state travels exactly as the trait promised — const
//        lvalues, zero copies.
//      * Route B: std::apply(f_, std::tuple_cat(bound_,
//        std::forward_as_tuple(std::forward<Args>(args)...))). Works, but
//        tuple_cat from a const lvalue tuple COPIES every bound element,
//        and those copies then reach f as rvalues — not what the trait
//        checked. Harmless for value-taking callables; a latent mismatch
//        otherwise. Making the trait and the call agree to the letter is
//        the discipline this step teaches.
//   3. Bind branch:
//        std::tuple_cat(bound_,
//                       std::make_tuple(std::forward<Args>(args)...))
//      make_tuple decays (a const char* stays a pointer, lvalue strings
//      are copied, rvalues are moved in); tuple_cat from const bound_
//      copies the existing state — exactly what reusability demands.
//   4. Naming the new type: either spell it out,
//        Curried<F, Bound..., std::decay_t<Args>...>{ ... }
//      or lean on CTAD — the implicit deduction guide from the
//      (F, std::tuple<...>) constructor lets Curried{f_, <tuple>} deduce
//      everything.
//   5. Why const& and not plain const: step 5 adds an &&-qualified
//      overload, and ref-qualified overloads cannot coexist with an
//      unqualified one — committing to const& now keeps that door open.
//      Meanwhile, with STEP5_READY still 0, rvalue objects bind to const&
//      just fine: chains like cv(2, 3)(4) already work in this step.
//   6. The provided static_assert(sizeof...(Args) > 0): a zero-argument
//      call would either re-run an invocation that eager semantics
//      already performed, or bind nothing and return a pointless copy.
//      Ruled out up front.
//
// SHAPE (const& overload)
//     if constexpr ( <trait> ) {
//         return std::apply(
//             [ <...> ](auto const&... b) { return std::invoke( <...> ); },
//             bound_);
//     } else {
//         return Curried{ <...>,
//                         std::tuple_cat( <...>,
//                                         std::make_tuple( <...> )) };
//     }
//
// ---------------------------------------------------------------------------
// STEP 5 (bonus) — move-aware Curried
//
// SPEC
//   auto consume = [](std::unique_ptr<int> p, int x) { return *p + x; };
//   curried(consume)(std::make_unique<int>(40))(2) == 42
//   * Implement the &&-qualified overload (guarded by STEP5_READY).
//   * Lvalue Curried objects keep step-4 semantics; rvalues become
//     one-shot, move-only-friendly pipelines.
//
// HINTS
//   1. Overload resolution refresher: with const& and && overloads
//      present, an rvalue object (a temporary, or a std::move'd lvalue)
//      prefers &&; lvalues can only take const&. In the chain above every
//      stage is a temporary, so every application — including the very
//      first bind that moves the unique_ptr INTO bound_ — rides the &&
//      overload.
//   2. Inside &&, *this is expiring: cannibalise it — std::move(f_),
//      std::move(bound_).
//   3. The trait:  std::is_invocable_v<F, Bound..., Args...>
//      Non-reference types are treated as rvalues, which now truthfully
//      models the call: f invoked as an rvalue, bound elements moved out.
//      A stored unique_ptr therefore satisfies a unique_ptr-by-value
//      parameter here, where the const& trait said no.
//   4. Invoke branch: std::apply over std::move(bound_). std::get on an
//      rvalue tuple yields rvalue references; receive them as
//      auto&&... b and pass std::forward<decltype(b)>(b)... into
//      std::invoke(std::move(f_), ...).
//   5. Bind branch:
//        std::tuple_cat(std::move(bound_),
//                       std::make_tuple(std::forward<Args>(args)...))
//      tuple_cat from an rvalue tuple moves the existing elements across;
//      make_tuple moves rvalue arguments in. Construct the next stage
//      with std::move(f_).
//   6. The question. Bind a unique_ptr, then store the stage in a NAMED
//      variable and call it as an lvalue with the final argument:
//        auto stage = curried(consume)(std::make_unique<int>(40));
//        stage(2);                              // note: no std::move
//      Predict before compiling: which overload? what does the trait say?
//      which branch runs? Walk it through: lvalue -> const& overload; the
//      trait asks is_invocable_v<F const&, unique_ptr const&, int>;
//      consume takes the pointer BY VALUE, and initialising that
//      parameter from a const unique_ptr& needs the deleted copy
//      constructor -> the trait says NO -> control falls into the BIND
//      branch -> tuple_cat tries to copy the unique_ptr out of const
//      bound_ -> a compile error erupting from deep inside <tuple>.
//      Turn that avalanche into one readable line — in the const& bind
//      branch:
//        static_assert((std::is_copy_constructible_v<Bound> && ...),
//                      "bound state is move-only: invoke this Curried as "
//                      "an rvalue (std::move it)");
//      The asymmetry is the design: copyable bound state -> reusable;
//      move-only bound state -> one-shot, and the type system says so.
//      (The passing test therefore uses std::move(stage)(2).)
//
// SHAPE (&& overload)
//   Mirror the const& shape, with moves in place of const accesses.
// ---------------------------------------------------------------------------
template <class F, class... Bound>
class Curried {
    F f_;
    std::tuple<Bound...> bound_;

public:
    Curried(F f, std::tuple<Bound...> bound)
        : f_(std::move(f)), bound_(std::move(bound)) {}

    template <class... Args>
    auto operator()(Args&&... args) const& {
        static_assert(sizeof...(Args) > 0, "apply at least one argument");
        // TODO(4)
    }

#if STEP5_READY
    template <class... Args>
    auto operator()(Args&&... args) && {
        static_assert(sizeof...(Args) > 0, "apply at least one argument");
        // TODO(5): the move-aware twin of the overload above.
    }
#endif
};

template <class F>
auto curried(F f) {
    return Curried<F>{std::move(f), {}};
}

// ===========================================================================
// Tests — do not modify below this line. Each assert is annotated with the
// property it pins down (and the wrong implementation it catches).
// ===========================================================================
int main() {
    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        auto add = [](int a, int b) { return a + b; };
        assert(curry2(add)(2)(3) == 5);           // the basic contract

        auto inc = curry2(add)(1);
        // inc outlives the temporary from curry2(add): these two asserts
        // catch reference captures (dangling) and consumed state alike.
        assert(inc(41) == 42);
        assert(inc(1) == 2);

        auto repeat = [](std::string s, int n) {
            std::string out;
            while (n-- > 0) out += s;
            return out;
        };
        // heterogeneous argument types; "ab" is deduced as const char* by
        // the stage and converted to std::string only at the final call
        assert(curry2(repeat)("ab")(3) == "ababab");
        std::cout << "step 1  curry2 ................. ok\n";
    }
#else
    std::cout << "step 1  curry2 ................. TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        auto concat = [](std::string a, std::string b) { return a + b; };
        std::string greeting = "Hello, ";
        auto greet = partial(concat, greeting);
        greeting = "Bye, ";  // bound BY VALUE at bind time: this mutation
                             // must be invisible — catches reference
                             // capture of the bound pack
        assert(greet("World") == "Hello, World");

        struct Account {
            int balance;
            int deposit(int amount) const { return balance + amount; }
        };
        // a pointer to member function as the callable — catches a plain
        // f(bound..., rest...) call in place of std::invoke
        auto deposit100 = partial(&Account::deposit, Account{100});
        assert(deposit100(50) == 150);

        auto pick = [](bool first, int& a, int& b) -> int& { return first ? a : b; };
        int x = 1, y = 2;
        // the returned reference must survive the trip: catches a decayed
        // (plain auto) return type, which makes this line ill-formed
        partial(pick, true)(x, y) = 99;
        assert(x == 99 && y == 2);
        std::cout << "step 2  partial ................ ok\n";
    }
#else
    std::cout << "step 2  partial ................ TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        auto add3 = [](int a, int b, int c) { return a + b + c; };
        assert(curry(add3)(1)(2)(3) == 6);        // the basic contract

        auto f1 = curry(add3)(1);
        auto f12 = f1(2);
        // every stage is a reusable value: catches shared or consumed
        // state between applications, and dangling reference captures
        assert(f12(3) == 6);
        assert(f12(39) == 42);
        assert(f1(10)(20) == 31);

        auto neg = [](int v) { return -v; };
        assert(curry(neg)(5) == -5);              // eager: unary applies at once

        auto join4 = [](std::string a, std::string b, std::string c, std::string d) {
            return a + b + c + d;
        };
        // four levels deep — exercises the SFINAE-friendliness of the
        // wrapper at every intermediate arity
        assert(curry(join4)("a")("b")("c")("d") == "abcd");
        std::cout << "step 3  curry .................. ok\n";
    }
#else
    std::cout << "step 3  curry .................. TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        auto volume = [](int l, int w, int h) { return l * w * h; };
        auto cv = curried(volume);
        // all groupings agree — catches an is_invocable test that fires
        // too early or too late
        assert(cv(2)(3)(4) == 24);
        assert(cv(2, 3)(4) == 24);
        assert(cv(2)(3, 4) == 24);
        assert(cv(2, 3, 4) == 24);

        auto base = cv(2, 3);
        assert(base(4) == 24);
        assert(base(10) == 60);  // bound state must survive the previous
                                 // call — catches moved-from bound_
        std::cout << "step 4  Curried groups ......... ok\n";
    }
#else
    std::cout << "step 4  Curried groups ......... TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 (bonus) --------------------------------------------------
#if STEP5_READY
    {
        auto consume = [](std::unique_ptr<int> p, int x) { return *p + x; };
        // every stage is a temporary: the whole chain rides the &&
        // overload, moving the pointer bind-to-bind and into the call
        assert(curried(consume)(std::make_unique<int>(40))(2) == 42);

        auto stage = curried(consume)(std::make_unique<int>(40));
        // an lvalue with move-only bound state must be std::move'd —
        // stage(2) without the move should trip your static_assert
        assert(std::move(stage)(2) == 42);
        std::cout << "step 5  move-aware Curried ..... ok\n";
    }
#else
    std::cout << "step 5  move-aware Curried ..... TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
