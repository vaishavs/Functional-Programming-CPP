// ===========================================================================
// The Box Model of Applicatives in C++ — TODO/FIXME exercise (detailed hints)
//
// One file, five steps, several kinds of work:
//   TODO(n)   — implement from scratch.
//   FIXME(n)  — the shipped code is wrong on purpose. Convict it with the
//               law checkers, then repair it.
//   PREDICT   — commit to an answer before compiling.
//   QUESTION  — answer in a comment next to the marker.
//   PROVE     — a short paper argument, written as a comment.
//
// After finishing a step, flip its STEPn_READY macro to 1 and rebuild.
// Steps depend on earlier ones — enable them in order. The file compiles
// and runs as shipped (all steps report TODO).
//
//   Build : g++ -std=c++20 -Wall -Wextra -o applicative box_applicative_todo.cpp
//   Run   : ./applicative
//
// Where this sits
//   The functor exercise gave each Box a morphism map, fmap, transporting
//   ONE plain function into the box. Applicative is the next rung: it lets
//   BOXED functions meet boxed arguments. The data, per functor F:
//     * pure : T -> F<T>                      (embed a value, effect-free)
//     * ap   : F<(T -> U)> x F<T> -> F<U>     (apply a boxed function)
//   subject to four laws (step 2 makes them executable):
//     identity       ap(pure(id), v)              ==  v
//     homomorphism   ap(pure(f), pure(x))         ==  pure(f(x))
//     interchange    ap(u, pure(y))               ==  ap(pure($y), u)
//                    where $y is "apply-to-y":  g |-> g(y)
//     composition    ap(ap(ap(pure(comp), u), v), w)
//                                                 ==  ap(u, ap(v, w))
//                    where comp is CURRIED composition: comp(f)(g)(x) = f(g(x))
//
//   Equivalently — and this file builds the bridge in step 3 — an
//   applicative is a lax monoidal endofunctor: a unit 1 -> F(1) and a
//   tensor zip : F<A> x F<B> -> F<pair<A, B>>, suitably coherent. The two
//   presentations reconstruct each other.
//
//   The effects reading: F<T> is "a T, plus an effect". Box adds nothing
//   (the Identity applicative); MaybeBox adds possible absence; LoggedBox
//   adds a monoid annotation (Writer). ap runs two effects and combines
//   them INDEPENDENTLY — neither effect can observe the other's value.
//   That independence is the entire character of applicative (and why an
//   Async applicative may run its effects in parallel). Step 4 closes on
//   the question this independence leaves open.
//
//   Honesty footnote, as before: every law here is checked UP TO
//   operator==. And a warning learned the hard way in the functor file:
//   some defects are invisible to the famous laws, and only the obscure
//   ones convict. Step 5 is built on that.
//
// Ground rules
//   * Standard library only.
//   * Value semantics throughout; pure and ap never mutate their inputs.
//   * Everything must compile clean under -Wall -Wextra.
//
// Hint layering per step: SPEC (the contract, pinned by the tests), HINTS
// (the mechanics), SHAPE (the skeleton, load-bearing parts elided as
// <...>). Stop reading as early as you can.
// ===========================================================================

#include <cassert>
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
// Given — carried over from the functor exercise, already lawful.
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
    return Box{std::invoke(f, b.value)};
}

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

// ---------------------------------------------------------------------------
// STEP 1 — the Identity applicative: pure and ap for Box
//
// SPEC
//   box_pure(x)          ==  Box{x}
//   ap(Box{f}, Box{x})   ==  Box{f(x)}
//   * Heterogeneous application must work: a boxed int -> string function
//     applied to a boxed int yields a boxed string.
//   * Inputs are taken by const&; nothing is mutated.
//
// HINTS
//   * ap is a free function template overloaded per functor, exactly like
//     fmap: its boxed arguments carry the functor's type, so unqualified
//     calls + ADL dispatch it from generic code. Nothing new there.
//   * pure is where C++ shows a seam — QUESTION(1): why can pure NOT be
//     an overload set the way fmap and ap are? Think before reading on.
//     Because pure's only parameter is a bare T: no argument names the
//     functor, C++ has no return-type-directed overload resolution, and
//     there is no compiler-inferred typeclass dictionary to consult. The
//     functor must be NAMED at every pure call. This file's convention:
//     each functor supplies its pure as a first-class callable (an inline
//     constexpr lambda), and generic code RECEIVES pure as an explicit
//     parameter. You will be hand-carrying the dictionary that Haskell
//     passes invisibly — watch how often it travels.
//   * box_pure: take x by value, move it in, let aggregate CTAD deduce
//     Box<decayed T> — the same route fmap took in the functor exercise.
//   * Box's ap: unwrap both, std::invoke the function payload on the
//     value payload (invoke, as always, so member pointers stay welcome),
//     rebox via CTAD. One line.
//
// SHAPE
//     inline constexpr auto box_pure = [](auto x) { return <...>; };
//
//     template <class Fn, class T>
//     auto ap(Box<Fn> const& ff, Box<T> const& fx) {
//         return Box{ <...> };
//     }
// ---------------------------------------------------------------------------
inline constexpr auto box_pure = [](auto x) {
    // TODO(1a)
};

