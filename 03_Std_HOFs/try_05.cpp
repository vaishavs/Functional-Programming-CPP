// ===========================================================================
// Standard Library Higher-Order Functions in C++ — TODO/FIXME exercise
//
// One file, five steps, several kinds of work:
//   TODO(n)     — implement from scratch.
//   FIXME(n)    — the shipped code is wrong on purpose. Convict it, then
//                 repair it. (One of them cannot be convicted at all. That
//                 is the point; see step 3.)
//   PREDICT     — commit to an answer before compiling.
//   QUESTION    — answer in a comment next to the marker.
//   PROVE       — a short paper argument, written as a comment.
//   EXPERIMENT  — a controlled edit: change something, predict the verdict,
//                 verify, restore.
//
// After finishing a step, flip its STEPn_READY macro to 1 and rebuild.
// Steps depend on earlier ones — enable them in order. The file compiles
// and runs as shipped (all steps report TODO).
//
//   Build : g++ -std=c++20 -Wall -Wextra -o hof stdlib_hof_todo.cpp
//   Run   : ./hof
//
// The thesis
//   The standard library is a higher-order library: algorithms take
//   callables, and in exchange they impose CONTRACTS on them — the named
//   requirements. Compare:
//       Compare          irreflexive, asymmetric, transitive, and with
//                        transitive equivalence  (std::sort, std::map, ...)
//       Predicate        same input, same answer — regardless of how often
//                        or on which copy it is called      (remove_if, ...)
//       BinaryOperation  associative AND commutative for std::reduce;
//                        std::accumulate demands neither
//   Break one and you get no diagnostic. You get undefined behaviour, or a
//   result the standard never promised — which on your machine, today, may
//   look exactly right.
//
//   So this file does to the named requirements what a test suite does to
//   ordinary code: MAKES THEM EXECUTABLE (step 2), then puts three
//   plausible-looking callables in front of them (step 3). Two are
//   convicted by an assert. The third is not, on this implementation, and
//   never will be — and it is the dangerous one.
//
//   Steps 4 and 5 turn from diagnosis to construction: composing and
//   projecting callables (and proving the composites inherit the
//   contracts), then the fact that callables are VALUES — copied, moved,
//   erased, and quietly duplicated by the algorithms that take them.
//
// Ground rules
//   * Standard library only, C++20.
//   * Value semantics: nothing here mutates its inputs unless the SPEC
//     says so.
//   * Everything must compile clean under -Wall -Wextra.
//
// Hint layering per step: SPEC (the contract, pinned by the tests), HINTS
// (the mechanics), SHAPE (the skeleton, load-bearing parts elided as
// <...>). Stop reading as early as you can.
// ===========================================================================

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#define STEP1_READY 0
#define STEP2_READY 0
#define STEP3_READY 0
#define STEP4_READY 0
#define STEP5_READY 0

// ---------------------------------------------------------------------------
// Given — the sample domain, used from step 3 onwards.
// ---------------------------------------------------------------------------
struct Task {
    std::string name;
    int priority;  // 1 (lowest) .. 5 (highest)
    friend bool operator==(Task const&, Task const&) = default;
};

inline std::vector<Task> sample_tasks() {
    return {{"deploy", 5}, {"triage", 3}, {"docs", 1}, {"hotfix", 5},
            {"cleanup", 2}};
}

