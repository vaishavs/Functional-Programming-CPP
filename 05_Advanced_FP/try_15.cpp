// ===========================================================================
// The Box Model of Monads in C++ — TODO/FIXME exercise
//
// One file, five steps, several kinds of work:
//   TODO(n)     — implement from scratch.
//   FIXME(n)    — the shipped code is wrong on purpose. Convict it with
//                 the law checkers, then repair it.
//   PREDICT     — commit to an answer before compiling.
//   QUESTION    — answer in a comment next to the marker.
//   PROVE       — a short paper argument, written as a comment.
//   EXPERIMENT  — a controlled sabotage: introduce a defect, predict the
//                 verdict, verify, restore.
//
// After finishing a step, flip its STEPn_READY macro to 1 and rebuild.
// Steps depend on earlier ones — enable them in order. The file compiles
// and runs as shipped (all steps report TODO).
//
//   Build : g++ -std=c++20 -Wall -Wextra -o monad box_monad_todo.cpp
//   Run   : ./monad
//
// The contract
//   Builds on fmap (functor exercise) and pure (applicative exercise).
//   Each monad M now also needs
//     mbind : M<T> x (T -> M<U>) -> M<U>
//   where the continuation (T -> M<U>) PRODUCES the next box from the
//   current value — the thing applicative's ap could not do. Three laws
//   (step 2 makes them executable), checked UP TO operator==:
//     left identity    mbind(pure(x), k)  ==  k(x)
//     right identity   mbind(m, pure)     ==  m
//     associativity    mbind(mbind(m, k), h)
//                          ==  mbind(m, x |-> mbind(k(x), h))
//
//   Two more operations built here: kfish (step 3), which composes two
//   continuations into one, and mjoin (step 4), which flattens M<M<T>>
//   into M<T> with mbind(m, k) = mjoin(fmap(k, m)).
//
//   Kleisli arrows (the k, h continuations) are functions, so tests that
//   compare them do it extensionally, at sample points, not with ==. And
//   as in the earlier two files: some defects are invisible to some laws.
//   Step 5 sharpens that to one defect per law, one law per defect.
//
// Ground rules
//   * Standard library only.
//   * Value semantics throughout; mbind never mutates its inputs.
//   * Everything must compile clean under -Wall -Wextra.
//
// Hint layering per step: SPEC (the contract, pinned by the tests), HINTS
// (the mechanics), SHAPE (the skeleton, load-bearing parts elided as
// <...>). Stop reading as early as you can.
// ===========================================================================

#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#define STEP1_READY 0
#define STEP2_READY 0
#define STEP3_READY 0
#define STEP4_READY 0
#define STEP5_READY 0

// ---------------------------------------------------------------------------
// Given — carried over from the functor and applicative exercises, lawful
// as you left them (LoggedBox's pure ships post-repair, with an empty log).
// ---------------------------------------------------------------------------
inline constexpr auto id = [](auto&& x) -> decltype(auto) {
    return std::forward<decltype(x)>(x);
};

template <class T>
struct Box {
    T value;
    friend bool operator==(Box const&, Box const&) = default;
};

template <class F, class T>
auto fmap(F&& f, Box<T> const& b) {
    // CTAD's copy-deduction candidate makes Box{expr} a COPY when expr
    // is already a Box, silently collapsing Box<Box<U>> to Box<U>. Step
    // 4's join needs genuine nesting, so the target type is spelled out.
    using U = std::remove_cvref_t<std::invoke_result_t<F&, T const&>>;
    return Box<U>{std::invoke(f, b.value)};
}

inline constexpr auto box_pure = [](auto x) {
    // Spelled for the same reason: pure of a box must NEST, and
    // Box{already_a_box} would copy instead.
    return Box<std::decay_t<decltype(x)>>{std::move(x)};
};

template <class T>
struct MaybeBox {
    std::optional<T> slot;
    friend bool operator==(MaybeBox const&, MaybeBox const&) = default;
};

template <class F, class T>
auto fmap(F&& f, MaybeBox<T> const& b) {
    using U = std::remove_cvref_t<std::invoke_result_t<F&, T const&>>;
    if (b.slot) return MaybeBox<U>{std::invoke(f, *b.slot)};
    return MaybeBox<U>{};
}

inline constexpr auto maybe_pure = [](auto x) {
    return MaybeBox<std::decay_t<decltype(x)>>{std::move(x)};
};

template <class T>
struct LoggedBox {
    T value;
    std::string log;  // combines with "" as identity, + as the operation
    friend bool operator==(LoggedBox const&, LoggedBox const&) = default;
};