template <class Fn, class T>
auto ap(Box<Fn> const& ff, Box<T> const& fx) {
    // TODO(1b)
}

// ---------------------------------------------------------------------------
// STEP 2 — the four laws, as executables
//
// SPEC
//   ccompose(f)(g)(x)                  ==  f(g(x))
//   ap_identity_law(pure, v)           ==  (ap(pure(id), v) == v)
//   ap_homomorphism_law(pure, f, x)    ==  (ap(pure(f), pure(x))
//                                             == pure(f(x)))
//   ap_interchange_law(pure, u, y)     ==  (ap(u, pure(y))
//                                             == ap(pure($y), u))
//   ap_composition_law(pure, u, v, w)  ==  (ap(ap(ap(pure(ccompose), u), v), w)
//                                             == ap(u, ap(v, w)))
//   * All checkers are generic over the functor: they take the functor's
//     pure as their first parameter (QUESTION(1) is why) and find ap by
//     unqualified call. They must work unchanged for Box now and for
//     MaybeBox and LoggedBox later.
//
// HINTS
//   * ccompose is CURRIED composition, one argument per application —
//     three nested unary lambdas, everything captured by value. You have
//     built exactly this before; do it from muscle memory. Why curried?
//     Because ap feeds a boxed function ONE boxed argument at a time:
//     every multi-argument function entering the boxed world must be
//     curried first. This is the moment currying stops being a party
//     trick.
//   * What each law is FOR — keep these one-liners next to the code:
//       identity      pure lifts id to the identity on F: pure adds no
//                     effect.
//       homomorphism  purely-pure computations collapse to pure of the
//                     plain result: the boxed world contains an exact
//                     copy of the unboxed one.
//       interchange   a pure argument commutes past an effectful
//                     function.
//       composition   boxed composition reassociates like plain
//                     composition: bracketing is invisible.
//     Together the four pin (pure, ap) down enough that the induced
//     functor  f |-> ap(pure(f), ·)  is lawful — step 3 checks its
//     agreement with fmap by machine.
//   * interchange's $y — "apply-to-y" — is a small lambda capturing y by
//     value and invoking its argument on it. Build it inside the checker.
//   * Foreshadowing, worth reading twice: step 5 ships a defect to which
//     identity and homomorphism are provably blind; only interchange and
//     composition convict. If the last two laws ever felt ceremonial,
//     they are about to earn their keep.
//   * std::invoke wherever a user-supplied callable is applied.
//
// SHAPE
//     inline constexpr auto ccompose = [](auto f) {
//         return [f](auto g) { return [f, g](auto x) { return <...>; }; };
//     };
//
//     template <class Pure, class B>
//     auto ap_identity_law(Pure pure, B const& v) {          // -> bool
//         return <...> == <...>;
//     }
//     template <class Pure, class Fn, class X>
//     auto ap_homomorphism_law(Pure pure, Fn f, X x) {       // -> bool
//         return <...> == <...>;
//     }
//     template <class Pure, class U, class Y>
//     auto ap_interchange_law(Pure pure, U const& u, Y y) {  // -> bool
//         auto apply_to = [ <...> ](auto const& g) { return <...>; };
//         return <...> == <...>;
//     }
//     template <class Pure, class U, class V, class W>
//     auto ap_composition_law(Pure pure, U const& u, V const& v,
//                             W const& w) {                  // -> bool
//         return <...> == <...>;
//     }
// ---------------------------------------------------------------------------
inline constexpr auto ccompose = [](auto f) {
    // TODO(2a)
};

