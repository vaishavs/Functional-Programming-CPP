// ===========================================================================
// The Box Model of Functors in C++ — TODO/FIXME exercise
//
// One file, five steps, several kinds of work:
//   TODO(n)   — implement from scratch.
//   FIXME(n)  — the shipped code is wrong on purpose. Convict it with the
//               law checkers, then repair it.
//   PREDICT   — commit to an answer before compiling.
//   QUESTION  — answer in a comment next to the marker.
//   PROVE     — write a short paper-proof as a comment.
//
// After finishing a step, flip its STEPn_READY macro to 1 and rebuild.
// Steps depend on earlier ones — enable them in order. The file compiles
// and runs as shipped (all steps report TODO).
//
//   Build : g++ -std=c++20 -Wall -Wextra -o functor box_functor_todo.cpp
//   Run   : ./functor
//
// The contract
//   A functor F needs a type constructor T |-> F<T> and
//     fmap : (T -> U) x F<T> -> F<U>
//   satisfying two laws, checked UP TO operator==:
//     identity        fmap(id, b)             ==  b
//     composition     fmap(compose(g, f), b)  ==  fmap(g, fmap(f, b))
//
// Ground rules
//   * Standard library only.
//   * Value semantics throughout: fmap never mutates its input box.
//   * Everything must compile clean under -Wall -Wextra.
//
// Hint layering per step: SPEC (the contract, pinned by the tests), HINTS
// (the mechanics), SHAPE (the skeleton with load-bearing expressions
// elided as <...>). Stop reading as early as you can.
// ===========================================================================

#include <cassert>
#include <cstddef>
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
// Given: id, used as fmap's identity throughout.
// decltype(auto) forwards the argument through unchanged — no decay, no
// copy — which is what the identity law needs.
// ---------------------------------------------------------------------------
inline constexpr auto id = [](auto&& x) -> decltype(auto) {
    return std::forward<decltype(x)>(x);
};

// ---------------------------------------------------------------------------
// STEP 1 — Box<T>: the smallest functor
//
// SPEC
//   fmap(f, Box<T>{v}) == Box<U>{f(v)}, where U is the decayed result of f.
//   * Heterogeneous mapping must work (int -> std::string): the object map
//     really sends T to U.
//   * The input box is never modified; the payload reaches f as a const
//     lvalue.
//   * Pointers to members are morphisms too (the P::len test).
//
// HINTS
//   * fmap is a free function template, overloaded once per box type.
//     Call it unqualified — ordinary lookup + ADL finds the right
//     overload even from generic code written earlier in the file (step
//     5 leans hard on this).
//   * Apply f with std::invoke(f, b.value). Plain call syntax f(b.value)
//     cannot handle pointers to members; invoke can.
//   * The result type — two routes:
//       (a) C++20 aggregate CTAD:  return Box{std::invoke(f, b.value)};
//           deduces the member type BY VALUE from the initializer, so
//           the result decays automatically.
//       (b) Spell it out:
//             Box<std::remove_cvref_t<std::invoke_result_t<F&, T const&>>>
//           Mirror the actual call: f is an lvalue inside fmap (hence
//           F&), and b.value is a const lvalue (hence T const&).
//   * Resist decoration. fmap applies f to the payload and rebuilds the
//     box — nothing else (no counting, logging, normalising, special-
//     casing).
//
// SHAPE
//     template <class F, class T>
//     auto fmap(F&& f, Box<T> const& b) {
//         return Box{ <...> };
//     }
// ---------------------------------------------------------------------------
template <class T>
struct Box {
    T value;
    friend bool operator==(Box const&, Box const&) = default;
};

template <class F, class T>
auto fmap(F&& f, Box<T> const& b) {
    // TODO(1)
}