template <class F, class T>
auto fmap(F&& f, LoggedBox<T> const& b) {
    return LoggedBox{std::invoke(f, b.value), b.log};
}

inline constexpr auto logged_pure = [](auto x) {
    return LoggedBox{std::move(x), std::string{}};
};

// ---------------------------------------------------------------------------
// STEP 1 — mbind for Box: sequencing with the effects turned off
//
// SPEC
//   mbind(Box{x}, k)  ==  k(x)        where k : T -> Box<U>
//   * No reboxing. The continuation already chose and built the result
//     box; mbind hands the payload over and returns what k returns.
//   * Heterogeneous continuations must work (int -> Box<std::string>).
//   * Inputs taken by const&; nothing mutated.
//
// HINTS
//   * Read the signatures side by side until the difference clicks:
//       fmap  :  (T -> U)       x  Box<T>  ->  Box<U>      (fmap wraps)
//       mbind :  Box<T>  x  (T -> Box<U>)  ->  Box<U>      (mbind trusts)
//     fmap's function is oblivious to the box, so fmap must rebox.
//     mbind's continuation RETURNS a box — it decides presence,
//     annotation, everything. A bind that wraps again produces
//     Box<Box<U>>, and the tests will not even compile against it: the
//     type system convicts before the laws get a chance.
//   * One line: invoke k on the payload, return the result. std::invoke,
//     as always. The deduced return type is k's business.
//   * Box just passes values through unchanged, so steps 2–3 can study
//     the mbind/kfish machinery before MaybeBox adds real effects in
//     step 4.
//   * QUESTION(1): why is this mbind and not bind? Unqualified calls are
//     how generic code finds the right overload (ADL). But once a box's
//     template argument is a std type — Box<std::string> arrives in step
//     2 — namespace std becomes an ASSOCIATED namespace, and an
//     unqualified bind(...) drags std::bind into the overload set.
//     Ambiguity or worse. Name accordingly.
//
// SHAPE
//     template <class T, class K>
//     auto mbind(Box<T> const& m, K&& k) {
//         return <...>;
//     }
// ---------------------------------------------------------------------------
template <class T, class K>
auto mbind(Box<T> const& m, K&& k) {
    // TODO(1)
}

// ---------------------------------------------------------------------------
// STEP 2 — the three laws, as executables
//
// SPEC
//   left_identity_law(pure, x, k)   ==  (mbind(pure(x), k) == k(x))
//   right_identity_law(pure, m)     ==  (mbind(m, pure) == m)
//   associativity_law(m, k, h)      ==  (mbind(mbind(m, k), h)
//                                          == mbind(m, x |-> mbind(k(x), h)))
//   * Generic over the functor: pure travels as an explicit parameter,
//     mbind is found by unqualified call + ADL. Unchanged for Box now,
//     MaybeBox and LoggedBox later.
//
// HINTS
//   * What each law is FOR:
//       left identity    entering the boxed world through pure and
//                        immediately binding is just calling k: pure is
//                        a frictionless ENTRY.
//       right identity   ending a chain with pure changes nothing: pure
//                        is a frictionless EXIT.
//       associativity    THE refactoring law. Extracting any sub-chain
//                        of binds into a named helper cannot change
//                        behaviour. Without it, "pull this pipeline
//                        stage into a function" would be a semantic
//                        gamble.
//   * Notice which checker takes no pure: associativity. It's a separate
//     piece of data from mbind, not derived from it.
//   * The composed continuation inside the associativity checker is a
//     small lambda capturing k and h by value, taking x by const&, and
//     binding k's result into h. Build it inside the checker.
//   * mbind(m, pure): pure IS a continuation — it has exactly the arrow
//     shape T -> M<T>. That observation is the seed of step 3.
//   * std::invoke wherever a user-supplied callable is applied.
//
// SHAPE
//     template <class Pure, class X, class K>
//     auto left_identity_law(Pure pure, X x, K k) {           // -> bool
//         return <...> == <...>;
//     }
//     template <class Pure, class M>
//     auto right_identity_law(Pure pure, M const& m) {        // -> bool
//         return <...> == <...>;
//     }
//     template <class M, class K, class H>
//     auto associativity_law(M const& m, K k, H h) {          // -> bool
//         auto composed = [ <...> ](auto const& x) { return <...>; };
//         return <...> == <...>;
//     }
// ---------------------------------------------------------------------------
template <class Pure, class X, class K>
auto left_identity_law(Pure pure, X x, K k) {  // deduces bool once implemented
    // TODO(2a)
}

template <class Pure, class M>
auto right_identity_law(Pure pure, M const& m) {  // deduces bool once implemented
    // TODO(2b)
}