template <class Pure, class B>
auto ap_identity_law(Pure pure, B const& v) {  // deduces bool once implemented
    // TODO(2b)
}

template <class Pure, class Fn, class X>
auto ap_homomorphism_law(Pure pure, Fn f, X x) {  // deduces bool once implemented
    // TODO(2c)
}

template <class Pure, class U, class Y>
auto ap_interchange_law(Pure pure, U const& u, Y y) {  // deduces bool once implemented
    // TODO(2d)
}

template <class Pure, class U, class V, class W>
auto ap_composition_law(Pure pure, U const& u, V const& v,
                        W const& w) {  // deduces bool once implemented
    // TODO(2e)
}

// ---------------------------------------------------------------------------
// STEP 3 — lift2, zip, and the lax monoidal bridge
//
// SPEC
//   lift2(pure, f, fa, fb)     applies a PLAIN binary f under the boxes:
//     lift2(box_pure, plus, Box{20}, Box{22}) == Box{42}
//   zip_boxes(pure, fa, fb)    is the monoidal tensor:
//     F<A> x F<B> -> F<std::pair<A, B>>
//   ap_via_zip(pure, ff, fx)   rebuilds ap from zip + fmap, and must
//     agree with ap on every functor in the file.
//   Also pinned by a test: the applicative's induced functor agrees with
//   the fmap you already had —  ap(pure(f), b) == fmap(f, b).
//
// HINTS
//   * lift2: ap applies one boxed argument at a time, so f must be
//     curried first. Rebuild curry2 inline: an outer lambda capturing f,
//     taking a, returning an inner lambda capturing f and a, taking b,
//     invoking f(a, b). Then
//       ap(ap(pure(curried_f), fa), fb)
//     — and note that the inner ap(pure(...), fa) is exactly the induced
//     fmap.
//   * zip_boxes: no new machinery — it IS lift2 of the pairing function
//     [](auto a, auto b) { return std::pair{ ... }; }. Move a and b into
//     the pair; CTAD names the pair type.
//   * ap_via_zip: zip the function box with the value box, then fmap the
//     evaluator p |-> invoke(p.first, p.second) over the result.
//   * Two presentations of one structure:
//       (pure, ap)     the programmer's applicative;
//       (unit, zip)    the lax monoidal functor — unit : 1 -> F(1) is
//                      pure of a unit value, zip is the tensor.
//     Each direction of the bridge is one line through the other's
//     vocabulary; the round trip must land where it started (the test
//     compares ap_via_zip against ap directly).
//   * QUESTION(3): match each of the four ap laws to the monoidal
//     coherence datum it shadows — left unitor, right unitor, associator,
//     naturality of the tensor. One line per law. The exact dictionary is
//     subtle and certainty is not required; the mapping intuition is the
//     exercise.
//   * PROVE(3), two lines: using homomorphism alone, show that
//     ap(pure(f), pure(x)) and fmap(f, pure(x)) must agree for a lawful
//     instance whose fmap is the induced one. The test then checks the
//     agreement numerically on a non-pure box too.
//
// SHAPE
//     template <class Pure, class F2, class FA, class FB>
//     auto lift2(Pure pure, F2 f, FA const& fa, FB const& fb) {
//         auto cf = [ <...> ](auto a) {
//             return [ <...> ](auto b) { return <...>; };
//         };
//         return <...>;
//     }
//
//     template <class Pure, class FA, class FB>
//     auto zip_boxes(Pure pure, FA const& fa, FB const& fb) {
//         return lift2( <...> );
//     }
//
//     template <class Pure, class FF, class FX>
//     auto ap_via_zip(Pure pure, FF const& ff, FX const& fx) {
//         return fmap( <...>, zip_boxes( <...> ));
//     }
// ---------------------------------------------------------------------------
template <class Pure, class F2, class FA, class FB>
auto lift2(Pure pure, F2 f, FA const& fa, FB const& fb) {
    // TODO(3a)
}

template <class Pure, class FA, class FB>
auto zip_boxes(Pure pure, FA const& fa, FB const& fb) {
    // TODO(3b)
}

template <class Pure, class FF, class FX>
auto ap_via_zip(Pure pure, FF const& ff, FX const& fx) {
    // TODO(3c)
}