// ---------------------------------------------------------------------------
// STEP 2 — the laws, as executables
//
// SPEC
//   compose(g, f)(x)          ==  g(f(x))                       (g after f)
//   identity_law(b)           ==  (fmap(id, b) == b)
//   composition_law(g, f, b)  ==  (fmap(compose(g, f), b)
//                                    == fmap(g, fmap(f, b)))
//   * All three must work unchanged for EVERY box type in this file —
//     Box now; TracedBox, EagerBox, MaybeBox later.
//
// HINTS
//   * compose: capture g and f by value; return a generic lambda applying
//     f, then g. Give it a reference-preserving return type
//     (-> decltype(auto)).
//   * identity_law is one line: compare fmap(id, b) to b with ==.
//   * composition_law: build both sides, compare. Argument convention:
//     composition_law(g, f, b) applies g AFTER f — matching compose(g, f).
//   * Call fmap unqualified inside the checkers; each box type's overload
//     is found at instantiation via argument-dependent lookup — including
//     overloads declared later in the file.
//   * Write both checkers even though later steps will show cases where
//     one law holds and the other doesn't.
//
// SHAPE
//     template <class G, class F2>
//     auto compose(G g, F2 f) {
//         return [ <...> ](auto&& x) -> <...> { return <...>; };
//     }
//
//     template <class B>
//     auto identity_law(B const& b) { return <...>; }          // -> bool
//
//     template <class G, class F2, class B>
//     auto composition_law(G g, F2 f, B const& b) {            // -> bool
//         return <...> == <...>;
//     }
// ---------------------------------------------------------------------------
template <class G, class F2>
auto compose(G g, F2 f) {
    // TODO(2a)
}

template <class B>
auto identity_law(B const& b) {  // deduces bool once implemented
    // TODO(2b)
}

template <class G, class F2, class B>
auto composition_law(G g, F2 f, B const& b) {  // deduces bool once implemented
    // TODO(2c)
}

// ---------------------------------------------------------------------------
// STEP 3 — two impostors. Convict, then rehabilitate.
//
// SPEC
//   After your fixes, both box types below must pass BOTH generic law
//   checkers, and the concrete value asserts in the tests must hold.
//   Before fixing anything, flip STEP3_READY and watch the checkers
//   convict — but commit to your PREDICTions first. (Requires step 2.)
//
// ...........................................................................
// FIXME(A) — TracedBox: an fmap that counts its own calls
//
// fmap is instrumented with a hop counter that increments on every call,
// so a dashboard can show how many transformations a value has been
// through.
//
// PREDICT before enabling the tests: which checkers convict TracedBox —
// identity, composition, or both? (Careful: BOTH sides of the composition
// check carry counters too.)
//
//   ---- spoiler fold: predict first, read on after -----------------------
//
//   The repair. Two legitimate routes — pick ONE, implement it, and defend
//   the choice in a comment at QUESTION(A):
//     Route 1 (purify): drop the increment (and arguably the field
//     itself). fmap(id, b) is then indistinguishable from b again.
//     Route 2 (quotient): keep hops as non-semantic bookkeeping, but
//     replace the defaulted operator== with one comparing value alone, so
//     hops can never affect a law check or a test.
//     Not legitimate: keeping hops both incremented and ==-observable.
//   The tests are route-agnostic (they check .value and the laws, never
//   hops), so either repair goes green.
// ...........................................................................
template <class T>
struct TracedBox {
    T value;
    int hops = 0;  // how many fmaps this value has survived
    friend bool operator==(TracedBox const&, TracedBox const&) = default;
};

template <class F, class T>
auto fmap(F&& f, TracedBox<T> const& b) {
    // FIXME(A): see the discussion above, and QUESTION(A): which route,
    // and what does it commit you to?
    return TracedBox{std::invoke(f, b.value), b.hops + 1};
}

// ...........................................................................
// FIXME(B) — EagerBox: the optimisation that wasn't
//
// When f maps a type to itself (an endomorphism), fmap applies f twice
// instead of once, on the theory that "it exercises the cache better".
//
// PREDICT before enabling the tests — this one is sneakier than (A). Work
// it on paper first, with f = times2, g = plus1, starting value 1:
//     fmap(id, EagerBox{1})                  = ?
//     fmap(compose(g, f), EagerBox{1})       = ?      (left side of the law)
//     fmap(g, fmap(f, EagerBox{1}))          = ?      (right side)
//
//   ---- spoiler fold: predict first, read on after -----------------------
//
//   fmap(id, ...) double-applies id — but id . id == id, so the identity
//   law PASSES. The composition law does not: the left side applies
//   (g . f) twice — g f g f, value 7 — while the right side applies f
//   twice then g twice — g g f f, value 6. Only one checker catches this,
//   which is why step 2 made you write both.
//
//   The repair: delete the branch. A lawful fmap must not care what T and
//   U are. Then answer QUESTION(B): name two other C++ mechanisms (besides
//   if constexpr + is_same) that could smuggle this same violation past a
//   reader.
// ...........................................................................
template <class T>
struct EagerBox {
    T value;
    friend bool operator==(EagerBox const&, EagerBox const&) = default;
};

