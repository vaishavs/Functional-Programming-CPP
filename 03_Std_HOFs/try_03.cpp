// ===========================================================================
// std::ranges — algorithms, views and adaptors: TODO/FIXME exercise
//
// Markers
//   TODO(n)     — implement from scratch.
//   FIXME(n)    — shipped code is wrong on purpose; the tests convict it.
//   PREDICT     — commit to an answer before compiling.
//   QUESTION    — answer in a comment next to the marker.
//   EXPERIMENT  — change something, observe, restore.
//
// Flip a step's STEPn_READY to 1 when you finish it. Steps are cumulative:
// enable them in order. Ships compiling and running with all steps off.
//
//   Build : g++ -std=c++23 -Wall -Wextra -o ranges ranges_views_todo.cpp
//   Run   : ./ranges
//
// Toolchain notes
//   * C++23 is required (zip, enumerate, chunk, chunk_by, slide, adjacent,
//     stride, cartesian_product, join_with, fold_left).
//   * std::ranges::to is C++23 but NOT in GCC 13 — hence TODO(1a), which
//     hand-rolls it. If your compiler has it, feel free to compare notes.
//   * Stub parameters are commented out (`int /*n*/`) so the shipped file
//     is warning-free. Restore the names as you implement.
//
// Conventions used by every test
//   * Nothing prints. Results are returned and compared with ==.
//   * Views are lazy; a test that compares values has always materialised.
// ===========================================================================

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rg = std::ranges;
namespace rv = std::views;

#define STEP1_READY 0
#define STEP2_READY 0
#define STEP3_READY 0
#define STEP4_READY 0
#define STEP5_READY 0

// ---------------------------------------------------------------------------
// Shared fixtures
// ---------------------------------------------------------------------------
struct Rec {
    std::string name;
    std::string dept;
    int score;
    friend bool operator==(Rec const&, Rec const&) = default;
};

inline std::vector<Rec> sample_recs() {
    return {{"ann", "eng", 90},
            {"bob", "eng", 72},
            {"cid", "ops", 55},
            {"dee", "ops", 88},
            {"eve", "eng", 64}};
}

// Instrumentation for FIXME(C). square() bumps the counter on every call.
inline int square_calls = 0;
inline int square(int x) {
    ++square_calls;
    return x * x;
}

// ===========================================================================
// STEP 1 — materialise, then build your first pipelines
// ===========================================================================
//
// TODO(1a)  to_vector(r) -> std::vector<range_value_t<R>>
//   Copy any input range into a fresh vector.
//   * rg::copy(r, std::back_inserter(out)).
//   * Element type: rg::range_value_t<R>. (Not range_reference_t — for
//     zip that would be a tuple of references.)
//   * The parameter MUST be R&&, not R const&. QUESTION(1a): why? Try
//     R const& and pass a filter view; read the error. (filter_view has
//     no const begin(): its cached begin is mutable state. transform_view
//     is const-iterable, so the mistake hides until someone filters.)
//
// TODO(1b)  evens_squared(data) -> std::vector<int>
//   Squares of the even elements, at most the first 3 of them.
//   Adaptors: rv::filter, rv::transform, rv::take. Materialise with (1a).
//
// TODO(1c)  squares_below(limit) -> std::vector<int>
//   1, 4, 9, ... — every positive square strictly below limit.
//   * Start from the INFINITE rv::iota(1). No bound, no end iterator.
//   * rv::take_while(pred) stops at the first failure.
//   QUESTION(1c): why does this terminate, and why would rv::filter in
//   place of take_while hang forever?
//
// TODO(1d)  as_sv(token) -> std::string_view
//   rv::split hands you a subrange of chars, not a string_view. Convert.
//   * std::string_view(token.begin(), token.end()) works everywhere.
//   * C++23 also allows the direct range constructor: std::string_view(t).
//   Keep it a template — split tokens have an unspellable type.
// ---------------------------------------------------------------------------
template <class R>
auto to_vector(R&& /*r*/) {
    // TODO(1a)
}

auto evens_squared(std::vector<int> const& /*data*/) {
    // TODO(1b)
}

auto squares_below(int /*limit*/) {
    // TODO(1c)
}

template <class Tok>
auto as_sv(Tok&& /*token*/) {
    // TODO(1d)
}