template <class M, class K, class H>
auto associativity_law(M const& m, K k, H h) {  // deduces bool once implemented
    // TODO(2c)
}

// ---------------------------------------------------------------------------
// STEP 3 — kfish: composing two continuations into one
//
// SPEC
//   kfish(k, h)(x)  ==  mbind(k(x), h)         ("k, then h", as one arrow)
//
// HINTS
//   * kfish: capture k and h by value; return a lambda taking x by
//     const&, binding k's result into h. It is the associativity
//     checker's composed continuation, promoted to a first-class
//     combinator.
//   * PROVE(3) — three one-liners. Substitute kfish's definition into
//     each line below and a monad law falls out directly:
//       kfish(pure, k)(x) == k(x)        <->  left identity
//       kfish(k, pure)(x) == k(x)        <->  right identity
//       kfish(kfish(k, h), g) == kfish(k, kfish(h, g))
//                                        <->  associativity
//   * Testing note: k and h are functions, so the tests compare them by
//     calling both sides at sample points, not with ==.
//
// SHAPE
//     template <class K, class H>
//     auto kfish(K k, H h) {
//         return [ <...> ](auto const& x) { return <...>; };
//     }
// ---------------------------------------------------------------------------
template <class K, class H>
auto kfish(K k, H h) {
    // TODO(3)
}

// ---------------------------------------------------------------------------
// STEP 4 — MaybeBox: sequencing that can stop, and the join bridge
//
// SPEC
//   mbind on MaybeBox: if m is empty, the result is empty AT K'S RESULT
//   TYPE and k is NEVER INVOKED (a test enforces this with a continuation
//   that aborts the process if called). If m holds x, the result is k(x).
//   mjoin(mm)             ==  mm flattened by one layer: M<M<T>> -> M<T>
//   mbind_via_join(m, k)  ==  mjoin(fmap(k, m)), agreeing with mbind on
//                             every functor in the file.
//
// HINTS
//   * bind does not know U and does not need to — the CONTINUATION owns
//     the output type. Name it once —
//       using MU = std::remove_cvref_t<std::invoke_result_t<K&, T const&>>;
//     — MU is k's whole RESULT BOX type, not a payload. The empty branch
//     returns MU{}; all mbind needs from MU is that default construction
//     means "empty".
//   * Do not "recover" from an empty input by invoking k on T{} — that
//     manufactures a computation out of structure, breaks right identity
//     (mbind(empty, pure) would resurrect a value), and the abort-test
//     convicts it before the laws even run.
//   * half_if_even returns a DIFFERENT box depending on the value it
//     sees, so whether the pipeline's third stage runs at all can't be
//     known without running the first two — that's what mbind can do
//     that ap couldn't.
//   * mjoin is one line: bind with the identity-shaped continuation —
//     mbind(mm, id). QUESTION(4a): why does id work here as a
//     continuation? (Look at what arrow shape M<T> -> M<T> has when the
//     outer payload is itself a box.)
//   * mbind_via_join: fmap wraps k's boxes into a second layer, join
//     flattens the layers away — mjoin(fmap(k, m)). The round-trip tests
//     pin the agreement with the direct mbind.
//   * A trap your own test-tinkering could hit: CTAD's copy-deduction
//     candidate is preferred for a single argument of the same class
//     template, so Box{Box{7}} constructs a COPY of Box{7}, not a
//     nesting. Every genuinely nested box in the tests spells its type
//     in full (Box<Box<int>>, MaybeBox<MaybeBox<int>>), and Box's given
//     fmap/pure name their target types for the same reason: a
//     box-returning continuation fed to a CTAD-built fmap would have its
//     nesting silently collapsed, leaving join nothing to flatten.
//     QUESTION(4c): which of the two would be worse to debug, and why
//     did the functor exercise never notice?
//
// SHAPE
//     template <class T, class K>
//     auto mbind(MaybeBox<T> const& m, K&& k) {
//         using MU = <...>;
//         if ( <...> ) return MU( <...> );
//         return MU{};
//     }
//
//     template <class MM>
//     auto mjoin(MM const& mm) {
//         return <...>;
//     }
//
//     template <class M, class K>
//     auto mbind_via_join(M const& m, K k) {
//         return <...>;
//     }
// ---------------------------------------------------------------------------
template <class T, class K>
auto mbind(MaybeBox<T> const& m, K&& k) {
    // TODO(4a)
}

template <class MM>
auto mjoin(MM const& mm) {
    // TODO(4b)
}

template <class M, class K>
auto mbind_via_join(M const& m, K k) {
    // TODO(4c)
}

