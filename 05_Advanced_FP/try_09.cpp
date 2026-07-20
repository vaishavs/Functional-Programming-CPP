// ===========================================================================
// The Box Model of Functors in C++ — TODO/FIXME exercise (detailed hints)
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
// The category in play
//   Objects are C++ types; morphisms are const-invocable callables T -> U;
//   composition is compose(g, f) (step 2); the identity morphism is id
//   (given below). A functor F consists of
//     * an object map   — a type constructor:   T        |->  F<T>
//     * a morphism map  — fmap:                 (T -> U) |->  (F<T> -> F<U>)
//   subject to two laws:
//     identity        fmap(id, b)             ==  b
//     composition     fmap(compose(g, f), b)  ==  fmap(g, fmap(f, b))
//   The laws are not decoration; they are the entire content of the word
//   "functor". An fmap without them is just a function with a hopeful name.
//
//   Honesty footnote: everything here is checked UP TO operator==. The
//   ambient category's notion of equality is whatever == says — a point
//   that stops being pedantic the moment you reach FIXME(A).
//
// Ground rules
//   * Standard library only.
//   * Value semantics throughout: fmap never mutates its input box.
//   * Everything must compile clean under -Wall -Wextra.
//
// Hint layering per step, as before: SPEC (the contract, pinned by the
// tests), HINTS (the mechanics), SHAPE (the skeleton with load-bearing
// expressions elided as <...>). Stop reading as early as you can.
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
// Given: the identity morphism.
// A forwarding identity: returns exactly what it was handed, reference and
// all. The decltype(auto) matters — a decaying id would still satisfy the
// laws up to ==, but this one is the honest identity morphism (and copies
// nothing on the way through).
// ---------------------------------------------------------------------------
inline constexpr auto id = [](auto&& x) -> decltype(auto) {
    return std::forward<decltype(x)>(x);
};

