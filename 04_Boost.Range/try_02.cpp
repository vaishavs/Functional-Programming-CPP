// =============================================================================
// Boost.Range bug-hunting exercise — ANNOTATED EDITION
// =============================================================================
// This file intentionally contains 26 separate, numbered bugs (Bug 0 through
// Bug 25) built around Boost.Range adaptors, Boost.Iterator internals, and a
// few classic <algorithm> pitfalls. The CODE below is left exactly as
// originally written — every bug is still there, unfixed, on purpose. Only
// the surrounding commentary has been expanded and, in a few places,
// corrected to be more precise about the actual mechanism at work.
//
// Each block comment covers, in order: what the line is trying to do, the
// exact C++/Boost rule it violates, the concrete consequence (compile
// error, undefined behavior, or silent wrong answer), and — where it adds
// value — a pointer toward the correct pattern.
//
// A few threads recur across multiple bugs and are worth knowing up front:
//   - Reference-lifetime extension (`const auto&` / `auto&&` bound directly
//     to a temporary) genuinely extends the OUTERMOST temporary's lifetime.
//     It does NOT reach into nested temporaries used to build that outer
//     value (e.g. a temporary container piped into an adaptor). Bugs 0, 5,
//     13, 21, and 24 all turn on this distinction, and some of them are
//     more "fragile" than "literally broken" once you trace it through.
//   - `filtered` ranges can never be random access — you cannot skip
//     directly to the Nth surviving element without testing everything
//     before it — and `dropped` is not actually a Boost.Range name at all
//     (Bugs 7, 22, 24). `transformed` ranges CAN be random access for
//     reading, but never offer a writable backing store unless the
//     function itself returns a reference (Bugs 1, 14, 20).
//   - Views (`filtered_range`, `iterator_range`, ...) are not Containers:
//     they have no `.erase()` / `.insert()` (Bugs 10, 18).
//   - `remove_if` rearranges without shrinking; pair it with `.erase()` or
//     use `remove_erase_if` instead (Bug 8).
// =============================================================================

#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

using namespace std;
using namespace boost::adaptors;