// ---------------------------------------------------------------------------
// STEP 1 — three generic HOFs over the algorithms
//
// SPEC
//   map_to(range, f)        ->  std::vector of f's decayed results
//   keep_if(range, pred)    ->  std::vector of the elements pred accepts
//   fold(range, init, op)   ->  left fold, strictly in order
//   * map_to must accept a POINTER TO MEMBER as f (the &Task::priority
//     test). Read the hints before deciding which algorithm to call.
//   * Inputs are taken by const&; nothing is mutated.
//
// HINTS
//   * The result element type mirrors the call, the same discipline as
//     everywhere:
//       using U = std::remove_cvref_t<
//           std::invoke_result_t<F&, std::ranges::range_reference_t<Range const&>>>;
//     F& because f is an lvalue inside the function; range_reference_t of
//     the CONST range because that is what the loop will hand it.
//   * THE trap of this step, and it is a real one — verified, not folklore:
//     the classic algorithms apply their callable with CALL SYNTAX,
//     op(*it), while the ranges algorithms go through std::invoke and
//     additionally accept a projection. Consequences:
//         std::transform(b, e, out, &Task::priority)        // ill-formed
//         std::ranges::transform(r, out, &Task::priority)   // fine
//     Two ways out, and both are worth knowing: reach for
//     std::ranges::transform, or wrap in a lambda that calls std::invoke.
//     std::count_if(b, e, &Task::flag) fails for exactly the same reason.
//   * Output: std::back_inserter(out) grows the vector as the algorithm
//     writes. Writing to out.begin() on an empty vector is the classic way
//     to corrupt memory here; reserve() is the optimisation, back_inserter
//     is the correctness.
//   * fold: C++20 has no ranges fold (std::ranges::fold_left is C++23), so
//     call std::accumulate with iterators. Note what accumulate promises
//     that reduce does not: strict left-to-right order, therefore no
//     algebraic requirement on op at all. Step 3 collects on that.
//   * The accumulator type is INIT's type, not the element type — that is
//     why the test pins both fold(doubles, 0, ...) and
//     fold(doubles, 0.0, ...). Only one of them is arithmetic; the other
//     is a truncating accident that compiles silently.
//   * QUESTION(1): the algorithms take their callable BY VALUE, and
//     std::for_each even returns it. Why by value rather than by
//     reference, and what does that imply for a callable holding state?
//     One line now; step 5 is the full reckoning.
//
// SHAPE
//     template <class Range, class F>
//     auto map_to(Range const& in, F f) {
//         using U = <...>;
//         std::vector<U> out;
//         out.reserve(std::ranges::size(in));
//         <...>;
//         return out;
//     }
//
//     template <class Range, class Pred>
//     auto keep_if(Range const& in, Pred pred) {
//         std::vector<std::ranges::range_value_t<Range>> out;
//         <...>;
//         return out;
//     }
//
//     template <class Range, class T, class Op>
//     auto fold(Range const& in, T init, Op op) {
//         return <...>;
//     }
// ---------------------------------------------------------------------------
template <class Range, class F>
auto map_to(Range const& in, F f) {
    // TODO(1a)
}

template <class Range, class Pred>
auto keep_if(Range const& in, Pred pred) {
    // TODO(1b)
}

template <class Range, class T, class Op>
auto fold(Range const& in, T init, Op op) {
    // TODO(1c)
}