// ---------------------------------------------------------------------------
// STEP 1 — Box<T>: the smallest possible functor (the Identity functor)
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
//   * fmap is deliberately a free function template, overloaded once per
//     functor. Overload resolution on the second parameter is C++'s
//     stand-in for a typeclass method — and unqualified calls resolved
//     through ordinary lookup + ADL will find the right overload even
//     from generic code written earlier in the file (step 5 leans hard
//     on this).
//   * Apply f with std::invoke(f, b.value). Call syntax f(b.value) cannot
//     handle pointers to members; invoke implements the full INVOKE
//     protocol — same reason as in the currying exercise's partial.
//   * The result type — two routes:
//       (a) C++20 aggregate CTAD:  return Box{std::invoke(f, b.value)};
//           Aggregate deduction guides deduce the member type BY VALUE
//           from the initializer, i.e. the result decays (cv-qualifiers
//           and references stripped) — exactly the value semantics this
//           file wants.
//       (b) Spell it out:
//             Box<std::remove_cvref_t<std::invoke_result_t<F&, T const&>>>
//           Note how the trait mirrors the actual call: f is an lvalue
//           inside fmap (hence F&), and b.value is a const lvalue (hence
//           T const&). Traits that mirror the call, letter for letter —
//           the same discipline the currying exercise drilled.
//   * Resist decoration. fmap applies f to the payload and rebuilds the
//     box; anything else it does — counting, logging, normalising,
//     special-casing — is a law violation queuing up for step 3.
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
//   * All three are generic: they must work unchanged for EVERY box type
//     in this file — Box now; TracedBox, EagerBox, MaybeBox later. That
//     genericity is the payoff of fmap-as-overload-set, and it is what
//     step 4 will quietly test.
//
// HINTS
//   * compose: capture g and f by value (value semantics, as everywhere);
//     return a generic lambda applying f, then g. Give it a
//     reference-preserving return type (-> decltype(auto)) so that the
//     composite of reference-preserving morphisms stays one — the tests
//     do not insist, the discipline does.
//   * identity_law is one line. The comparison is the point: the law is
//     checked UP TO the box's operator==. This is not a proof about
//     mathematical objects; it is membership in the equivalence that ==
//     induces. File that thought under FIXME(A).
//   * composition_law: build both sides, compare. Argument convention:
//     composition_law(g, f, b) applies g AFTER f — matching compose(g, f).
//   * Call fmap unqualified inside the checkers; each box type's overload
//     is found at instantiation (the call is dependent, so argument-
//     dependent lookup gathers candidates then — including overloads
//     declared later in the file, since their box types live in the
//     global namespace).
//   * Teaser for step 3: in Haskell, the identity law IMPLIES the
//     composition law for any fmap of the right type — a free theorem
//     from parametricity (Wadler 1989, "Theorems for Free!"). Write the
//     composition checker anyway. C++ is about to show you why it earns
//     its keep.
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
// FIXME(A) — TracedBox: "observability, what could it cost?"
//
// Someone instrumented fmap with a hop counter so a dashboard can show how
// many transformations a value has been through.
//
// PREDICT before enabling the tests: which checkers convict TracedBox —
// identity, composition, or both? (Careful: BOTH sides of the composition
// check carry counters too.)
//
//   ---- spoiler fold: predict first, read on after -----------------------
//
//   The repair. Two legitimate routes — pick ONE, implement it, and defend
//   the choice in a comment at QUESTION(A):
//     Route 1 (purify): the only observable act of a lawful fmap is
//       applying f to the payload. Drop the increment — and arguably the
//       field. fmap(id, b) is then indistinguishable from b again.
//     Route 2 (quotient): declare hops non-semantic bookkeeping. Replace
//       the defaulted operator== with one comparing value alone. The laws
//       then hold in the quotient the coarser equality induces — the
//       header's "up to ==" footnote, cashing out. The commitment you are
//       signing: hops may never again influence any semantic decision
//       anywhere, or the quotient was a lie.
//     Not legitimate: keeping hops both incremented and ==-observable.
//   The tests are deliberately route-agnostic (they check .value and the
//   laws, never hops), so either honest repair goes green.
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
// A colleague noticed that when f maps a type to itself (an endomorphism),
// "applying it an extra time exercises the cache better". Review escaped.
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
//   twice then g twice — g g f f, value 6. The two laws are independent
//   witnesses here, and only one of them saw the crime.
//
//   Why this cannot happen in Haskell: fmap :: (a -> b) -> f a -> f b is
//   PARAMETRIC — it cannot inspect a or b — so by the free theorem the
//   identity law alone forces the composition law. C++ templates are not
//   parametric: if constexpr over std::is_same_v, explicit and partial
//   specialisations, and constrained overloads can all branch on the
//   types. No parametricity, no free theorem — which is exactly why
//   step 2 made you write BOTH checkers.
//
//   The repair: delete the branch. A lawful fmap is oblivious to what T
//   and U are. Then answer QUESTION(B): name two other C++ mechanisms
//   (besides if constexpr + is_same) that could smuggle this same
//   violation past a reader.
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
//   MaybeBox is the Maybe functor: object map T |-> 1 + T (an empty state
//   or a value). Storage is given below — std::optional<T>.
//   * fmap applies f to the contents IF PRESENT; an empty box maps to an
//     empty box AT THE MAPPED TYPE:
//       fmap(show, MaybeBox<int>{}) == MaybeBox<std::string>{}
//   * Both step-2 checkers must pass for empty and full boxes, WITHOUT
//     writing a single new line of law code. That is the payoff of
//     generic checkers — and the real test of your step-2 signatures.
//
// HINTS
//   * The shape — empty or engaged — IS this functor's structure. fmap
//     acts on contents only and transports the shape untouched.
//   * Compute the target type once:
//       using U = std::remove_cvref_t<std::invoke_result_t<F&, T const&>>;
//     Both branches must name one and the same return type, so aggregate
//     CTAD alone cannot carry you here — the empty branch has nothing to
//     deduce from. Spell MaybeBox<U> in both.
//   * The wrong turn, rhyming with FIXME(A): "helpfully" producing
//     f(T{}) for the empty case manufactures a value out of structure.
//     fmap(id, empty) would no longer be empty, and the identity law dies
//     the same death TracedBox died.
//   * Half of structure preservation comes free from the type system:
//     returning MaybeBox<T>{} from the empty branch will not even compile
//     once f is heterogeneous. Type errors as the cheap first line of law
//     enforcement.
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
//   The composite Box ∘ MaybeBox is itself a functor. The tests spell its
//   two laws by hand; PROVE(5) asks you for the paper version.
//
// HINTS
//   * Why lift must exist at all in C++: fmap here is an OVERLOAD SET of
//     function templates — not a value. It cannot be captured, stored, or
//     passed; partial(fmap, f) from the currying exercise cannot even
//     name it. lift wraps the unqualified call in a generic lambda — a
//     hand-rolled niebloid — restoring first-class status.
//   * Inside lift, call fmap UNQUALIFIED. The call is dependent, so
//     candidates are gathered at instantiation via argument-dependent
//     lookup: one lift serves every functor in the file, including any
//     defined after lift itself. This is the closest C++ comes to "the"
//     typeclass method.
//   * Capture f by value; take the box by const&.
//   * Categorically: lift IS the morphism map, curried. And functor
//     composition is literally composition of lifts:
//       (F ∘ G)<T>  =  F<G<T>>          (F ∘ G)(f)  =  lift_F(lift_G(f))
//     Nothing new needs implementing for the composite — that is the
//     theorem, and the tests below are its machine-checked shadow.
//   * PROVE(5), as a comment, two lines per law: show that F ∘ G inherits
//     lawfulness from F and G.
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

        // the composed functor's laws, spelled by hand — the machine-
        // checked shadow of PROVE(5)
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