// ===========================================================================
// STEP 2 — algorithms with projections
// ===========================================================================
//
// Every rg:: algorithm takes (range, comparator = {}, projection = {}).
// The projection is applied to each element BEFORE the comparator or
// predicate sees it, so `&Rec::score` replaces a wrapping lambda. `{}`
// keeps the default comparator (rg::less).
//
// TODO(2a)  names_sorted_by_score_desc(recs) -> std::vector<std::string>
//   Names, highest score first.
//   * rg::sort(v, std::greater<>{}, &Rec::score) — note the projection
//     slot is third, so the comparator must be spelled explicitly.
//   * Take recs BY VALUE: sort mutates, the caller's copy must not move.
//   * Then project to names with rv::transform(&Rec::name) + to_vector.
//
// TODO(2b)  top_scorer(recs) -> std::string
//   * rg::max_element(recs, {}, &Rec::score) returns an ITERATOR. Deref.
//
// TODO(2c)  count_in_dept(recs, dept) -> std::ptrdiff_t
//   * rg::count(range, value, projection) — no predicate needed.
//
// TODO(2d)  first_below(recs, threshold) -> std::optional<std::string>
//   First record (original order) scoring strictly below threshold.
//   * rg::find_if(recs, pred, &Rec::score); the predicate then takes an
//     int, not a Rec.
//   * Compare the result against rg::end(recs) before dereferencing;
//     return std::nullopt when not found.
//
// TODO(2e)  all_at_least(recs, threshold) -> bool
//   * rg::all_of with a projection.
//
// TODO(2f)  partition_passing(recs, threshold) -> std::pair<std::vector<Rec>, std::ptrdiff_t>
//   Reorder so records scoring >= threshold come first, RELATIVE ORDER
//   PRESERVED inside both groups, and report how many passed.
//   * rg::stable_partition (not rg::partition — that one may shuffle).
//   * It returns a subrange marking the failing tail; the pass count is
//     the distance from the beginning to that subrange's begin.
// ---------------------------------------------------------------------------
auto names_sorted_by_score_desc(std::vector<Rec> /*recs*/) {
    // TODO(2a)
}

auto top_scorer(std::vector<Rec> const& /*recs*/) {
    // TODO(2b)
}

auto count_in_dept(std::vector<Rec> const& /*recs*/, std::string_view /*dept*/) {
    // TODO(2c)
}

auto first_below(std::vector<Rec> const& /*recs*/, int /*threshold*/) {
    // TODO(2d)
}

auto all_at_least(std::vector<Rec> const& /*recs*/, int /*threshold*/) {
    // TODO(2e)
}

auto partition_passing(std::vector<Rec> /*recs*/, int /*threshold*/) {
    // TODO(2f)
}

// ===========================================================================
// STEP 3 — four defects. Predict each verdict, then repair.
// ===========================================================================
//
// FIXME(A) — the erase-remove trap.
//   drop_odds should leave only the even values. It does not.
//   PREDICT the size of the returned vector for {1,2,3,4,5,6} before
//   running. rg::remove_if does not resize anything: it shuffles the
//   survivors to the front and returns a SUBRANGE covering the junk tail.
//   Repair: erase that tail. The subrange's .begin()/.end() are the
//   arguments vector::erase wants.
//
// FIXME(B) — the dangling trap. THIS ONE CONVICTS AT COMPILE TIME.
//   best_score() calls an algorithm on a temporary container. Algorithms
//   returning iterators refuse to hand back an iterator into a range that
//   died at the semicolon: for a non-borrowed rvalue they return
//   rg::dangling instead. Enable step 3 and read the error at the assert.
//   Repair: give the container a name so it outlives the iterator.
//   QUESTION(B): rvalue containers piped into a view are safe
//   (make_scores() | rv::transform(f) owns its vector via owning_view).
//   Why is a raw iterator different?
//
// FIXME(C) — the re-evaluation trap. Views do not memoise.
//   expensive_evens squares the values and keeps the even squares. The
//   answer is right; the cost is not. For the 8 inputs in the test,
//   square() runs TWELVE times. PREDICT where the extra calls come from
//   before reading on: filter_view's iterator evaluates its predicate to
//   locate the next match — which runs the transform — and the transform
//   runs AGAIN when you dereference, because nothing was cached.
//   Repair: a pure reordering. x*x is even exactly when x is even, so the
//   predicate can move upstream of the transform and test the cheap input
//   instead. Filter first, transform the 4 survivors: 4 calls, same
//   answer. Recognising that a predicate can be pushed upstream is the
//   whole skill — pipelines are not automatically reassociated for you.
//   EXPERIMENT(C): after repairing, swap the order back and confirm 12.
//   Then PREDICT the count for the repaired pipeline with a second
//   traversal added (two range-for loops over the same view), and check.
//
// FIXME(D) — the stale-view trap.
//   peek_bump_peek: read the first even, set v[0] = 100 (now even), read
//   the first even again. PREDICT the pair before running.
//   filter_view caches begin() on first use — it is allowed to, since
//   finding the first match is O(n) and views are expected to be cheap to
//   re-traverse. After the container changes, that cache is stale.
//   Repair: build a fresh view over the container after mutating it. A
//   view is a cursor over data, not a live query.
// ---------------------------------------------------------------------------
inline std::vector<int> drop_odds(std::vector<int> v) {
    // FIXME(A): the return value is the whole point.
    rg::remove_if(v, [](int x) { return x % 2 != 0; });
    return v;
}