// ---------------------------------------------------------------------------
// STEP 2 — the named requirements, as executables
//
// SPEC
//   check_strict_weak_ordering(comp, xs)  -> SwoReport, false-y if comp
//       fails any of the four Compare properties on the sample xs:
//         irreflexive        !comp(a, a)
//         asymmetric         comp(a, b)  =>  !comp(b, a)
//         transitive         comp(a, b) && comp(b, c)  =>  comp(a, c)
//         equiv. transitive  equiv(a, b) && equiv(b, c)  =>  equiv(a, c)
//       where equiv(a, b) = !comp(a, b) && !comp(b, a)
//   check_pure_predicate(pred, xs)   -> bool: pred's answer depends on its
//       argument and NOTHING else — not on call count, not on which copy.
//   check_associative(op, xs)        -> bool: op(op(a,b),c) == op(a,op(b,c))
//   check_commutative(op, xs)        -> bool: op(a,b) == op(b,a)
//   * All four are generic and get reused unchanged in steps 3 and 4.
//
// HINTS
//   * These are sampled property checks, not proofs — the same honesty
//     footnote every law checker carries. A clean report means "no
//     counterexample in xs", which is exactly as strong as your sample.
//     Choose samples with duplicates and with equivalent-but-distinct
//     elements; that is where orderings die.
//   * Loop over ALL ordered pairs and triples INCLUDING the diagonal.
//     Skipping a == b is the single most common way to write a checker
//     that misses the most common bug — see step 3, FIXME(A).
//   * SwoReport is given below with an explicit operator bool, so
//     assert(check_strict_weak_ordering(...)) works by contextual
//     conversion, while the four named fields stay available for
//     inspection when you want to know WHICH property died. You will want
//     to know, twice.
//   * check_pure_predicate has to defeat two different kinds of state, so
//     probe for both:
//       (a) repetition — a fresh copy of pred, applied to the same x
//           several times, must give one answer every time;
//       (b) history — a copy that has already swept the whole range must
//           still agree with a pristine copy on every x.
//     Take pred BY VALUE and make your own copies from it; never probe
//     with the same object you are wearing in.
//   * QUESTION(2): the standard permits algorithms to copy function
//     objects freely and, for most algorithms, to invoke them an
//     unspecified number of times. Given that, is (a) or (b) the more
//     faithful model of what an algorithm will do to your predicate — and
//     why does a checker want both?
//   * check_associative/check_commutative need op's result to be
//     comparable with == and feedable back into op; the samples in the
//     tests are ints and strings, which satisfy both.
//
// SHAPE
//     template <class Comp, class Range>
//     auto check_strict_weak_ordering(Comp comp, Range const& xs) {
//         SwoReport r;
//         auto equiv = [ <...> ](auto const& a, auto const& b) { return <...>; };
//         for (auto const& a : xs)
//             if ( <...> ) r.irreflexive = false;
//         for (auto const& a : xs) for (auto const& b : xs)
//             if ( <...> ) r.asymmetric = false;
//         for (auto const& a : xs) for (auto const& b : xs) for (auto const& c : xs) {
//             if ( <...> ) r.transitive = false;
//             if ( <...> ) r.equiv_transitive = false;
//         }
//         return r;
//     }
//
//     template <class Pred, class Range>
//     auto check_pure_predicate(Pred pred, Range const& xs) {   // -> bool
//         Pred worn = pred;
//         for (auto const& x : xs) (void)std::invoke(worn, x);   // wear it in
//         for (auto const& x : xs) {
//             Pred fresh = pred;
//             <... repetition probe ...>
//             <... history probe: fresh vs worn ...>
//         }
//         return true;
//     }
// ---------------------------------------------------------------------------
struct SwoReport {
    bool irreflexive = true;
    bool asymmetric = true;
    bool transitive = true;
    bool equiv_transitive = true;
    // Contextual conversion: assert(report) works, report.transitive still
    // readable. Explicit keeps it from decaying into an int in arithmetic.
    explicit operator bool() const {
        return irreflexive && asymmetric && transitive && equiv_transitive;
    }
};

template <class Comp, class Range>
auto check_strict_weak_ordering(Comp comp, Range const& xs) {  // -> SwoReport
    // TODO(2a)
}

template <class Pred, class Range>
auto check_pure_predicate(Pred pred, Range const& xs) {  // -> bool
    // TODO(2b)
}

template <class Op, class Range>
auto check_associative(Op op, Range const& xs) {  // -> bool
    // TODO(2c)
}

template <class Op, class Range>
auto check_commutative(Op op, Range const& xs) {  // -> bool
    // TODO(2d)
}