// ---------------------------------------------------------------------------
// STEP 4 — MaybeBox: the applicative of possible absence
//
// SPEC
//   maybe_pure(x)  ==  MaybeBox<decayed T>{x}      (present, no ceremony)
//   ap: the result is present iff BOTH the function and the argument
//   are; otherwise it is the empty box AT THE MAPPED TYPE.
//   * All four step-2 checkers must pass across presence patterns —
//     without one new line of law code.
//   * lift2 must behave: both present -> combined; any absent -> absent.
//
// HINTS
//   * maybe_pure cannot lean on aggregate CTAD the way box_pure did:
//     MaybeBox's member is std::optional<T>, and aggregate deduction
//     guides do not invert optional's converting constructor — there is
//     no deducing T from a bare int through optional<T>. Spell the type:
//       MaybeBox<std::decay_t<decltype(x)>>{std::move(x)}
//     A small asymmetry worth remembering: CTAD sees through members OF
//     the deduced type itself, not through members that merely convert.
//   * ap: the same both-branches-must-name-one-type discipline as
//     MaybeBox's fmap. Compute
//       using U = std::remove_cvref_t<std::invoke_result_t<Fn const&, T const&>>;
//     once; return MaybeBox<U>{ ... } when both slots are engaged,
//     MaybeBox<U>{} otherwise. Note which Fn the trait mentions: the
//     STORED function type — the boxed function is data here, and the
//     trait mirrors invoking it out of a const box.
//   * Absence is an effect, and ap combines effects: an absent FUNCTION
//     annihilates just as thoroughly as an absent argument. The tests pin
//     both directions.
//   * QUESTION(4) — the door applicative cannot open. lift2 consults both
//     boxes no matter what; the SHAPE of the computation is fixed before
//     any value is seen. Write, in a comment (signature only, no code),
//     the operation you would need for the SECOND box to be CHOSEN by
//     the value inside the first — and name the structure that provides
//     it. That structure is the next exercise. Flip side, worth stating
//     in your answer: the rigidity you just bumped into is exactly why
//     applicative effects are statically analysable — and why an Async
//     functor may run both effects in parallel.
//
// SHAPE
//     inline constexpr auto maybe_pure = [](auto x) {
//         return MaybeBox< <...> >{ <...> };
//     };
//
//     template <class Fn, class T>
//     auto ap(MaybeBox<Fn> const& ff, MaybeBox<T> const& fx) {
//         using U = <...>;
//         if ( <...> ) return MaybeBox<U>{ <...> };
//         return MaybeBox<U>{};
//     }
// ---------------------------------------------------------------------------
inline constexpr auto maybe_pure = [](auto x) {
    // TODO(4a)
};

template <class Fn, class T>
auto ap(MaybeBox<Fn> const& ff, MaybeBox<T> const& fx) {
    // TODO(4b)
}