inline std::vector<int> make_scores() { return {41, 97, 63}; }

inline auto best_score() {
    // FIXME(B): make_scores() dies at the end of this expression.
    auto it = rg::max_element(make_scores());
    return it;
}

inline std::vector<int> expensive_evens(std::vector<int> const& data) {
    // FIXME(C): correct answer, 3x the calls.
    std::vector<int> out;
    for (int x : data | rv::transform(square)
                      | rv::filter([](int s) { return s % 2 == 0; }))
        out.push_back(x);
    return out;
}

inline std::pair<int, int> peek_bump_peek(std::vector<int>& v) {
    auto evens = v | rv::filter([](int x) { return x % 2 == 0; });
    int before = *evens.begin();
    v[0] = 100;
    // FIXME(D): `evens` cached its begin() during the read above.
    int after = *evens.begin();
    return {before, after};
}

// ===========================================================================
// STEP 4 — the multi-range adaptors
// ===========================================================================
//
// Each returns a materialised vector, so pipe then call to_vector.
// Structured bindings on the adaptor's element are the ergonomic move:
//   rv::transform([](auto p) { auto [a, b] = p; return ...; })
//
// TODO(4a)  zip_labels(names, scores) -> std::vector<std::string>
//   "ann=90", ... Stops at the shorter range. rv::zip.
//
// TODO(4b)  numbered(items) -> std::vector<std::string>
//   "0:x", "1:y", ... rv::enumerate yields (index, element); the index is
//   a ptrdiff_t, so std::to_string is happy.
//
// TODO(4c)  deltas(series) -> std::vector<int>
//   Successive differences: {3,7,2,9} -> {4,-5,7}.
//   rv::adjacent<2> (alias rv::pairwise) yields overlapping pairs.
//   Empty or single-element input yields an empty result — check that
//   your version does not underflow.
//
// TODO(4d)  batches(v, n) -> std::vector<std::vector<int>>
//   Consecutive groups of n; the last may be short. rv::chunk.
//   Each chunk is itself a range: to_vector twice (inner then outer).
//
// TODO(4e)  run_lengths(v) -> std::vector<std::pair<int, int>>
//   {1,1,2,2,2,3} -> {{1,2},{2,3},{3,1}} as (value, count).
//   rv::chunk_by(pred) starts a new group when pred(prev, curr) is false,
//   so std::equal_to<>{} groups equal neighbours. Group size via
//   rg::distance; the value via .front().
//
// TODO(4f)  every_kth(v, k) -> std::vector<int>
//   Always includes the first element. rv::stride.
//
// TODO(4g)  flatten(nested) -> std::vector<int>
//   One level down. rv::join.
//
// TODO(4h)  grid(xs, ys) -> std::vector<std::string>
//   All combinations, LAST range varying fastest: {1,2} x {'x','y'} ->
//   "1x","1y","2x","2y". rv::cartesian_product.
//
// QUESTION(4): which of chunk / chunk_by / slide / stride can still be a
// random_access_range, and which of these views can you iterate through a
// const reference? (Both answers matter the moment you pass one to a
// generic helper.)
// ---------------------------------------------------------------------------
auto zip_labels(std::vector<std::string> const& /*names*/,
                std::vector<int> const& /*scores*/) {
    // TODO(4a)
}

auto numbered(std::vector<std::string> const& /*items*/) {
    // TODO(4b)
}

auto deltas(std::vector<int> const& /*series*/) {
    // TODO(4c)
}

auto batches(std::vector<int> const& /*v*/, int /*n*/) {
    // TODO(4d)
}

auto run_lengths(std::vector<int> const& /*v*/) {
    // TODO(4e)
}