// ---------------------------------------------------------------------------
// STEP 5 — LoggedBox: the Writer monad, shipped with a one-law defect
//
// The lawful instance: run the continuation on the payload, keep ITS
// value, and concatenate the annotations in order, each exactly once:
//     value :  k(m.value).value
//     log   :  m.log + k(m.value).log
// pure (given above, post-repair) contributes the empty log.
//
// SPEC (post-repair)
//   All three checkers pass; pipelines concatenate logs left-to-right,
//   once each; kfish sequences annotations; mbind_via_join agrees with
//   mbind.
//
// The bug hunt — read before flipping STEP5_READY
//   FIXME(A) — the shipped mbind drops the continuation's log and keeps
//     only m.log.
//   PREDICT the full three-law verdict sheet for the shipped code before
//   running. QUESTION(5a): EXACTLY ONE law convicts — work the log
//   algebra for the other two. Right identity survives because pure's
//   log is the unit (nothing was there to drop); associativity survives
//   because BOTH bracketings drop their way down to the same m.log.
//
//   EXPERIMENT(5b) — after repairing (A), commit a controlled sabotage:
//   swap the log to r.log alone (the MIRROR defect — drop the input's
//   history instead). PREDICT first: which single law convicts now?
//   Rebuild, verify the abort lands where you said, restore the repair.
//   Between (A) and its mirror you will have shown: left identity
//   watches the unit from the LEFT, right identity watches it from the
//   RIGHT, and associativity acquits both.
//
//   PROVE(5c) — show that the value components agree in every law
//   regardless of the log policy, so each monad law reduces EXACTLY to
//   one law of the annotation's combining operation:
//       left identity    <->   "" + a  ==  a         (left unit)
//       right identity   <->   a + ""  ==  a         (right unit)
//       associativity    <->   (a + b) + c  ==  a + (b + c)
//   Conclusion: LoggedBox is a lawful monad IF AND ONLY IF the log
//   combines like this. Two lines per law; write them.
//
//   QUESTION(5d) — name a plausible C++ "log" type and combining
//   operation for which correct-looking plumbing still yields an
//   unlawful monad, and say which law dies. (Hint: there is a common
//   arithmetic type on every machine whose + is not associative.)
// ---------------------------------------------------------------------------
template <class T, class K>
auto mbind(LoggedBox<T> const& m, K&& k) {
    auto r = std::invoke(k, m.value);
    // FIXME(A): drops the continuation's log — see the discussion above.
    // (Braced-init evaluates left to right, and moving r.value does not
    // disturb r.log, so the repair can read both members of r safely.)
    return LoggedBox{std::move(r.value), m.log};
}

// ===========================================================================
// Tests — do not modify below this line. Each assert is annotated with the
// property it pins down (and the wrong implementation it catches).
// ===========================================================================
int main() {
    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        auto k_inc = [](int x) { return Box{x + 1}; };
        assert(mbind(Box{41}, k_inc) == Box{42});  // the contract

        // heterogeneous continuation — catches a result type wired to T
        auto k_show = [](int x) { return Box{std::to_string(x)}; };
        assert(mbind(Box{7}, k_show) == Box<std::string>{"7"});

        // dependent choice — degenerate here, real in step 4: the NEXT
        // box is produced from the value. (A reboxing bind would yield
        // Box<Box<...>> and fail to even compile against these compares.)
        auto pick = [](int x) {
            return x > 0 ? Box{std::string{"pos"}} : Box{std::string{"neg"}};
        };
        assert(mbind(Box{7}, pick) == Box<std::string>{"pos"});
        std::cout << "step 1  Box: mbind ............. ok\n";
    }
#else
    std::cout << "step 1  Box: mbind ............. TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        auto k_inc = [](int x) { return Box{x + 1}; };
        auto k_double = [](int x) { return Box{x * 2}; };

        assert(left_identity_law(box_pure, 5, k_inc));
        assert(right_identity_law(box_pure, Box{5}));
        assert(associativity_law(Box{5}, k_inc, k_double));

        // heterogeneous chain through associativity: int ~> string ~>
        // size_t — catches checkers quietly assuming one payload type
        auto k_show = [](int x) { return Box{std::to_string(x)}; };
        auto k_len = [](std::string const& s) { return Box{s.size()}; };
        assert(associativity_law(Box{12345}, k_show, k_len));
        std::cout << "step 2  three laws, executable . ok\n";
    }