// ---------------------------------------------------------------------------
// STEP 5 — LoggedBox: the Writer applicative, shipped with two defects
//
// LoggedBox annotates a value with a std::string log; the annotation
// monoid is ("", +). The lawful instance: pure attaches the EMPTY log
// (the monoid unit), and ap appends the function box's log, then the
// argument box's log, EACH EXACTLY ONCE:
//     value :  ff.value applied to fx.value
//     log   :  ff.log + fx.log
// The functor layer below is given and lawful — fmap transports the
// annotation untouched.  pure and ap ship broken.
//
// SPEC (post-repair)
//   All four checkers pass; lift2 concatenates logs left-to-right, once
//   each; ap_via_zip agrees with ap — the step-3 bridge cashing its
//   genericity cheque on a third functor.
//
// The bug hunt — read before flipping STEP5_READY
//   Two INDEPENDENT defects sit in the shipped code below. The tests are
//   ordered as two acts:
//     Act 1: identity and homomorphism run first. PREDICT their verdicts
//       on the shipped code. Any conviction here is FIXME(A)'s doing —
//       and here is the claim to verify on paper, QUESTION(5a): identity
//       and homomorphism are provably BLIND to FIXME(B). (Look at whose
//       log FIXME(B) mangles, and what those two laws ever put in that
//       position.)
//     Act 2: fix (A) alone, re-run. Identity and homomorphism go green —
//       and interchange convicts. PREDICT, before fixing (B): which half
//       of the four-law verdict sheet flips when (B) is repaired, and
//       why? You met this phenomenon in the functor exercise (EagerBox:
//       the famous law passes, another convicts). Same lesson, one rung
//       up: the laws are independent witnesses, and some defects are
//       visible only to the unglamorous ones.
//
//   FIXME(A) — pure stamps provenance. "Knowing a value was born pure is
//     useful for the dashboard," said the author, attaching "pure " to
//     every pure's log. But the unit of the annotation monoid is
//     load-bearing: pure must be EFFECT-FREE, and here effect-free means
//     the empty log. Nothing less is the unit; nothing else is lawful.
//   FIXME(B) — ap duplicates the function box's log. "The function
//     side's provenance kept getting lost in long chains, so it is
//     recorded twice to make sure it survives." Each log, once. Effect
//     ARITHMETIC — unit, single occurrence, associativity — is the law.
//   QUESTION(5b), answer after both repairs: swapping the concatenation
//     to fx.log + ff.log would NOT be a bug — that is the Dual monoid /
//     Backwards instance, a DIFFERENT lawful applicative sequencing
//     effects right-to-left. Effect ORDER is a design choice; effect
//     arithmetic is not. Say in one line how you would detect, from
//     outside, which of the two lawful orders a given instance uses.
// ---------------------------------------------------------------------------
template <class T>
struct LoggedBox {
    T value;
    std::string log;  // annotation monoid: unit "", operation +
    friend bool operator==(LoggedBox const&, LoggedBox const&) = default;
};

// Given — the (lawful) functor layer: the annotation rides along untouched.
template <class F, class T>
auto fmap(F&& f, LoggedBox<T> const& b) {
    return LoggedBox{std::invoke(f, b.value), b.log};
}

inline constexpr auto logged_pure = [](auto x) {
    // FIXME(A): provenance is not an effect pure is allowed to have.
    return LoggedBox{std::move(x), std::string{"pure "}};
};

template <class Fn, class T>
auto ap(LoggedBox<Fn> const& ff, LoggedBox<T> const& fx) {
    // FIXME(B): "recorded twice to make sure it survives."
    return LoggedBox{std::invoke(ff.value, fx.value),
                     ff.log + ff.log + fx.log};
}

// ===========================================================================
// Tests — do not modify below this line. Each assert is annotated with the
// property it pins down (and the wrong implementation it catches).
// ===========================================================================
int main() {
    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        auto plus1 = [](int x) { return x + 1; };
        auto times2 = [](int x) { return x * 2; };

        assert(box_pure(41) == Box{41});             // pure embeds, no ceremony
        assert(ap(Box{plus1}, Box{41}) == Box{42});  // the contract

        // heterogeneous: boxed int -> string meets boxed int — catches a
        // result type hard-wired to the argument's type
        auto show = [](int x) { return std::to_string(x); };
        assert(ap(Box{show}, Box{7}) == Box<std::string>{"7"});

        // the boxed function is data: a different function, same shape
        assert(ap(Box{times2}, Box{21}) == Box{42});
        std::cout << "step 1  Box: pure + ap ......... ok\n";
    }
#else
    std::cout << "step 1  Box: pure + ap ......... TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        auto plus1 = [](int x) { return x + 1; };
        auto times2 = [](int x) { return x * 2; };

        assert(ccompose(plus1)(times2)(10) == 21);   // f after g, curried

        assert(ap_identity_law(box_pure, Box{5}));
        assert(ap_homomorphism_law(box_pure, plus1, 5));
        assert(ap_interchange_law(box_pure, Box{plus1}, 9));
        assert(ap_composition_law(box_pure, Box{plus1}, Box{times2}, Box{10}));

        // heterogeneous through the composition law: u : F<string->size_t>,
        // v : F<int->string> — catches checkers quietly assuming one type
        auto show = [](int x) { return std::to_string(x); };
        auto len = [](std::string const& s) { return s.size(); };
        assert(ap_composition_law(box_pure, Box{len}, Box{show}, Box{12345}));
        std::cout << "step 2  four laws, executable .. ok\n";
    }