// ---------------------------------------------------------------------------
// STEP 3 — three impostors, one per checker, three kinds of repair
//
// Read this whole block before flipping STEP3_READY.
//
// ...........................................................................
// FIXME(A) — by_priority: the off-by-one-character comparator
//
// PREDICT before running: the checker reports four properties. Which of
// the four does <= break, and which does it satisfy? Work it on paper —
// the answer is not "all four", and the two survivors are the reason this
// bug lives so long in real code.
//
//   ---- spoiler fold: predict first, read on after -----------------------
//
//   comp(a,a) is true, so irreflexivity dies immediately; asymmetry dies
//   with it on the diagonal (this is why your loops must include a == b).
//   But transitivity holds — <= really is transitive — and equivalence
//   transitivity holds VACUOUSLY, because equiv(a,b) = a>b && b>a is never
//   true: under <=, no two elements are ever equivalent. Two of four.
//
//   Repair: the comparator itself. A Compare is a STRICT ordering; the
//   name of the requirement is the whole hint.
//
//   Why this file never hands the shipped comparator to std::sort, not
//   even to "see what happens": with <=, libstdc++'s unguarded insertion
//   sort can walk off the front of the buffer. The corruption is silent
//   and the crash, when it comes, is somewhere else entirely. Verify the
//   requirement, THEN call the algorithm — that ordering is the lesson,
//   and step 4 is where a checked comparator finally gets used.
// ...........................................................................
inline constexpr auto by_priority = [](Task const& a, Task const& b) {
    return a.priority <= b.priority;  // FIXME(A)
};

// ...........................................................................
// FIXME(B) — NeedsAttention: the rate limiter that moved in next door
//
// The intent is a genuine property of a task: high priority needs
// attention. Then a pager incident arrived, and "only flag the first two,
// to avoid alert spam" was added where it was easiest to add.
//
// PREDICT: what does keep_if(tasks, needs_attention).size() return as
// shipped, and what does the intent say it should be?
//
//   ---- spoiler fold -----------------------------------------------------
//
//   The predicate now answers from call history, so its result depends on
//   how many times the algorithm called it and on WHICH COPY was called.
//   Both probes in check_pure_predicate convict it.
//
//   Repair: the predicate goes back to being a property — priority alone.
//   The cap is a property of the RESULT, not of the test, so it belongs at
//   the call site (take the first two of the filtered range: with C++20,
//   std::views::filter then std::views::take, or simply resize the vector
//   keep_if returned). QUESTION(3b): name one more reason the cap does not
//   belong in the predicate, beyond the copy problem — think about what
//   std::ranges::sort or std::stable_partition would do with a predicate
//   that answers differently on its second visit to the same element.
// ...........................................................................
struct NeedsAttention {
    int budget = 2;  // FIXME(B)
    bool operator()(Task const& t) { return t.priority >= 3 && budget-- > 0; }
};

// ...........................................................................
// FIXME(C) — join_all: the one that gets away
//
// concat is string concatenation: associative, with "" as its unit — a
// monoid, exactly the Writer annotation from the monad exercise. It is NOT
// commutative. std::reduce requires BOTH, and its result is otherwise
// unspecified; std::accumulate requires NEITHER, because it fixes the
// order left-to-right.
//
// PREDICT, then verify by running: does the shipped join_all produce the
// wrong string on YOUR machine? Then answer QUESTION(3c): if the output is
// right, what exactly is broken — and what would have to change for the
// symptom to appear?
//
//   ---- spoiler fold -----------------------------------------------------
//
//   No. On this toolchain the serial std::reduce walks left to right and
//   returns precisely what accumulate returns, at every size tried. There
//   is no assert in this file that convicts FIXME(C), and there is no
//   assert you could write that would convict it here. What is broken is
//   the PERMISSION: nothing promises this, so the symptom is free to
//   appear under an execution policy, under a different standard library,
//   or after a routine toolchain bump — as a scrambled string in
//   production, from a line that never changed.
//
//   That is why step 2 exists. For a whole class of requirement
//   violations, testing the output is not a weak witness; it is no witness
//   at all. Testing the REQUIREMENT is the only reliable one, and it costs
//   four small loops.
//
//   Repair: not the operation — concat is a perfectly good monoid, and the
//   test asserts as much. Swap the algorithm for the one whose contract
//   you can actually meet. Third impostor, third kind of repair: the
//   callable, the design, and now the call site.
// ...........................................................................
inline constexpr auto concat = [](std::string a, std::string b) {
    return a + b;
};