template <class F, class T>
auto fmap(F&& f, EagerBox<T> const& b) {
    using U = std::remove_cvref_t<std::invoke_result_t<F&, T const&>>;
    if constexpr (std::is_same_v<T, U>) {
        // FIXME(B): endomorphisms get "one extra for the cache".
        return EagerBox<U>{std::invoke(f, std::invoke(f, b.value))};
    } else {
        return EagerBox<U>{std::invoke(f, b.value)};
    }
}

// ---------------------------------------------------------------------------
// STEP 4 — MaybeBox: a functor with actual structure
//
// SPEC
//   MaybeBox wraps std::optional<T>: either empty or holding a value.
//   * fmap applies f to the contents IF PRESENT; an empty box maps to an
//     empty box AT THE MAPPED TYPE:
//       fmap(show, MaybeBox<int>{}) == MaybeBox<std::string>{}
//   * Both step-2 checkers must pass for empty and full boxes, WITHOUT
//     writing a single new line of law code.
//
// HINTS
//   * fmap acts on contents only and leaves the empty/engaged shape
//     untouched.
//   * Compute the target type once:
//       using U = std::remove_cvref_t<std::invoke_result_t<F&, T const&>>;
//     Both branches must name the same return type, so aggregate CTAD
//     alone cannot carry you here — the empty branch has nothing to
//     deduce from. Spell MaybeBox<U> in both.
//   * The wrong turn, rhyming with FIXME(A): "helpfully" producing
//     f(T{}) for the empty case manufactures a value out of structure.
//     fmap(id, empty) would no longer be empty, breaking the identity law.
//   * Returning MaybeBox<T>{} from the empty branch will not even compile
//     once f is heterogeneous — the type mismatch catches the mistake at
//     compile time.
//   * std::optional already compares correctly (empty == empty; engaged
//     values compare through), so the defaulted operator== lifts right.
//
// SHAPE
//     template <class F, class T>
//     auto fmap(F&& f, MaybeBox<T> const& b) {
//         using U = <...>;
//         if ( <...> ) return MaybeBox<U>{ <...> };
//         return MaybeBox<U>{};
//     }
// ---------------------------------------------------------------------------
template <class T>
struct MaybeBox {
    std::optional<T> slot;
    friend bool operator==(MaybeBox const&, MaybeBox const&) = default;
};

template <class F, class T>
auto fmap(F&& f, MaybeBox<T> const& b) {
    // TODO(4)
}

// ---------------------------------------------------------------------------
// STEP 5 — lift, and the composition of functors
//
// SPEC
//   lift(f) is fmap with the morphism pre-applied:
//     lift(f)(box) == fmap(f, box)          for EVERY box type in the file
//   Consequently lift(lift(f)) maps through two layers:
//     lift(lift(f)) applied to a Box<MaybeBox<T>> touches the innermost T.
//   The tests spell out both laws by hand for this nested case; PROVE(5)
//   asks for the general argument as a comment.
//
// HINTS
//   * Why lift needs to exist: fmap here is an OVERLOAD SET of function
//     templates, not a value — it cannot be captured, stored, or passed
//     around. lift wraps the unqualified call in a generic lambda,
//     turning it back into a first-class callable.
//   * Inside lift, call fmap UNQUALIFIED. The call is dependent, so
//     candidates are gathered at instantiation via argument-dependent
//     lookup: one lift serves every functor in the file, including any
//     defined after lift itself.
//   * Capture f by value; take the box by const&.
//   * PROVE(5), as a comment, two lines per law — show that nesting two
//     lifts still satisfies identity and composition:
//       identity:     lift(lift(id)) == lift(id)   [ G's identity law ]
//                                    == id         [ F's identity law ]
//       composition:  analogous — use each functor's composition law
//                     exactly once.
//
// SHAPE
//     template <class F>
//     auto lift(F f) {
//         return [ <...> ](auto const& box) { return <...>; };
//     }
// ---------------------------------------------------------------------------
template <class F>
auto lift(F f) {
    // TODO(5)
}