auto every_kth(std::vector<int> const& /*v*/, int /*k*/) {
    // TODO(4f)
}

auto flatten(std::vector<std::vector<int>> const& /*nested*/) {
    // TODO(4g)
}

auto grid(std::vector<int> const& /*xs*/, std::vector<char> const& /*ys*/) {
    // TODO(4h)
}

// ===========================================================================
// STEP 5 — capstone: parse, group, aggregate, format
// ===========================================================================
//
// TODO(5a)  parse(doc) -> std::vector<Rec>
//   doc is newline-separated "name,dept,score" lines with a trailing
//   newline. Parse every line, including the malformed-score one (-1
//   parses fine; step 5b filters it out later).
//   * Outer rv::split('\n'), inner rv::split(','); as_sv from TODO(1d)
//     turns tokens into string_views.
//   * SKIP EMPTY LINES: splitting "a\nb\n" on '\n' yields a trailing
//     empty token. Forgetting this costs you an out-of-range field access.
//   * Integers: std::from_chars(sv.data(), sv.data() + sv.size(), out).
//     Handles the leading '-'.
//   * Rec holds std::strings; the views point into doc, so construct
//     std::string from each string_view.
//
// TODO(5b)  report(recs) -> std::string
//   Drop records with a negative score, then one line per department in
//   ascending department order:
//       "dept: <integer average> (<count>)"
//   joined by '\n', with NO trailing newline.
//   * Filter, materialise, then rg::sort with projection &Rec::dept —
//     chunk_by only groups ADJACENT equals, so sorting first is what
//     turns it into a group-by.
//   * Group: rv::chunk_by comparing the dept of neighbours.
//   * Per group: count with rg::distance; total with
//     rg::fold_left(grp | rv::transform(&Rec::score), 0, std::plus<>{}).
//     Integer division truncates — the expected values assume that.
//   * Assemble the lines, then rv::join_with('\n') to stitch them. That
//     yields a range of CHARS, so materialise into a std::string (a plain
//     loop, or to_vector then construct).
// ---------------------------------------------------------------------------
inline constexpr std::string_view kDoc =
    "ann,eng,90\n"
    "bob,eng,72\n"
    "cid,ops,55\n"
    "dee,ops,88\n"
    "eve,eng,-1\n"
    "gus,ops,91\n";

auto parse(std::string_view /*doc*/) {
    // TODO(5a)
}

auto report(std::vector<Rec> const& /*recs*/) {
    // TODO(5b)
}