#else
    std::cout << "step 2  four laws, executable .. TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        auto plus1 = [](int x) { return x + 1; };

        // the applicative's induced functor agrees with fmap — PROVE(3)'s
        // numeric shadow, checked on a non-pure box
        assert(ap(box_pure(plus1), Box{41}) == fmap(plus1, Box{41}));

        assert(lift2(box_pure, std::plus<>{}, Box{20}, Box{22}) == Box{42});

        // the tensor — catches pairing that drops or reorders components
        assert((zip_boxes(box_pure, Box{1}, Box{std::string{"a"}})
                == Box{std::pair{1, std::string{"a"}}}));

        // around the loop: ap -> zip -> ap lands where it started
        assert(ap_via_zip(box_pure, Box{plus1}, Box{41})
               == ap(Box{plus1}, Box{41}));
        std::cout << "step 3  lift2 / zip bridge ..... ok\n";
    }
#else
    std::cout << "step 3  lift2 / zip bridge ..... TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        auto plus1 = [](int x) { return x + 1; };
        auto times2 = [](int x) { return x * 2; };

        MaybeBox<int> full{20};
        MaybeBox<int> none{};

        assert(maybe_pure(5) == MaybeBox<int>{5});
        assert(ap(maybe_pure(plus1), full) == MaybeBox<int>{21});

        // absence annihilates from EITHER side — catches an ap that only
        // checks the argument slot
        assert(ap(MaybeBox<decltype(plus1)>{}, full) == MaybeBox<int>{});
        assert(ap(maybe_pure(plus1), MaybeBox<int>{}) == MaybeBox<int>{});

        // all four laws, free of charge, across presence patterns — the
        // step-2 checkers, unchanged
        assert(ap_identity_law(maybe_pure, full));
        assert(ap_identity_law(maybe_pure, none));
        assert(ap_homomorphism_law(maybe_pure, plus1, 5));
        assert(ap_interchange_law(maybe_pure, maybe_pure(plus1), 9));
        assert(ap_interchange_law(maybe_pure, MaybeBox<decltype(plus1)>{}, 9));
        assert(ap_composition_law(maybe_pure, maybe_pure(plus1),
                                  MaybeBox<decltype(times2)>{}, full));

        // the workhorse: independent effects, combined
        assert(lift2(maybe_pure, std::plus<>{}, MaybeBox<int>{20},
                     MaybeBox<int>{22}) == MaybeBox<int>{42});
        assert(lift2(maybe_pure, std::plus<>{}, MaybeBox<int>{},
                     MaybeBox<int>{22}) == MaybeBox<int>{});
        std::cout << "step 4  MaybeBox applicative ... ok\n";
    }
#else
    std::cout << "step 4  MaybeBox applicative ... TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 ----------------------------------------------------------
#if STEP5_READY
    {
        auto plus1 = [](int x) { return x + 1; };
        auto times2 = [](int x) { return x * 2; };

        // Act 1 — any conviction here is FIXME(A)'s: these two laws are
        // blind to FIXME(B) (you argued why at QUESTION(5a))
        assert((ap_identity_law(logged_pure, LoggedBox{5, std::string{"v"}})));
        assert(ap_homomorphism_law(logged_pure, plus1, 5));

        // Act 2 — green above, yet still convicting here until FIXME(B)
        // is repaired: the unglamorous laws earn their keep
        assert((ap_interchange_law(logged_pure,
                                   LoggedBox{plus1, std::string{"u"}}, 9)));
        assert((ap_composition_law(logged_pure,
                                   LoggedBox{plus1, std::string{"a"}},
                                   LoggedBox{times2, std::string{"b"}},
                                   LoggedBox{10, std::string{"c"}})));

        // concrete semantics: effects sequence left-to-right, once each —
        // catches duplication and reordering alike
        assert((lift2(logged_pure, std::plus<>{},
                      LoggedBox{1, std::string{"L"}},
                      LoggedBox{2, std::string{"R"}})
                == LoggedBox{3, std::string{"LR"}}));

        // the step-3 bridge, cashing its genericity cheque on functor #3
        assert((ap_via_zip(logged_pure, LoggedBox{plus1, std::string{"f"}},
                           LoggedBox{41, std::string{"x"}})
                == ap(LoggedBox{plus1, std::string{"f"}},
                      LoggedBox{41, std::string{"x"}})));
        std::cout << "step 5  LoggedBox reformed ..... ok\n";
    }
#else
    std::cout << "step 5  LoggedBox reformed ..... TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