// ===========================================================================
// Tests — do not modify below this line. Each assert is annotated with the
// property it pins down (and the wrong implementation it catches).
// ===========================================================================
int main() {
    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        assert(fmap([](int x) { return x + 1; }, Box{41}) == Box{42});  // contract

        // heterogeneous: the object map really sends int to std::string —
        // catches a Box<T> return type hard-wired to the input type
        assert(fmap([](int x) { return std::to_string(x); }, Box{7})
               == Box<std::string>{"7"});

        // pointers to members are morphisms too — catches plain call
        // syntax in place of std::invoke
        struct P {
            int len;
        };
        assert(fmap(&P::len, Box{P{5}}) == Box{5});

        // morphisms may take their argument by const& — catches result-
        // type computations that trip over reference-returning invokes
        auto size = [](std::string const& s) { return s.size(); };
        assert(fmap(size, Box<std::string>{"functor"}) == Box<std::size_t>{7});
        std::cout << "step 1  Box + fmap ............. ok\n";
    }
#else
    std::cout << "step 1  Box + fmap ............. TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        auto times2 = [](int x) { return x * 2; };
        auto plus1 = [](int x) { return x + 1; };

        assert(compose(plus1, times2)(10) == 21);  // g AFTER f — order pinned

        assert(identity_law(Box{5}));
        assert(composition_law(plus1, times2, Box{5}));

        // heterogeneous composition: show : int -> string, len : string -> size_t
        auto show = [](int x) { return std::to_string(x); };
        auto len = [](std::string const& s) { return s.size(); };
        assert(compose(len, show)(12345) == 5u);
        assert(composition_law(len, show, Box{12345}));
        std::cout << "step 2  laws as executables .... ok\n";
    }
#else
    std::cout << "step 2  laws as executables .... TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        auto times2 = [](int x) { return x * 2; };
        auto plus1 = [](int x) { return x + 1; };

        // FIXME(A) — as shipped, the checkers convict TracedBox. Predict
        // the verdict split before running; after your chosen route, both
        // must acquit — and the .value check keeps both routes honest.
        assert(identity_law(TracedBox<int>{5}));
        assert(composition_law(plus1, times2, TracedBox<int>{5}));
        assert(fmap(times2, TracedBox<int>{21}).value == 42);

        // FIXME(B) — identity passes even as shipped (you predicted why)...
        assert(identity_law(EagerBox<int>{1}));
        // ...which is exactly why a separate composition checker earns its
        // keep: 7 on the left, 6 on the right, until the branch is gone.
        assert(composition_law(plus1, times2, EagerBox<int>{1}));
        std::cout << "step 3  impostors reformed ..... ok\n";
    }
#else
    std::cout << "step 3  impostors reformed ..... TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        auto times2 = [](int x) { return x * 2; };
        auto plus1 = [](int x) { return x + 1; };
        auto show = [](int x) { return std::to_string(x); };

        MaybeBox<int> full{42};
        MaybeBox<int> none{};

        assert(fmap(plus1, full) == MaybeBox<int>{43});
        assert(fmap(plus1, none) == MaybeBox<int>{});          // shape preserved...
        assert(fmap(show, none) == MaybeBox<std::string>{});   // ...at the mapped type

        // zero new law code: the step-2 checkers, unchanged, empty and full
        assert(identity_law(full) && identity_law(none));
        assert(composition_law(plus1, times2, full));
        assert(composition_law(plus1, times2, none));
        std::cout << "step 4  MaybeBox ............... ok\n";
    }
#else
    std::cout << "step 4  MaybeBox ............... TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 ----------------------------------------------------------
#if STEP5_READY
    {
        auto times2 = [](int x) { return x * 2; };
        auto plus1 = [](int x) { return x + 1; };

        // lift restores first-class status to the overload set...
        assert(lift(plus1)(Box{41}) == Box{42});
        // ...and one lift serves every functor (ADL at work)
        assert(lift(plus1)(MaybeBox<int>{1}) == MaybeBox<int>{2});

        Box<MaybeBox<int>> nested{MaybeBox<int>{20}};
        Box<MaybeBox<int>> hollow{MaybeBox<int>{}};

        assert(lift(lift(plus1))(nested) == Box{MaybeBox<int>{21}});
        assert(lift(lift(plus1))(hollow) == hollow);  // inner shape survives both layers

        // both functor laws, checked by hand on the nested case (see
        // PROVE(5))
        assert(lift(lift(id))(nested) == nested);
        assert(lift(lift(compose(plus1, times2)))(nested)
               == lift(lift(plus1))(lift(lift(times2))(nested)));
        std::cout << "step 5  functors compose ....... ok\n";
    }
#else
    std::cout << "step 5  functors compose ....... TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