#else
    std::cout << "step 2  three laws, executable . TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        auto k_inc = [](int x) { return Box{x + 1}; };
        auto k_double = [](int x) { return Box{x * 2}; };
        auto k_show = [](int x) { return Box{std::to_string(x)}; };

        // pure acts as kfish's identity — left and right, at sample points
        assert(kfish(box_pure, k_inc)(10) == k_inc(10));
        assert(kfish(k_inc, box_pure)(10) == k_inc(10));

        // composition associates, heterogeneously: int ~> int ~> string
        assert(kfish(kfish(k_inc, k_double), k_show)(5)
               == kfish(k_inc, kfish(k_double, k_show))(5));

        // and kfish means what it says — catches an argument-order swap
        assert(kfish(k_inc, k_double)(20) == Box{42});  // (20 + 1) * 2
        std::cout << "step 3  Kleisli category ....... ok\n";
    }
#else
    std::cout << "step 3  Kleisli category ....... TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        auto half_if_even = [](int x) {
            return x % 2 == 0 ? MaybeBox<int>{x / 2} : MaybeBox<int>{};
        };

        MaybeBox<int> twelve{12};

        // the cliffhanger, cashed: each stage is chosen by the last value
        assert(mbind(twelve, half_if_even) == MaybeBox<int>{6});
        assert(mbind(mbind(twelve, half_if_even), half_if_even)
               == MaybeBox<int>{3});
        assert(mbind(mbind(mbind(twelve, half_if_even), half_if_even),
                     half_if_even) == MaybeBox<int>{});  // 3 is odd: stop

        // short-circuit is REAL: k must never run on empty — convicts
        // any "recover with T{}" implementation on the spot
        auto boom = [](int) -> MaybeBox<int> { std::abort(); };
        assert(mbind(MaybeBox<int>{}, boom) == MaybeBox<int>{});

        // the three laws across presence patterns — step-2 checkers,
        // unchanged, zero new law code
        assert(left_identity_law(maybe_pure, 5, half_if_even));
        assert(right_identity_law(maybe_pure, twelve));
        assert(right_identity_law(maybe_pure, MaybeBox<int>{}));
        assert(associativity_law(twelve, half_if_even, half_if_even));
        assert(associativity_law(MaybeBox<int>{}, half_if_even, half_if_even));

        // join flattens one layer — all four shapes of nesting (note the
        // spelled types: Box{Box{7}} would COPY, not nest — see hints)
        assert(mjoin(Box<Box<int>>{Box{7}}) == Box{7});
        assert((mjoin(MaybeBox<MaybeBox<int>>{MaybeBox<int>{7}})
                == MaybeBox<int>{7}));
        assert((mjoin(MaybeBox<MaybeBox<int>>{}) == MaybeBox<int>{}));
        assert((mjoin(MaybeBox<MaybeBox<int>>{MaybeBox<int>{}})
                == MaybeBox<int>{}));

        // the bridge round-trips: bind = join after fmap
        auto k_inc = [](int x) { return Box{x + 1}; };
        assert(mbind_via_join(Box{41}, k_inc) == mbind(Box{41}, k_inc));
        assert(mbind_via_join(twelve, half_if_even)
               == mbind(twelve, half_if_even));
        std::cout << "step 4  MaybeBox + join bridge . ok\n";
    }
#else
    std::cout << "step 4  MaybeBox + join bridge . TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 ----------------------------------------------------------
#if STEP5_READY
    {
        auto k_log = [](int x) { return LoggedBox{x + 1, std::string{"k"}}; };
        auto h_log = [](int x) { return LoggedBox{x * 2, std::string{"h"}}; };

        // As shipped, exactly ONE of these three convicts — you PREDICTed
        // which. After the repair all three pass; EXPERIMENT(5b)'s mirror
        // defect moves the single conviction to a different line.
        assert((left_identity_law(logged_pure, 5, k_log)));
        assert((right_identity_law(logged_pure,
                                   LoggedBox{5, std::string{"m"}})));
        assert((associativity_law(LoggedBox{5, std::string{"m"}},
                                  k_log, h_log)));

        // concrete semantics: histories multiply in order, once each —
        // catches dropping and duplication alike
        assert((mbind(mbind(LoggedBox{1, std::string{"a"}}, k_log), h_log)
                == LoggedBox{4, std::string{"akh"}}));

        // Kleisli arrows carry their annotations through composition
        assert((kfish(k_log, h_log)(1) == LoggedBox{4, std::string{"kh"}}));

        // mbind_via_join checked on a third functor — join concatenates
        // the two log layers into one
        assert((mbind_via_join(LoggedBox{1, std::string{"a"}}, k_log)
                == mbind(LoggedBox{1, std::string{"a"}}, k_log)));
        std::cout << "step 5  LoggedBox reformed ..... ok\n";
    }
#else
    std::cout << "step 5  LoggedBox reformed ..... TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