inline std::string join_all(std::vector<std::string> const& ws) {
    // FIXME(C): the operation is associative but not commutative.
    return std::reduce(ws.begin(), ws.end(), std::string{}, concat);
}

// ---------------------------------------------------------------------------
// STEP 4 — composing and projecting callables
//
// SPEC
//   p_and(p, q)(x)   ==  p(x) && q(x)          (short-circuiting)
//   p_or(p, q)(x)    ==  p(x) || q(x)          (short-circuiting)
//   less_by(proj)    ->  a Compare that orders by the projected value
//   max_by(r, proj)  ->  a COPY of the element maximising proj
//   * less_by's results are fed straight back into step 2's checker: a
//     constructed comparator must pass the same contract a hand-written
//     one does.
//
// HINTS
//   * p_and / p_or: capture p and q by value, return a generic lambda
//     taking x by const&, apply through std::invoke so that member
//     pointers work as predicates. && and || keep their short-circuit —
//     which matters the moment a predicate is expensive, and matters more
//     when one of them is only valid after the other returned true.
//   * std::not_fn(pred) is the negation you do not have to write; the
//     tests use it. It is also the reason there is no p_not TODO here.
//   * less_by(proj) is the comparator factory the ranges algorithms make
//     almost unnecessary — almost, because std::map, std::set,
//     std::priority_queue and the classic algorithms all still want a
//     comparator, not a projection. Invoke the projection with
//     std::invoke so &Task::priority works, and compare with < only:
//     building it from <= would reintroduce FIXME(A) at one remove.
//   * max_by: std::ranges::max_element(in, {}, proj) — the {} is
//     std::ranges::less. Dereference the returned iterator to copy the
//     element. Empty ranges give end(); the tests avoid them, and a
//     production version would return an optional.
//   * PROVE(4a), four lines: assume < on the projected type is a strict
//     weak ordering, and show less_by(proj) is one too — one line per
//     property, each an immediate consequence of the corresponding
//     property downstairs. Then note where the assumption fails: it is not
//     exotic, and EXPERIMENT(4c) below is exactly that failure.
//   * PROVE(4b), the algebra: with always_true as unit, do predicates
//     under p_and form a monoid? State the identity and associativity
//     laws, say in what sense they hold (the same sense the monad
//     exercise's Kleisli arrows were equal — these are functions, so the
//     equality is extensional), and name the unit for p_or.
//   * EXPERIMENT(4c) — the famous one. The doubles test below passes.
//     Now add std::nan("") to that sample vector, rebuild, and inspect
//     the SwoReport's four fields. PREDICT first: which properties fail?
//     Exactly one does, and it is the property nobody checks by hand —
//     because every comparison involving NaN is false, so NaN is
//     "equivalent" to everything, and equivalence stops being transitive.
//     This is the whole reason sorting a container of doubles that might
//     contain NaN is undefined behaviour rather than merely untidy.
//     Restore the sample afterwards.
//
// SHAPE
//     template <class P, class Q>
//     auto p_and(P p, Q q) {
//         return [ <...> ](auto const& x) { return <...>; };
//     }
//
//     template <class Proj>
//     auto less_by(Proj proj) {
//         return [ <...> ](auto const& a, auto const& b) { return <...>; };
//     }
//
//     template <class Range, class Proj>
//     auto max_by(Range const& in, Proj proj) {
//         return <...>;
//     }
// ---------------------------------------------------------------------------
inline constexpr auto always_true = [](auto const&) { return true; };

template <class P, class Q>
auto p_and(P p, Q q) {
    // TODO(4a)
}