int main() {
    vector<int> nums = {1, 2, 3, 4, 5};

    // =========================================================================
    // BUG 0 — "Dangling" range reference (and why that label is subtler than
    //          it looks)
    // =========================================================================
    // What this line is trying to do: build a lazy view over `nums` where
    // every element has been squared, and hold onto it via `const auto&` so
    // it can be sorted (see Bug 1) without paying for a separate container.
    //
    // The naive read — "auto& binds to a temporary adaptor, so it's gone by
    // the semicolon" — is not quite right, and it's worth understanding why,
    // because getting this wrong in either direction is exactly how people
    // get bitten by Boost.Range. `nums | transformed(...)` calls
    // `operator|`, which constructs and returns a `boost::transformed_range`
    // BY VALUE — the whole expression is a prvalue. C++'s reference-
    // lifetime-extension rule says binding a `const T&` (or `T&&`) DIRECTLY
    // to a prvalue extends that prvalue's lifetime to match the reference's
    // own lifetime. So the `transformed_range` wrapper `squares` refers to
    // is NOT destroyed at the end of this statement — it lives until
    // `squares` itself goes out of scope, at the end of `main`.
    //
    // What actually IS fragile here:
    //   1. The wrapper stores iterators obtained from `nums`, a named,
    //      long-lived object — those iterators stay valid as long as `nums`
    //      isn't reallocated, destroyed, or moved from (see Bug 25) while
    //      `squares` is alive.
    //   2. Lifetime extension only rescues the OUTERMOST temporary bound
    //      directly by the reference. If the range being adapted were
    //      itself a temporary (e.g. `vector<int>{1,2,3,4,5} |
    //      transformed(...)`, see Bug 11 and Bug 13), that inner temporary
    //      is NOT extended — it's destroyed at the end of the full
    //      expression regardless of what the outer reference does. THAT is
    //      genuine dangling.
    //   3. `squares` is const-qualified and lazily computed: it never
    //      actually stores squared values anywhere, it recomputes `x * x`
    //      on every dereference. That combination — not a dead pointer — is
    //      what breaks Bug 1.
    // The real lesson: relying on lifetime extension for adaptor chains
    // works only by accident of exactly how the expression happens to be
    // shaped, and an innocent-looking refactor (wrapping this in a
    // function, swapping `nums` for a temporary, adding one more layer of
    // `auto`) silently turns "technically fine" into "genuinely dangling"
    // with no compiler warning either way. Treat `const auto&` / `auto&&`
    // bound straight to an adaptor pipeline as suspect by default;
    // materialize into a real container when the result must outlive the
    // statement that built it, e.g.:
    //     vector<int> squares_v;
    //     boost::range::push_back(squares_v, nums | transformed([](int x){ return x*x; }));
    // =========================================================================
    const auto& squares = nums | transformed([](int x) { return x * x; }); // fragile, not literally dangling — see note above

    // =========================================================================
    // BUG 1 — Sorting a const, lazily-computed range in place
    // =========================================================================
    // `boost::range::sort` forwards to `std::sort`, which needs a mutable
    // RandomAccessRange: it must be able to both compare AND SWAP elements
    // in place. `squares` fails this on two independent grounds, either one
    // of which is fatal on its own:
    //
    //   1. Const-correctness: `squares` was declared `const auto&` in
    //      Bug 0, so it's a const object. You can no more call a mutating
    //      algorithm on it than you could call `.push_back()` on a
    //      `const vector`.
    //
    //   2. Nothing to swap: `transformed`'s iterator computes `x * x` fresh
    //      on every dereference and returns that INT BY VALUE (a prvalue),
    //      not a reference into any storage. There is no lvalue for
    //      `std::sort` to write back into — the squared values were never
    //      stored anywhere; they're synthesized on demand from `nums`.
    //      Because the transform function returns a value type rather than
    //      a reference, this iterator's legacy
    //      `std::iterator_traits<...>::iterator_category` doesn't come out
    //      as `random_access_iterator_tag` either — it's effectively capped
    //      at bidirectional — so `std::sort`'s own overload/concept checks
    //      reject it before the "nothing to swap" problem even comes up.
    //
    // Expect a wall of template-instantiation errors here, not one clean
    // line. If the intent was "give me the squares, sorted," materialize
    // them into an owned, mutable container first:
    //     vector<int> sorted_squares;
    //     boost::range::push_back(sorted_squares, nums | transformed([](int x){ return x*x; }));
    //     boost::range::sort(sorted_squares);
    // =========================================================================
    boost::range::sort(squares); // won't compile: const, and the transform result isn't writable random-access

    // =========================================================================
    // BUG 2 — Adaptor order changes MEANING, not just style (silent logic bug)
    // =========================================================================
    // `filtered` and `transformed` are function composition, and — exactly
    // like function composition — they are not commutative. Each stage in a
    // `|` pipeline operates on the OUTPUT of the previous stage, so:
    //
    //     nums | transformed(square) | filtered(p)
    //
    // evaluates `p` against the SQUARED values (1,4,9,16,25 for nums =
    // {1..5}), giving {16, 25} for p(x) = x >= 10 — i.e. it filters
    // *after* squaring, which is what this code currently does. Swap the
    // two stages —
    //
    //     nums | filtered(p) | transformed(square)
    //
    // — and `p` is now evaluated against the ORIGINAL values, before
    // squaring. With that same p(x) = x >= 10 predicate you'd get an EMPTY
    // range (none of 1..5 is >= 10) and then square whatever survived —
    // zero elements either way, but for a completely different reason, and
    // with a predicate that would need rewriting (e.g. `x >= 3`) to be
    // meaningful against the original values at all.
    //
    // Same source data, same predicate expression, two different pipelines,
    // two different results — and both versions compile cleanly and both
    // look equally "reasonable" at a glance. That's what makes this class
    // of bug dangerous: no crash, no warning, no const violation to grep
    // for, just a range that quietly contains the wrong elements. Whenever
    // you chain `filtered`/`transformed`, stop and ask explicitly: "is this
    // predicate meant to see the original values, or the values produced by
    // the stage before it?" — and double-check that the order you typed is
    // the order you actually meant, not just the order that happened to
    // come out first.
    // =========================================================================
    auto filtered_squares = nums
        | transformed([](int x) { return x * x; })
        | filtered([](int x) { return x >= 10; }); // filters the SQUARED values — confirm that's what you meant

    // =========================================================================
    // BUG 3 — Passing a container where copy() wants an output iterator
    // =========================================================================
    // `boost::range::copy(source, out)`'s second parameter must be an
    // OutputIterator (something `*out++ = value` works on) — e.g.
    // `std::back_inserter(copy_vec)` or `boost::begin(some_range)` — not a
    // container object. `nums` is a `vector<int>`, not an iterator, so it
    // doesn't type-check against the algorithm's expected signature.
    //
    // Two more things make this worse than a simple typo:
    //   - `copy_vec` is declared right above and never used again — a
    //     strong signal that IT, not `nums`, was the intended destination.
    //   - `filtered_squares` is itself a LAZY view still reading from
    //     `nums` on demand. Even if this were coerced into compiling (say,
    //     via `boost::begin(nums)`), you'd be overwriting the very
    //     container `filtered_squares` reads from WHILE `copy` walks it —
    //     so later reads through `filtered_squares` could observe values
    //     `copy` just wrote, not the originals. Aliasing the source and
    //     destination of a copy through a lazy view is a correctness
    //     landmine independent of the type mismatch that stops this from
    //     compiling in the first place.
    // Fix: boost::range::copy(filtered_squares, std::back_inserter(copy_vec));
    // =========================================================================
    vector<int> copy_vec;
    boost::range::copy(filtered_squares, nums); // `nums` is a container, not an OutputIterator — and it aliases the source

    // =========================================================================
    // BUG 4 — Stateful/mutable capture inside a lazy transform
    // =========================================================================
    // `transformed`'s function is not guaranteed to run exactly once per
    // logical element. Boost.Range views are LAZY: the function is invoked
    // afresh every time an iterator is dereferenced, and different
    // algorithms — or simply iterating the same range twice, calling
    // `boost::size`, running a `find` before a `copy`, and so on — can
    // dereference the same position more than once. Because this lambda
    // captures `offset` BY REFERENCE and mutates it (`offset++`) on every
    // invocation, dereferencing the "same" logical element twice yields two
    // DIFFERENT results (e.g. first look: nums[0] + 0; a later look at that
    // same position: nums[0] + 1, or higher). That breaks the basic
    // expectation that a range yields a consistent value every time you
    // look at a given position.
    //
    // This also silently defeats any algorithm or optimization that assumes
    // dereferencing is idempotent / referentially transparent — plenty are
    // entitled to assume exactly that for anything at least an
    // InputIterator. Transform functions fed to lazy adaptors should be
    // pure: no shared mutable state, no side effects, same input producing
    // the same output no matter how many times or in what order it's
    // invoked.
    // =========================================================================
    int offset = 0;
    auto offsetted = nums | transformed([&offset](int x) { return x + offset++; }); // impure: result depends on call count, not just x

    // =========================================================================
    // BUG 5 — Splitting a range into loose begin/end iterators
    // =========================================================================
    // As with Bug 0, `auto&&` binding directly to the `filtered_range`
    // prvalue DOES trigger lifetime extension here, and `nums` is a durable
    // lvalue — so `temp_range` itself is not dangling in this exact
    // instance. The bug being illustrated is the general PATTERN, not a
    // guaranteed crash on this specific line: pulling `it1`/`it2` out of an
    // adaptor and holding them as separate variables decouples the
    // iterators from whatever keeps the underlying view object (and, for
    // some adaptors, the predicate/functor state it owns) alive.
    // `boost::filter_iterator`s carry a copy of the predicate and bounds
    // derived from the range object they came from; if that owning range
    // object were ever NOT lifetime-extended — a temporary passed through
    // one more layer of indirection, returned from a helper function,
    // stored as a class member — `it1`/`it2` would be left pointing at a
    // destroyed object even though nothing about `it1`/`it2` themselves
    // looks suspicious at their own call site. Prefer iterating the range
    // directly (range-based for, or algorithms taking the range as a whole)
    // over manually extracting and storing `begin()`/`end()` separately —
    // it removes an entire class of "it happened to work" bugs.
    // =========================================================================
    auto&& temp_range = nums | filtered([](int x) { return x % 2 == 0; });
    auto it1 = boost::begin(temp_range);
    auto it2 = boost::end(temp_range);
    for (auto it = it1; it != it2; ++it) { // fragile pattern, even though this particular instance is memory-safe
        cout << *it << " ";
    }
    cout << endl;

    // =========================================================================
    // BUG 6 — A function that doesn't exist in Boost.Range
    // =========================================================================
    // `boost::range::apply_filtered` is not part of Boost.Range: no header,
    // no declaration, nothing — it's a fabricated name. It's left commented
    // out for exactly that reason; uncommenting it fails to compile with an
    // "undeclared identifier" / "no member named" error. The point of
    // including it is to practice recognizing APIs that merely SOUND
    // plausible next to real ones like `filtered`, `remove_if`, and
    // `remove_erase_if`.
    //
    // What you actually want depends on intent:
    //   - A read-only, lazy view of the elements matching a predicate,
    //     leaving `nums` untouched:
    //         nums | filtered([](int x){ return x > 3; })
    //   - Physically removing the non-matching elements from `nums` itself:
    //         boost::range::remove_erase_if(nums, [](int x){ return x <= 3; });
    //     (note the predicate is inverted versus `filtered` — `filtered`
    //     keeps what matches, `remove_erase_if` removes what matches)
    // =========================================================================
    // boost::range::apply_filtered(nums, [](int x) { return x > 3; }); // fictional API — would fail to compile

    // =========================================================================
    // BUG 7 — `dropped` isn't even a real Boost.Range adaptor, and the
    //          intended "drop N elements" idea is unsafe on an empty range
    //          regardless
    // =========================================================================
    // Two separate problems stacked here:
    //
    //   1. `dropped` does not exist in Boost.Range. The complete list of
    //      adaptors pulled in by <boost/range/adaptors.hpp> is:
    //      adjacent_filtered, copied, filtered, formatted, indexed,
    //      indirected, map_keys/map_values, replaced, replaced_if,
    //      reversed, sliced, strided, tokenized, transformed, uniqued.
    //      There is no `dropped`. (A same-named adaptor exists in the
    //      separate, unrelated "Oven" library by p-stade, and
    //      "drop"/`views::drop` exists in range-v3 and C++20's <ranges> —
    //      it's easy to misremember which range library a name belongs
    //      to.) With only `using namespace boost::adaptors;` in scope,
    //      there is no candidate for `dropped` to resolve to, so this line
    //      fails to compile with an unresolved-identifier error before any
    //      of its runtime behavior matters. The nearest real Boost.Range
    //      tool for "skip the first N" is `sliced(N, boost::size(rng))`.
    //      (Note also that the variable being declared on this same line is
    //      ALSO named `dropped` — harmless here, since a variable's own
    //      initializer can't see its own name, but it's a confusing choice
    //      once you realize the function name doesn't even resolve to
    //      anything real.)
    //
    //   2. Even granting a working "drop first N" adaptor: `nums |
    //      filtered(x > 100)` is EMPTY (no element of {1,2,3,4,5} exceeds
    //      100), so "drop 1" is being asked to skip past an element that
    //      doesn't exist. Advancing an iterator beyond a range's `end()` —
    //      even by one step, even if you never read through it — is
    //      undefined behavior; only `end()` itself is a legal "one past the
    //      last element" position. Then, independently,
    //      `*boost::begin(dropped)` dereferences what is likely `end()` (or
    //      worse) — a SECOND, separate source of undefined behavior.
    //      "May crash" undersells it: this isn't "probably fine, just
    //      risky," it's UB, so the actually observed behavior (silent
    //      garbage, a clean crash, or an innocent-looking wrong number) is
    //      unpredictable by design, not just by bad luck.
    // =========================================================================
    auto dropped = nums | filtered([](int x) { return x > 100; }) | dropped(1); // `dropped` isn't real Boost.Range, and the filtered range is empty anyway
    cout << "First after drop: " << *boost::begin(dropped) << endl; // dereferences a likely-end() iterator: UB, not just "maybe wrong"

    // =========================================================================
    // BUG 8 — remove_if rearranges; it does not shrink the container
    // =========================================================================
    // This is the classic "erase-remove idiom" trap. `std::remove_if` (and
    // `boost::range::remove_if`, which forwards to it) does NOT delete
    // anything. It shuffles elements so everything that should be "kept" is
    // moved to the front, in order, and returns an iterator marking the new
    // logical end; everything from that iterator to the ORIGINAL end is
    // left in a valid-but-unspecified state (frequently stale duplicates of
    // retained elements). Crucially, `nums.size()` is completely unchanged
    // by this call.
    //
    // The return value here is discarded and `.erase()` is never called, so
    // `nums` still has 5 elements after this line — just with the
    // surviving even numbers moved to the front and unspecified leftovers
    // occupying the tail. There's no crash and no compiler warning;
    // downstream code that trusts `nums.size()`, or iterates the "wrong"
    // tail elements, just quietly gets incorrect data.
    //
    // Fix — either finish the idiom by hand:
    //     nums.erase(boost::range::remove_if(nums, pred), nums.end());
    // or use Boost.Range's one-call version that does both steps for you:
    //     boost::range::remove_erase_if(nums, pred);
    // =========================================================================
    boost::range::remove_if(nums, [](int x) { return x % 2 == 1; }); // return value discarded — nums.size() is still 5

    // =========================================================================
    // BUG 9 — unique_copy only collapses ADJACENT duplicates, and this test
    //          data can't reveal that, so the call quietly proves nothing
    // =========================================================================
    // The `const auto&` here isn't actually the problem — reading through a
    // const reference is exactly what a copy-style algorithm's SOURCE
    // parameter is supposed to accept, so `const_nums` compiles and runs
    // fine as `unique_copy`'s input. The `const` is a red herring.
    //
    // The real, easy-to-miss trap is semantic: `unique_copy` (modeled on
    // `std::unique`) only removes a duplicate if it is IMMEDIATELY ADJACENT
    // to the value before it. It is not a general "keep only distinct
    // values" operation, and it does not sort or reorder anything first. On
    // an unsorted sequence with duplicates spread apart (e.g.
    // {1, 2, 1, 3, 1}), it happily copies every one of those three 1's
    // through untouched, because none of them sits right next to another
    // 1. People very commonly reach for `unique`/`unique_copy` expecting
    // "give me the distinct values" and are surprised later when
    // non-adjacent duplicates survive.
    //
    // This particular call can't even demonstrate that failure mode: `nums`
    // is {1,2,3,4,5} — sorted and already fully distinct — so `unique_copy`
    // just copies all five elements through unchanged no matter how it's
    // implemented, and `unique_vec` "looks correct" purely by accident of
    // the input data. If you actually need global de-duplication, sort
    // first — don't trust a passing result that was built on data
    // incapable of failing.
    // =========================================================================
    const auto& const_nums = nums;
    vector<int> unique_vec;
    boost::range::unique_copy(const_nums, back_inserter(unique_vec)); // only strips ADJACENT dupes; nums has none, so this proves nothing

    // =========================================================================
    // BUG 10 — remove_erase_if needs an owning Container, not a Range/view
    // =========================================================================
    // `remove_erase_if` performs the full erase-remove idiom for you:
    // internally it calls `remove_if` and then calls
    // `.erase(new_end, old_end)` ON THE SAME ARGUMENT you passed in. That
    // means its argument must be an actual CONTAINER — something that owns
    // its storage and exposes an `.erase(first, last)` member, like
    // `std::vector`.
    //
    // `boost::make_iterator_range(nums.begin(), nums.end())` builds a
    // `boost::iterator_range<...>` — a lightweight, NON-OWNING VIEW made of
    // nothing more than a begin/end iterator pair. It satisfies the `Range`
    // concept (it has `begin()`/`end()`), but it is not a `Container`: it
    // has no `.erase()` member, because it doesn't own any elements to
    // erase — `nums` does. Passing it to `remove_erase_if` should fail to
    // compile with something like "no member named 'erase'" once the
    // template tries to call `.erase()` on the `iterator_range`. This is a
    // good illustration of the Range-vs-Container distinction that shows up
    // constantly with Boost.Range (and later, with C++20's own <ranges>
    // views): a view lets you iterate or re-window existing data, but only
    // the underlying container can be asked to grow or shrink. If you need
    // to mutate in place, operate on `nums` itself, not on a view built
    // from it.
    // =========================================================================
    boost::range::remove_erase_if(boost::make_iterator_range(nums.begin(), nums.end()),
        [](int x) { return x < 0; }); // iterator_range has no .erase() — it's a view, not a container

    // =========================================================================
    // BUG 11 — sliced() over a temporary vector: genuine dangling, unlike Bug 0
    // =========================================================================
    // This is the real thing Bug 0's original comment warned about, and
    // it's worth contrasting the two directly now that the lifetime-
    // extension mechanics are clear. Here, the container being adapted —
    // `vector<int>{1,2,3,4,5}` — is itself an unnamed TEMPORARY, not a
    // named lvalue like `nums`. It is passed into `operator|` as a function
    // ARGUMENT, and per the standard's lifetime rules, a temporary used to
    // compute a function's result is destroyed at the end of the FULL
    // EXPRESSION that contains the call. It is NOT the direct initializer
    // bound by `sliced_ref`'s reference, so it is NOT covered by
    // reference-lifetime-extension. Only the outer `sliced_range` WRAPPER
    // object — the thing directly bound by `const auto&` — gets its life
    // extended; the `vector<int>` buffer it points into does not, and is
    // gone by the time the semicolon is reached.
    //
    // So `sliced_ref` ends up holding iterators into a `vector<int>` that
    // no longer exists — genuinely dangling, full stop, no "it happens to
    // be safe here" caveat the way Bug 0 had. Any later read through
    // `sliced_ref` is undefined behavior. The fix is simple: give the
    // container a name and a scope that actually outlives the view.
    //     vector<int> temp{1,2,3,4,5};
    //     const auto& sliced_ref2 = temp | sliced(1, 4); // fine — `temp` outlives `sliced_ref2`
    // =========================================================================
    const auto& sliced_ref = vector<int>{1, 2, 3, 4, 5} | sliced(1, 4); // the vector is a temporary; this genuinely dangles

    // =========================================================================
    // BUG 12 — Lambda captures a block-scoped local by reference
    // =========================================================================
    // `local` has automatic storage duration scoped to this `{ ... }`
    // block. The lambda captures it BY REFERENCE (`[&local]`), and that
    // reference lives inside `with_local` (a `transformed_range`) for as
    // long as `with_local` exists. As written, `with_local` is ALSO
    // declared inside this same block and is never touched again after the
    // closing brace, so this exact snippet does not, by itself, trigger the
    // undefined behavior — nothing reads through `with_local` once `local`
    // is gone.
    //
    // It's still flagged as a bug because it's a template for a much easier
    // mistake to make: the moment `with_local` (or its captured lambda)
    // escapes this block — returned from a function, stored in a member
    // variable, assigned outside the block, passed to something that keeps
    // it around — every subsequent dereference reads through a reference to
    // a destroyed `int`. That's a dangling reference capture, the same
    // family of bug as returning a reference to a local variable from a
    // function, just reached via a lazy adaptor instead of a `return`. Best
    // practice: never let a range/lambda that captures locals BY REFERENCE
    // outlive the scope of those locals — capture by VALUE (`[local]`)
    // instead if the range might be used later.
    // =========================================================================
    {
        int local = 42;
        auto with_local = nums | transformed([&local](int x) { return x + local; }); // fine only because it's never used past this line
    }

    // =========================================================================
    // BUG 13 — A range returned from a function, built over a function-local
    //          temporary: unambiguously dangling, independent of how it's bound
    // =========================================================================
    // Inside `make_range`, `vector<int>{1, 2, 3}` is a temporary
    // constructed while evaluating the `return` statement's expression. It
    // is consumed by `operator|` to build a `filtered_range`, which is then
    // returned BY VALUE out of the lambda. The moment `make_range()`
    // finishes executing — BEFORE control even returns to the caller, and
    // certainly before `auto&& r = make_range();` gets to decide how to
    // bind its result — that temporary `vector<int>` has already been
    // destroyed, because it was local to the function body and its
    // lifetime cannot extend past the function's own execution no matter
    // what the function returns.
    //
    // This is a stronger, more clear-cut case than Bug 0 / Bug 5 / Bug 21:
    // it does not matter one bit whether `r` is declared `auto&&`, `auto`,
    // or `const auto&` — by the time `make_range()` returns, the
    // `filtered_range` object it produced ALREADY contains iterators into
    // memory that no longer belongs to anyone. Reference-binding tricks at
    // the call site can extend the lifetime of the small wrapper OBJECT
    // being returned; they cannot resurrect a stack-allocated buffer that
    // belonged to a function which has already exited. If a function needs
    // to hand back a range, either return an owning container by value (and
    // let the caller adapt it), or take the container by reference so its
    // lifetime is visibly tied to something the caller controls.
    // =========================================================================
    auto make_range = []() {
        return vector<int>{1, 2, 3} | filtered([](int x) { return x > 1; });
    };
    auto&& r = make_range(); // dangling before this line even finishes running — no binding style fixes it

    // =========================================================================
    // BUG 14 — operator[] on an adaptor: sometimes real, never something to
    //          assume — and the "sometimes" is worth understanding precisely
    // =========================================================================
    // This one is more subtle than it looks, and it genuinely is
    // adaptor/version-dependent — read this as "how to reason about it,"
    // not "a fixed rule."
    //
    // `transformed`'s iterator applies the function at the BASE iterator's
    // position; since that's a stateless, 1-to-1 structural mapping
    // (position N in the adapted range always corresponds to position N in
    // `nums`), Boost's `transform_iterator` PRESERVES the base range's
    // traversal category. Over a `vector<int>` (random access), the
    // resulting iterator is random access too, and Boost.Iterator's facade
    // machinery generates a working `operator[]` from that. So
    // `indexed[2]` most likely DOES compile here and returns
    // `nums[2] * nums[2] == 9`.
    //
    // What it does NOT get you is a promise that this keeps working under
    // refactors: swap the underlying container for a `std::list` (only
    // bidirectional), or put a `filtered` stage anywhere before the
    // `transformed`, and `operator[]` either stops compiling or stops being
    // O(1) — `filter_iterator` is structurally incapable of random access
    // (see Bug 24), because you cannot know which element is "2 positions
    // further along among the survivors" without walking and testing every
    // intervening element. There's also a second, independent axis: the
    // value `operator[]` hands back here is a freshly computed PRVALUE
    // `int`, not a reference into `nums` — fine for reading (as here), but
    // it's part of why writing through this kind of access doesn't work
    // (see Bug 1).
    // The safe habit, regardless of which specific case you're in: don't
    // reach for `range[i]`; use `*std::next(boost::begin(range), i)`, which
    // is correct for any iterator category (and degrades to an honest O(n)
    // walk instead of silently doing the wrong thing or failing to
    // compile).
    // =========================================================================
    auto indexed = nums | transformed([](int x) { return x * x; });
    cout << "Indexed access: " << indexed[2] << endl; // likely works here, but only because transformed+vector happens to stay random-access

    // =========================================================================
    // BUG 15 — Writing into a transformed VIEW of an empty vector
    // =========================================================================
    // `boost::range::copy(source, dest)` writes into EXISTING slots
    // starting at `boost::begin(dest)` — it does not insert or grow
    // anything (that's what `back_inserter`/`push_back`-based destinations
    // are for). Two independent problems here:
    //
    //   1. `tmp_vec` is default-constructed and empty, so `tmp_vec |
    //      transformed(...)` is a view over ZERO elements — there is no
    //      room to copy `nums`'s 5 elements into. Depending on exactly how
    //      the range-based overload bounds itself, this either does
    //      nothing (if it correctly checks against `boost::end(dest)`) or
    //      walks straight past an empty destination and writes out of
    //      bounds — undefined behavior, because classic `std::copy`-style
    //      algorithms are driven purely by the SOURCE's length and have no
    //      idea how big the destination actually is.
    //
    //   2. Even with a non-empty `tmp_vec`, "copying into a transformed
    //      view" doesn't mean what it looks like it means. `transformed`'s
    //      iterator computes `x * x` from the underlying element on
    //      dereference; there is no reverse mapping from "the squared value
    //      being assigned" back to "what the original element should
    //      become," so this isn't a meaningful destination for a copy even
    //      in principle — the same fundamental issue as trying to sort
    //      through a transformed view in Bug 1 — unless the transform
    //      function specifically returns a real, writable reference, which
    //      `x * x` does not.
    // =========================================================================
    vector<int> tmp_vec;
    boost::range::copy(nums,
        tmp_vec | transformed([](int x) { return x * x; })
    ); // destination is both empty and not a meaningful place to "write" a squared value into

    // =========================================================================
    // BUG 16 — std::for_each wants (first, last, unary_fn), not (range, binary_fn)
    // =========================================================================
    // Two independent mismatches stacked on one line:
    //
    //   1. Classic `std::for_each` (from <algorithm>, and this file never
    //      pulls in `std::ranges`) has the signature
    //      `for_each(InputIt first, InputIt last, UnaryFunction f)` — it
    //      takes a PAIR OF ITERATORS, not a single range object. Passing
    //      `squares` directly doesn't match any overload, so this fails to
    //      compile with a "no matching function" error before the second
    //      problem even matters.
    //
    //   2. Even patched to `std::for_each(boost::begin(squares),
    //      boost::end(squares), someFn)`, the third argument must be
    //      invocable with exactly ONE argument — `for_each` calls `f(*it)`
    //      for each element, purely for its side effects. `std::greater<int>`
    //      is a BINARY comparator (`bool operator()(const T&, const T&)`),
    //      meant for ordering (e.g. as a comparator for `sort` or a
    //      `priority_queue`), and has no matching single-argument call
    //      operator — another "no matching function" error.
    //
    // If the goal was to print/inspect every squared value, the fix is a
    // real unary callable:
    //     std::for_each(boost::begin(squares), boost::end(squares),
    //                    [](int x){ cout << x << " "; });
    // or, more idiomatically here, a plain range-based for loop.
    // =========================================================================
    std::for_each(squares, std::greater<int>{}); // wrong argument shape AND wrong arity of functor

    // =========================================================================
    // BUG 17 — Dereferencing a find() result without checking for "not found"
    // =========================================================================
    // `nums` is {1,2,3,4,5}; `found_range` filters down to elements greater
    // than 10, which is EMPTY — nothing in `nums` qualifies.
    // `boost::range::find` searching for 11 in an empty range cannot find
    // it, and — exactly like `std::find` — signals "not found" by returning
    // the range's `end()` iterator (here, `boost::end(found_range)`), not
    // by throwing or returning something you can safely ignore.
    //
    // The code dereferences `*pos` unconditionally. `end()` is a sentinel,
    // one past the last valid element (or, here, equal to `begin()` since
    // the range is empty) — it does not refer to a real element, and
    // dereferencing it is undefined behavior, full stop. This is one of the
    // single most common real-world sources of UB in C++: forgetting the
    // `if (pos != boost::end(found_range))` check that every `find`-family
    // result needs before it's used. Always check before you dereference —
    // this isn't "may be end," it definitely is here.
    // =========================================================================
    auto found_range = nums | filtered([](int x) { return x > 10; });
    auto pos = boost::range::find(found_range, 11);
    cout << "Found: " << *pos << endl; // pos == end() here — dereferencing it is UB, not just "maybe wrong"

    // =========================================================================
    // BUG 18 — Calling a Container member function (.erase()) on a view
    // =========================================================================
    // `nums | filtered(...)` produces a temporary `boost::filtered_range<...>`
    // — a lightweight, non-owning WINDOW onto `nums`'s elements, not a
    // container. Views like this (the same is true of C++20's
    // `std::ranges::filter_view`) deliberately do not define `.erase()`,
    // `.insert()`, `.push_back()`, or any other storage-mutating member:
    // they don't own storage, so there is nothing for them, specifically,
    // to erase — the elements physically live in `nums`. This should fail
    // to compile outright with something like "no member named 'erase' in
    // boost::filtered_range<...>" — a hard, immediate compile error, not a
    // subtle runtime bug, which actually makes it one of the SAFER mistakes
    // on this list (the compiler catches it for you). It's included here
    // because confusing "a view onto data" with "a container that owns
    // data" is an extremely common category error for anyone coming from
    // either classic STL container APIs or C++20's `<ranges>` views — the
    // two look similar at a glance (both have `begin()`/`end()`) but
    // support fundamentally different operations. If the goal was to erase
    // the elements matching `x > 3` from `nums` itself, that's
    //     boost::range::remove_erase_if(nums, [](int x){ return x > 3; });
    // called on `nums`, the actual container — not on a view built from it.
    // =========================================================================
    (nums | filtered([](int x) { return x > 3; })).erase( // filtered_range has no .erase() — it's a view, not a container
        nums.begin(), nums.end()
    );

    // =========================================================================
    // BUG 19 — std::vector<T&> is ill-formed, and "just store the adaptors"
    //          has a second problem even once the reference issue is fixed
    // =========================================================================
    // `decltype(squares)` does not give you the `transformed_range<...>`
    // VALUE type here — `squares` was declared as a reference
    // (`const auto& squares = ...` back in Bug 0), and `decltype` applied
    // to the NAME of an entity whose declared type is a reference
    // reproduces that reference type. So `decltype(squares)` is
    // `const boost::transformed_range<Lambda, vector<int>>&` — a REFERENCE
    // type — and `std::vector<T&>` is ill-formed: standard containers
    // require their element type to be an "Erasable" object type
    // (default-constructible / assignable / destructible as a genuine
    // object), and references satisfy none of that (they can't be rebound,
    // default-constructed, or stored the way a container slot needs).
    // Expect a deep, fairly unreadable template-instantiation error, often
    // bottoming out in something like `std::allocator<T&>` being
    // ill-formed.
    //
    // Even setting that aside — imagine `squares` had instead been a
    // plain, non-reference `auto`, so `decltype` gave the VALUE type —
    // storing a bunch of range adaptors in a container is still worth
    // pausing on. Copying one of these lightweight adaptor objects is
    // normally cheap (it just copies a functor and a couple of iterators),
    // but every copy is an independent lazy view that is only as valid as
    // whatever it was built over; stuffing N of them into a `vector`
    // multiplies every lifetime hazard discussed above (Bugs 0, 5, 11–13,
    // 21) by N, now living somewhere less obviously scoped than a single
    // named local variable.
    // =========================================================================
    vector<decltype(squares)> vec_of_ranges; // decltype(squares) is a REFERENCE type — vector<T&> doesn't compile

    // =========================================================================
    // BUG 20 — std::random_shuffle: removed, wrong argument shape, wrong
    //          range type — three independent problems on one line
    // =========================================================================
    //   1. `std::random_shuffle` was deprecated in C++14 and REMOVED
    //      entirely in C++17 (its implicit reliance on `std::rand()` was
    //      considered a design mistake — poor-quality, non-thread-safe,
    //      globally-seeded randomness). Compiled against C++17 or later,
    //      <algorithm> simply doesn't declare this name any more, so this
    //      line may not compile at all depending on which standard the
    //      project targets. The replacement is `std::shuffle(first, last,
    //      urbg)`, which additionally requires an explicit
    //      UniformRandomBitGenerator argument (e.g. a `std::mt19937`).
    //
    //   2. Even on an older standard where the function still exists, its
    //      classic signature takes a PAIR of RandomAccessIterators
    //      (`first, last`), not a single range object — the same category
    //      of mistake as Bug 16's `for_each` call.
    //
    //   3. Even patched to pass iterators, `squares` is const (Bug 0) and
    //      its transform_iterator has no real, writable storage behind it
    //      (Bug 1) — shuffling, like sorting, needs to SWAP elements in
    //      place, and there is nothing here to swap.
    // =========================================================================
    std::random_shuffle(squares); // removed in C++17+, wrong argument shape, and squares isn't mutable/swappable anyway

    // =========================================================================
    // BUG 21 — reversed over a named lvalue: same lifetime-extension story
    //          as Bug 0, included here so the pattern is easy to spot in
    //          the wild
    // =========================================================================
    // `nums | reversed` (note `reversed` is a plain adaptor tag object
    // here, not a function call — Boost.Range lets `operator|` accept it
    // directly) returns a `reversed_range` prvalue wrapping — typically —
    // `std::reverse_iterator`s over `nums`'s own iterators. Bound via
    // `const auto&`, that wrapper's lifetime is extended exactly as in
    // Bug 0, and since `nums` is a durable named object (not a temporary),
    // the iterators it wraps stay valid for as long as `reversed_ref` is
    // used in this function. This specific instance is, in practice,
    // memory-safe.
    //
    // It's labeled "fragile" for the same reason Bug 0 was: this code is
    // structurally identical to the genuinely-dangling patterns in Bug 11
    // and Bug 13, and nothing about reading this line tells you which
    // situation you're in without tracing where `nums` came from. Change
    // `nums` to the result of a function call, move this into a helper that
    // takes its argument by value, or add one more layer of adaptor
    // composition around a temporary, and this flips from "safe by luck"
    // to "dangling" with zero change to the syntax that would warn you.
    // Don't rely on this pattern continuing to be safe just because it is
    // today.
    // =========================================================================
    const auto& reversed_ref = nums | reversed; // safe today because `nums` is a durable lvalue — don't rely on that surviving a refactor
    for (int x : reversed_ref) {
        cout << x << " ";
    }
    cout << endl;

    // =========================================================================
    // BUG 22 — Same missing-adaptor problem as Bug 7, plus: filtered ranges
    //          can't report their own size, so nothing can clamp "drop 100"
    // =========================================================================
    // As in Bug 7, `dropped` is not a real Boost.Range adaptor (see the
    // full list of what's actually in <boost/range/adaptors.hpp> noted
    // there), so this fails to compile before any of the following even
    // becomes relevant. Assuming a working "skip the first N" tool for the
    // sake of the lesson:
    //
    // `nums | filtered(even)` yields {2, 4} — 2 surviving elements out of
    // 5. Asking to drop 100 of them means advancing an iterator 100 steps
    // through a sequence that only has 2. Unlike `vector`, a filtered range
    // cannot answer "how many elements do I have" without walking through
    // and testing the predicate against every element — there's no O(1)
    // `.size()` the way `nums.size()` is for a `vector`; Boost.Range calls
    // this NOT being a "Sized Range." Even a correct, existing "drop"
    // implementation that wanted to safely clamp `N` to the range's actual
    // length would need to pay for that walk-and-count up front, and a
    // naive one — repeatedly incrementing the begin iterator N times, which
    // is how you're forced to advance a merely-Bidirectional
    // `filter_iterator`, see Bug 24 — has nothing stopping it from
    // incrementing straight past `end()`, which is undefined behavior the
    // moment it happens, whether or not you ever read through the result.
    // `*boost::begin(dropped2)` then dereferences whatever that overshoot
    // left behind — likely `end()`, itself independently UB to dereference
    // (same mistake as Bug 17), or worse.
    // =========================================================================
    auto dropped2 = nums | filtered([](int x) { return x % 2 == 0; }) | dropped(100); // `dropped` isn't real, and 100 far exceeds the 2 elements that would survive filtering
    cout << "dropped2 front: " << *boost::begin(dropped2) << endl; // near-certain UB: advances (and then dereferences) past end()

    // =========================================================================
    // BUG 23 — count_if wants a unary predicate; std::greater<int> is binary
    // =========================================================================
    // `boost::range::count_if(range, pred)` calls `pred(element)` once per
    // element and counts how many times it returns true — `pred` must be
    // invocable with exactly ONE argument. `std::greater<int>` is a
    // comparator — `bool operator()(const T& lhs, const T& rhs)` — designed
    // to compare TWO values against each other (e.g. as the ordering used
    // by `std::sort` or `std::priority_queue` to produce a descending
    // order). It has no single-argument overload, so `count_if`'s internal
    // call `pred(x)` has no matching `operator()` to bind to, and this
    // fails to compile — the same underlying mistake as Bug 16 (a binary
    // comparator handed to something that wants a unary function), just
    // against a different algorithm.
    //
    // `std::greater<int>` alone also doesn't carry a "greater than WHAT?"
    // value — it needs to be paired with a fixed comparison value to become
    // a meaningful unary predicate, e.g.
    //     boost::range::count_if(nums, [](int x){ return x > 3; });
    // or, to specifically reuse `std::greater`, partially apply it:
    //     int threshold = 3;
    //     boost::range::count_if(nums, [threshold](int x){ return std::greater<int>{}(x, threshold); });
    // =========================================================================
    auto count = boost::range::count_if(nums, std::greater<int>{}); // std::greater<int> takes two arguments; count_if's predicate takes one

    // =========================================================================
    // BUG 24 — Sorting a filtered range fails on iterator category,
    //          regardless of the temporary-lifetime story
    // =========================================================================
    // `auto&&` binds directly to the `filtered_range` prvalue, so — same as
    // Bugs 0/5/21 — that wrapper object's lifetime IS extended to match
    // `local_range`'s own scope (the end of this block), not "destroyed at
    // the semicolon" as the original comment suggested. `nums` is a durable
    // lvalue, so `local_range` isn't dangling in the memory-unsafety sense
    // either.
    //
    // The much more solid, guaranteed reason this fails: `boost::filter_iterator`
    // caps its traversal at BIDIRECTIONAL no matter how capable the
    // underlying iterator is — straight from Boost's own header
    // (filter_iterator.hpp): the traversal tag is only promoted toward
    // random access if the base iterator's traversal converts to
    // random-access, and even then it is explicitly forced DOWN to
    // `bidirectional_traversal_tag`. This isn't a missed optimization; it's
    // inherent to filtering: you cannot jump directly to "the element 5
    // positions further among the ones that pass the predicate" without
    // walking through and testing every intervening element, so true O(1)
    // random access is mathematically unavailable for a filtered sequence —
    // only forward/backward stepping is. `boost::range::sort` requires
    // RandomAccessIterators (it needs O(1) jumps for its partitioning
    // scheme), so passing a `filtered_range` — this one, or any filtered
    // range over any underlying container — will not compile, full stop,
    // independent of everything else on this line.
    // =========================================================================
    {
        auto&& local_range = nums | filtered([](int x) { return x > 2; });
        boost::range::sort(local_range); // fails to compile: filter_iterator is capped at bidirectional, sort needs random access
    } // (the lifetime story here is a red herring — this line was never going to compile)

    // =========================================================================
    // BUG 25 — Reading a moved-from container, and everything built over its
    //          OLD state earlier in this function
    // =========================================================================
    // `std::move(nums)` performs no move by itself — it is purely a cast to
    // an rvalue reference that makes the following
    // `vector<int> moved_nums = move(nums);` eligible to invoke `vector`'s
    // MOVE constructor instead of its copy constructor. `vector`'s move
    // constructor steals the source's internal heap buffer (a pointer
    // swap, no element copying) and leaves the source in a "valid but
    // unspecified state" — every mainstream standard library leaves a
    // moved-from `vector` EMPTY in practice, but that's an implementation
    // choice you're relying on, not something the standard promises you
    // can depend on for program correctness.
    //
    // Using `nums` again afterward for anything beyond destruction or
    // reassignment (as the trailing comment already flags) is unspecified-
    // behavior territory. The sharper point specific to THIS file: by this
    // point in `main`, a whole series of earlier variables — `squares`,
    // `filtered_squares`, `offsetted`, `temp_range`, `found_range`,
    // `dropped`/`dropped2`, `indexed`, `reversed_ref`, and more — are lazy
    // views that were built directly over `nums` and are STILL IN SCOPE
    // (nothing in `main` has gone out of scope to retire them). The moment
    // `nums` is moved from, every single one of those becomes suspect: each
    // one stored iterator VALUES obtained from `nums` at construction time,
    // and `nums`'s own begin/end have now changed identity, since its old
    // buffer ownership was transferred to `moved_nums`. Whether those stale
    // iterators still "happen" to point at valid memory depends entirely on
    // implementation details of the move (in practice, often yes, because
    // the buffer itself typically isn't freed, just re-owned by
    // `moved_nums`) — but relying on that is exactly the same category of
    // mistake as everything else in this file: it might work today, on
    // this compiler, with this standard library, and that is not the same
    // thing as being correct. Treat any range/iterator/adaptor built over a
    // container's old state as invalid the instant that container is moved
    // from.
    // =========================================================================
    vector<int> moved_nums = move(nums);
    auto moved_squares = moved_nums | transformed([](int x) { return x * x; });
    // If nums is moved from and later used, behavior is undefined — and by this point in
    // main(), nums has already been captured by several earlier lazy views built above.

    return 0;
}