// ===========================================================================
// Tests — do not modify. Each assert names the mistake it catches.
// ===========================================================================
int main() {
    // ---- step 1 ----------------------------------------------------------
#if STEP1_READY
    {
        std::vector<int> data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        assert((to_vector(data | rv::take(3)) == std::vector<int>{1, 2, 3}));

        // to_vector must accept a filter view — catches an R const& parameter
        auto odd = data | rv::filter([](int x) { return x % 2 != 0; });
        assert((to_vector(odd) == std::vector<int>{1, 3, 5, 7, 9}));

        // and a view whose value type is not its reference type
        auto pairs = to_vector(rv::zip(data, data));
        assert(pairs.size() == 10 && pairs[2].first == 3);

        assert((evens_squared(data) == std::vector<int>{4, 16, 36}));
        assert((evens_squared({1, 3, 5}) == std::vector<int>{}));

        // infinite source, bounded by take_while — hangs if you used filter
        assert((squares_below(30) == std::vector<int>{1, 4, 9, 16, 25}));
        assert((squares_below(1) == std::vector<int>{}));

        auto toks = std::string_view{"ab,cd"} | rv::split(',');
        assert(as_sv(*toks.begin()) == "ab");
        std::cout << "step 1  materialise + pipelines  ok\n";
    }
#else
    std::cout << "step 1  materialise + pipelines  TODO (flip STEP1_READY)\n";
#endif

    // ---- step 2 ----------------------------------------------------------
#if STEP2_READY
    {
        auto recs = sample_recs();

        assert((names_sorted_by_score_desc(recs)
                == std::vector<std::string>{"ann", "dee", "bob", "eve", "cid"}));
        // taken by value — catches a signature that sorts the caller's data
        assert(recs == sample_recs());

        assert(top_scorer(recs) == "ann");
        assert(count_in_dept(recs, "eng") == 3);
        assert(count_in_dept(recs, "hr") == 0);

        assert(first_below(recs, 70) == std::optional<std::string>{"cid"});
        assert(first_below(recs, 50) == std::nullopt);  // catches a missing
                                                        // end() check
        assert(all_at_least(recs, 50));
        assert(!all_at_least(recs, 60));

        auto [ordered, passed] = partition_passing(recs, 70);
        assert(passed == 3);
        // relative order kept in BOTH groups — catches rg::partition
        assert((to_vector(ordered | rv::transform(&Rec::name))
                == std::vector<std::string>{"ann", "bob", "dee", "cid", "eve"}));
        std::cout << "step 2  algorithms + projections ok\n";
    }
#else
    std::cout << "step 2  algorithms + projections TODO (flip STEP2_READY)\n";
#endif

    // ---- step 3 ----------------------------------------------------------
#if STEP3_READY
    {
        // FIXME(A): remove_if alone resizes nothing
        assert((drop_odds({1, 2, 3, 4, 5, 6}) == std::vector<int>{2, 4, 6}));
        assert((drop_odds({1, 3}) == std::vector<int>{}));

        // FIXME(B): compile error until the container outlives the iterator
        assert(best_score() == 97);

        // FIXME(C): same answer, 4 calls instead of 12
        square_calls = 0;
        assert((expensive_evens({1, 2, 3, 4, 5, 6, 7, 8})
                == std::vector<int>{4, 16, 36, 64}));
        assert(square_calls == 4);

        // FIXME(D): the second read must see the mutation
        std::vector<int> v{1, 2, 3, 4, 5, 6};
        assert((peek_bump_peek(v) == std::pair{2, 100}));
        std::cout << "step 3  four traps repaired ..... ok\n";
    }
#else
    std::cout << "step 3  four traps repaired ..... TODO (flip STEP3_READY)\n";
#endif

    // ---- step 4 ----------------------------------------------------------
#if STEP4_READY
    {
        std::vector<std::string> names{"ann", "bob", "cid"};
        std::vector<int> scores{90, 72};

        // zip stops at the shorter range — catches indexing by the longer
        assert((zip_labels(names, scores)
                == std::vector<std::string>{"ann=90", "bob=72"}));

        assert((numbered({"x", "y"})
                == std::vector<std::string>{"0:x", "1:y"}));

        assert((deltas({3, 7, 2, 9}) == std::vector<int>{4, -5, 7}));
        assert((deltas({5}) == std::vector<int>{}));  // catches an n-1 underflow

        assert((batches({1, 2, 3, 4, 5, 6, 7, 8}, 3)
                == std::vector<std::vector<int>>{{1, 2, 3}, {4, 5, 6}, {7, 8}}));

        assert((run_lengths({1, 1, 2, 2, 2, 3})
                == std::vector<std::pair<int, int>>{{1, 2}, {2, 3}, {3, 1}}));

        assert((every_kth({1, 2, 3, 4, 5, 6, 7, 8}, 3)
                == std::vector<int>{1, 4, 7}));

        assert((flatten({{1, 2}, {}, {3}, {4, 5}})
                == std::vector<int>{1, 2, 3, 4, 5}));

        // last range varies fastest — catches swapped loops
        assert((grid({1, 2}, {'x', 'y'})
                == std::vector<std::string>{"1x", "1y", "2x", "2y"}));
        std::cout << "step 4  multi-range adaptors ... ok\n";
    }
#else
    std::cout << "step 4  multi-range adaptors ... TODO (flip STEP4_READY)\n";
#endif

    // ---- step 5 ----------------------------------------------------------
#if STEP5_READY
    {
        auto recs = parse(kDoc);
        assert(recs.size() == 6);  // catches a swallowed trailing empty line
        assert((recs[0] == Rec{"ann", "eng", 90}));
        assert((recs[4] == Rec{"eve", "eng", -1}));  // negatives parse
        assert((recs[5] == Rec{"gus", "ops", 91}));

        // eng: (90+72)/2 = 81 over 2 records; ops: (55+88+91)/3 = 78 over 3
        assert(report(recs) == "eng: 81 (2)\nops: 78 (3)");

        // no trailing newline, and a single group still works
        assert(report({{"zoe", "hr", 70}, {"yan", "hr", 75}}) == "hr: 72 (2)");
        assert(report({{"bad", "hr", -5}}) == "");
        std::cout << "step 5  capstone report ........ ok\n";
    }
#else
    std::cout << "step 5  capstone report ........ TODO (flip STEP5_READY)\n";
#endif

    std::cout << "\ndone\n";
    return 0;
}