template <class P, class Q>
auto p_or(P p, Q q) {
    // TODO(4b)
}

template <class Proj>
auto less_by(Proj proj) {
    // TODO(4c)
}

template <class Range, class Proj>
auto max_by(Range const& in, Proj proj) {
    // TODO(4d)
}

// ---------------------------------------------------------------------------
// STEP 5 — callables are values
//
// Everything above passed callables around as if that were free. It is
// not: they are objects, algorithms copy them, and two of the most durable
// bugs in C++ live in the gap between "the callable I passed" and "the
// callable that ran".
//
// ...........................................................................
// FIXME(D) — count_visits: the tally that is always zero
//
// PREDICT the shipped return value before running.
//
//   ---- spoiler fold -----------------------------------------------------
//
//   std::for_each takes its function object BY VALUE. The instance that
//   counted is the algorithm's copy; the local one never saw an element,
//   and the answer is 0 no matter how long the range is.
//
//   Two legitimate repairs — pick ONE and defend it in a comment at
//   QUESTION(5a):
//     Route 1: std::for_each RETURNS the (moved-from-the-inside) function
//       object, precisely so this is recoverable. Use the return value.
//       Note that std::ranges::for_each returns an aggregate of the
//       iterator and the function, so the same idea reads differently.
//     Route 2: pass std::ref(v). A reference_wrapper is copyable — so the
//       algorithm still gets to copy freely, which is what it wants — but
//       every copy forwards to the one object you own.
//   Route 1 keeps the callable a value and is the more honest fit for an
//   algorithm that promises nothing about copies. Route 2 is what you
//   reach for when the state must be shared with something else too.
//   The tests accept either.
//
//   Not a repair: making the count a mutable member so the const call
//   operator can touch it. That changes which copy is mutated, not how
//   many copies there are.
// ...........................................................................
struct Visits {
    int n = 0;
    void operator()(Task const&) { ++n; }
};

inline int count_visits(std::vector<Task> const& tasks) {
    Visits v;
    std::for_each(tasks.begin(), tasks.end(), v);
    // FIXME(D)
    return v.n;
}

// ...........................................................................
// FIXME(E) — drop_matching: the algorithm that does not do what it says
//
// PREDICT the shipped result's size, and its contents.
//
//   ---- spoiler fold -----------------------------------------------------
//
//   std::remove_if cannot remove anything: it has iterators, not the
//   container. It compacts the survivors to the front and returns the
//   iterator one past the last of them; everything from there to end() is
//   valid but unspecified, and size() has not moved. Discarding that
//   return value throws away the only piece of information the algorithm
//   produced.
//
//   Repairs, either one: the erase-remove idiom,
//       v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
//   or, since C++20, the one-liner that cannot be got wrong:
//       std::erase_if(v, pred);
//   QUESTION(5b): std::erase_if exists as a free function per container
//   rather than as a member. Given std::list has always had a member
//   remove_if with better complexity, what does that tell you about what
//   erase-remove was really working around?
// ...........................................................................
template <class Pred>
std::vector<int> drop_matching(std::vector<int> v, Pred pred) {
    std::remove_if(v.begin(), v.end(), pred);  // FIXME(E)
    return v;
}

// ...........................................................................
// QUESTION(5c) + EXPERIMENT(5d) — type erasure
//
// std::function<void(Task const&)> stores a COPY of whatever it is given
// and copies of the std::function copy that state again. So the FIXME(D)
// bug survives being wrapped: a counter inside a std::function is still a
// counter you do not own.
//
// EXPERIMENT(5d): after the tests below pass, add this to the step 5
// block, predict the printed value, then run it:
//     Visits v;
//     std::function<void(Task const&)> f = std::ref(v);
//     auto g = f;                       // copy the std::function
//     f(tasks[0]); g(tasks[0]);
//     std::cout << v.n << "\n";         // PREDICT before running
// Then change std::ref(v) to plain v and predict again. The difference is
// the whole of std::ref in one line: it makes a copyable handle whose
// copies are not copies of the state.
//
// QUESTION(5c): every call through std::function is an indirect one, and
// the template parameter route (as used everywhere else in this file)
// avoids it entirely. Name two situations where std::function earns that
// cost anyway. One hint: a container of heterogeneous callables. Another:
// before C++23's deducing this, a lambda that needs to call itself had
// nowhere to name its own type.
// ...........................................................................

// ===========================================================================
// Tests — do not modify below this line (except where an EXPERIMENT says
// so). Each assert is annotated with the property it pins down.
// ===========================================================================
int main() {
    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        auto tasks = sample_tasks();

        // a pointer to member as the mapping function — catches the
        // classic-algorithm route, which will not compile for this
        assert((map_to(tasks, &Task::priority)
                == std::vector<int>{5, 3, 1, 5, 2}));

        // heterogeneous mapping: Task -> std::string
        assert((map_to(tasks, [](Task const& t) { return t.name; })
                == std::vector<std::string>{"deploy", "triage", "docs",
                                            "hotfix", "cleanup"}));

        assert(keep_if(std::vector<int>{1, 2, 3, 4, 5},
                       [](int x) { return x % 2 == 0; })
               == (std::vector<int>{2, 4}));

        assert(fold(std::vector<int>{1, 2, 3, 4}, 0, std::plus<>{}) == 10);

        // the accumulator has INIT's type. Both lines below are correct
        // C++ doing exactly what it promised; only one is arithmetic.
        std::vector<double> ds{1.5, 2.25, 3.0};
        assert(fold(ds, 0.0, std::plus<>{}) == 6.75);  // double accumulator
        assert(fold(ds, 0, std::plus<>{}) == 6);       // int: truncates each step
        std::cout << "step 1  map_to / keep_if / fold  ok\n";
    }
#else
    std::cout << "step 1  map_to / keep_if / fold  TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        std::vector<int> xs{1, 2, 2, 3, 5};  // duplicates on purpose

        assert(check_strict_weak_ordering(std::less<>{}, xs));
        assert(check_strict_weak_ordering(std::greater<>{}, xs));

        // <= must be rejected, and by exactly two of the four properties
        auto le = [](int a, int b) { return a <= b; };
        auto r = check_strict_weak_ordering(le, xs);
        assert(!r);
        assert(!r.irreflexive && !r.asymmetric);   // the diagonal convicts
        assert(r.transitive && r.equiv_transitive);  // these survive — see FIXME(A)

        // purity: a stateless predicate passes, a call-counting one does not
        assert(check_pure_predicate([](int x) { return x > 2; }, xs));
        struct EveryOther {
            int seen = 0;
            bool operator()(int) { return seen++ % 2 == 0; }
        };
        assert(!check_pure_predicate(EveryOther{}, xs));

        // reduce-safety: + qualifies on both counts, - on neither
        assert(check_associative(std::plus<>{}, xs));
        assert(check_commutative(std::plus<>{}, xs));
        assert(!check_associative(std::minus<>{}, xs));
        assert(!check_commutative(std::minus<>{}, xs));
        std::cout << "step 2  requirement checkers .... ok\n";
    }
#else
    std::cout << "step 2  requirement checkers .... TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        auto tasks = sample_tasks();

        // FIXME(A) — convicted here. No sort appears in this block on
        // purpose: an unverified comparator never meets an algorithm.
        assert(check_strict_weak_ordering(by_priority, tasks));

        // FIXME(B) — convicted by both probes; and the intent restored:
        // three tasks have priority >= 3
        assert(check_pure_predicate(NeedsAttention{}, tasks));
        assert(keep_if(tasks, NeedsAttention{}).size() == 3);

        // FIXME(C) — no conviction available. These two asserts state
        // FACTS about concat: a monoid that is not commutative. They pass
        // before and after the repair, as does the line below them. The
        // only thing that changes is whether the code is entitled to its
        // answer.
        std::vector<std::string> ws{"al", "be", "ga"};
        assert(check_associative(concat, ws));
        assert(!check_commutative(concat, ws));
        assert(join_all(ws) == "albega");

        // for contrast, an operation that really is reduce-safe
        std::vector<int> ns{1, 2, 3, 4};
        assert(check_associative(std::plus<>{}, ns)
               && check_commutative(std::plus<>{}, ns));
        std::cout << "step 3  impostors diagnosed ..... ok\n";
    }
#else
    std::cout << "step 3  impostors diagnosed ..... TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        auto tasks = sample_tasks();
        auto urgent = [](Task const& t) { return t.priority >= 3; };
        // {deploy, docs, cleanup} — deliberately overlapping urgent in
        // exactly one place, so && and || cannot be confused
        auto early = [](Task const& t) { return t.name.front() <= 'd'; };

        // composition, and std::not_fn as the negation you do not write
        assert(keep_if(tasks, p_and(urgent, early)).size() == 1u);  // deploy
        assert(keep_if(tasks, p_or(urgent, early)).size() == 5u);
        assert(keep_if(tasks, std::not_fn(urgent)).size() == 2u);

        // the unit of p_and — PROVE(4b)'s numeric shadow, extensionally
        for (auto const& t : tasks)
            assert(p_and(urgent, always_true)(t) == urgent(t));

        // a CONSTRUCTED comparator must satisfy the same contract a
        // handwritten one does — step 2's checker, unchanged
        assert(check_strict_weak_ordering(less_by(&Task::priority), tasks));
        assert(check_strict_weak_ordering(less_by(&Task::name), tasks));

        // EXPERIMENT(4c) lives here: add std::nan("") to this vector,
        // rebuild, and read the four fields of the report.
        std::vector<double> ds{3.5, -1.0, 2.0};
        assert(check_strict_weak_ordering(less_by(std::identity{}), ds));

        // a checked comparator, finally used
        auto sorted = tasks;
        std::ranges::sort(sorted, less_by(&Task::priority));
        assert(sorted.front().priority == 1 && sorted.back().priority == 5);

        // the ranges route says the same thing with a projection instead:
        // {} is std::ranges::less, and the projection goes through invoke
        auto sorted2 = tasks;
        std::ranges::sort(sorted2, {}, &Task::priority);
        assert(map_to(sorted2, &Task::priority) == map_to(sorted, &Task::priority));

        assert(max_by(tasks, &Task::priority).priority == 5);
        assert(max_by(tasks, [](Task const& t) { return t.name.size(); }).name
               == "cleanup");

        // std::bind_front specialises a binary callable; std::mem_fn turns
        // a member into one — both are HOFs the library hands you
        auto at_least = [](int bound, Task const& t) { return t.priority >= bound; };
        assert(keep_if(tasks, std::bind_front(at_least, 5)).size() == 2);
        assert(map_to(tasks, std::mem_fn(&Task::priority))
               == map_to(tasks, &Task::priority));
        std::cout << "step 4  composition + projection  ok\n";
    }
#else
    std::cout << "step 4  composition + projection  TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 ----------------------------------------------------------
#if STEP5_READY
    {
        auto tasks = sample_tasks();

        // FIXME(D) — the algorithm counted; your object did not
        assert(count_visits(tasks) == 5);

        // FIXME(E) — remove_if compacts and reports; it cannot erase
        auto odd = [](int x) { return x % 2 != 0; };
        auto kept = drop_matching({1, 2, 3, 4, 5}, odd);
        assert(kept.size() == 2);                    // size must actually shrink
        assert((kept == std::vector<int>{2, 4}));    // and the tail must be gone
        std::cout << "step 5  callables are values ... ok\n";
    }
#else
    std::cout << "step 5  callables are values ... TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
