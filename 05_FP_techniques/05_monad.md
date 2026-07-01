# Monads in C++ (the Box Model)

A monad is a box that can chain computations while handling context internally. Each step receives the previous value and produces a brand-new box, which the monad automatically flattens back to a single layer.

With the earlier levels, the function handed in always returned a plain value:
* functor: function takes a value, returns a plain value
* applicative: same, just across several boxes

The signature contrast across all three levels is the entire story:  
```
   Functor      transform :   (A → B)    →  F<A>  →  F<B>
                              └ plain ┘                       function OUTSIDE the box

   Applicative  ap        :  F<A → B>    →  F<A>  →  F<B>
                              └─ box ─┘                       function INSIDE a box

   Monad        bind      :  (A → F<B>)  →  F<A>  →  F<B>
                              └ returns a BOX ┘                input is UNWRAPPED,
                                                              continuation BUILDS the box

```
```
   ┌─────────────────────────────────────────────────────────────┐
   │ Functor                         transform   (A→B lifted)    │
   │  ┌──────────────────────────────────────────────────────┐   │
   │  │ Applicative       pure + φ / ap   (n boxes → one)    │   │
   │  │  ┌────────────────────────────────────────────────┐  │   │
   │  │  │ Monad      pure(η) + join(μ) / bind            │  │   │ ← here
   │  │  │            value → next box's shape            │  │   │
   │  │  └────────────────────────────────────────────────┘  │   │
   │  └──────────────────────────────────────────────────────┘   │
   └─────────────────────────────────────────────────────────────┘
     every monad is an applicative is a functor; the converses all fail.

```
Sometimes, the natural function **itself produces a box**. Mapping such a function gives a *nested box*, and neither `transform` nor `ap` can flatten it:

```
   f : T → F<U>      (a function that returns a BOX)

   ┌──────┐                            ┌────────────┐
   │ F<T> │ ──── transform / ap ────▶  │  F< F<U> > │
   └──────┘                            └────────────┘
                                    nested box — stuck!
```

Example: Consider the statement "for the number n, give me the list 1 through n". That function returns a *list* — a box. If that is run over a box, the result is a *box inside a box*, which is almost never what is wanted.

```
   start:  [1, 2, 3]                    a box of numbers

   apply  n → [1..n]  to each:

           [ [1], [1,2], [1,2,3] ]      a box of boxes  ← nested, awkward
```

A monad's job is to flatten that nesting back to a single box:

```
           [ [1], [1,2], [1,2,3] ]      box of boxes
                     │
                  flatten
                     ▼
           [ 1, 1,2, 1,2,3 ]            one flat box  ← what is wanted
```

## The key difference from an applicative
Consider a function that itself returns a box (because it branches, reads state, is async, has a failed step, etc.). Mapping such a function gives a *nested box*, and neither `transform` nor `ap` can flatten it. In an applicative, the boxes are **independent** — all known up front, none depends on what's inside another. In a monad, each step can look at the **actual value** that came out of the previous step and decide what to do next.

```
   APPLICATIVE — boxes fixed in advance, independent:

        box A    box B    box C        all known before running
          └────────┼────────┘

   MONAD — each step depends on the previous result:

        box A ──► (look at its value) ──► choose box B
                                          └► (look at ITS value) ──► choose box C
```

That dependence — "decide the next box based on the value that just came out" — is the extra power, and the reason the function returns a box instead of a plain value.

Since `bind`'s function `A → F<B>` receives an actual value and uses it to produce the next box, two things follow inevitably:

```
 APPLICATIVE  (independent)              MONAD  (dependent)
   all boxes built up front                box A ─► value a
   then combined                                │  (a decides the next box)
                                                ▼
   • parallelizable                        f(a) ─► box B ─► value b
   • can accumulate ALL errors                  │
                                                ▼
                                           g(b) ─► box C
                                           • SEQUENTIAL (each step needs the prior value)
                                           • SHORT-CIRCUITS at the first empty/failed box
```

- **Sequencing is mandatory.** The next step cannot be started until the previous step's value exists, because the latter step is *built from* that value. Monadic chains are inherently ordered.

- **Short-circuiting is automatic.** If an intermediate box is "empty/failed," there is no value to feed the next function, so the remainder of the chain is skipped — the opposite of the applicative's run-everything-and-accumulate behavior.


## The operations

A monad has **two core operations** (plus the ones inherited from functor and applicative, since every monad is also those).

#### Operation 1 — `pure` (wrap): plain value into a box

The same wrap operation seen at every level. One bare value in, one box out.

```
   ┌─────┐              ┌─────────┐
   │  5  │  ── pure ──► │ [  5  ] │
   └─────┘              └─────────┘
```

#### Operation 2 — `bind`: the defining operation

This is what *makes* it a monad. `bind` takes a box and a function that returns **another box**, applies it, and flattens the result to a single box — all in one step.

```
   ┌───────────┐     function:  value → BOX
   │  box of A │            (returns a box, not a plain value)
   └───────────┘
        │
        │  bind:  open the box, run the function on the contents,
        │         then flatten the box-of-boxes into one box
        ▼
   ┌───────────┐
   │  box of B │     ← single box, already flattened
   └───────────┘
```
In C++, it looks like this:
```cpp
template<
    template<typename> class Context,
    typename FunctionReturningContext,
    typename Input
>
auto bind(
    Context<Input> value,
    FunctionReturningContext fn
);
// fn: [](Input x) -> Context<Output> { ... }
// Input -> Context<Output>
```
Meaning:
```
    Context<Input>
    -> (Input -> Context<Output>)
    -> Context<Output>
```
The function's return value is wrapped: `Input -> Context<Output>`. The input it receives is bare; the box it hands back is what makes it a monad. Because the function returns `Context<Output>`, naively running it would give `Context<Context<Output>>` — a doubled wrapper — so `bind` also needs to flatten one layer. This flattening operation is called `join` (`Context<Context<T>> -> Context<T>`).
```
bind(c, f)  =  join( transform(f, c) )
                     └ transform gives Context<Context<Output>> ┘
                  └ join flattens to Context<Output> ────────┘


        F< F<A> >                                              F<A>
   ┌────────────────────┐                                 ┌──────────────┐
   │   ┌───────────┐    │              join               │              │
   │   │   F<A>    │    │   ───────────────────────────►  │     ...      │
   │   │  ┌─────┐  │    │     dissolve the outer box      │              │
   │   │  │  a  │  │    │      into the inner one         └──────────────┘
   │   │  └─────┘  │    │
   │   └───────────┘    │
   └────────────────────┘
        two layers                                            one layer
```

However, in C++, there's **no** single operator called `bind`; it is called **`transform` then `join`** for ranges. The `transform` (`std::views::transform`) makes the box-of-boxes, and `join` (`std::views::join`) flattens it. 

```
   f : T → F<U>      (a function that returns a BOX)

   step 1: transform f               step 2: join
   ┌──────┐                          ┌────────────┐               ┌──────┐
   │ F<T> │ ──── transform(f) ────▶  │  F< F<U> > │ ──── join ──▶ │ F<U> │
   └──────┘                          └────────────┘               └──────┘
                                    nested box — stuck!        flatten one layer

   bind(m, f)  ==  join(transform(f, m))
```

## The monad laws  
A monad must satisfy three laws, the analogues of the functor and applicative laws at this level, and they are most transparently stated through bind and pure. 

* Left identity: `bind(pure(a), f) = f(a)` — injecting a value and immediately binding is just calling the continuation, so pure adds no structure on the way in. 
* Right identity: `bind(m, pure) = m` — binding `pure` changes nothing. 
* Associativity: `bind(bind(m, f), g) = bind(m, [](x){ return bind(f(x), g); })` — the order of grouping a chain of binds does not matter.

```
   Associativity — regrouping a chain of binds is safe:

      (m  ▷  f)  ▷  g     ==     m  ▷  ( x ↦ f(x) ▷ g )
       └ left-grouped ┘            └ right-grouped ┘            both equal

```
Together, these guarantee that long chains like `bind(bind(bind(m, f), g), h)` are unambiguous and behave like a single, well-defined sequence.

As at every prior level, these are a contract the compiler never checks, and they presuppose *pure* continuations. C++ views are lazy, so a continuation is re-invoked on each access and an impure function (mutating, printing, time-dependent) runs an unpredictable number of times; and a joined view that refers to a temporary range *dangles*. The guarantees are only as clean as the purity and lifetimes of what is fed in.

## The Boxes
In C++, "monadic boxes" refers to wrapper types that hold a value (or the absence/multiplicity of one) and expose composable operations like `transform` (map), `and_then` (bind/flatMap), and `or_else`. Here's a breakdown by family.

#### 1. Maybe / Optional family — `std::optional<T>`
Models "a value that might not exist." No exceptions, no null pointers.
- **Operations (C++23):** `.transform(f)`, `.and_then(f)`, `.or_else(f)`, `.value_or(default)`
- **Context:** parsing results, lookups, optional config fields

#### 2. Either / Result family — `std::expected<T, E>` (C++23)
Models "a value or an error," carrying the error type instead of throwing.
- **Operations:** `.transform(f)`, `.and_then(f)`, `.transform_error(f)`, `.or_else(f)`
- **Context:** fallible operations (file I/O, parsing) without exceptions

#### 3. Sum-type family — `std::variant<Ts...>`
Models "one of several possible types." Not monadic out of the box, but the substrate others (like `expected`) are built on.
- **Operations:** `std::visit` (acts like a pattern-matching bind)
- **Context:** state machines, tagged unions, AST nodes

#### 4. Async / Future family — `std::future<T>`, `std::shared_future<T>`, coroutines
Models "a value that will exist later." Coroutines (`co_await`) give this true monadic bind syntax.
- **Operations:** `.get()` (blocking), `co_await` (suspending bind), `std::async`
- **Context:** concurrency, async pipelines, C++20 coroutine-based generators/tasks

#### 5. Sequence / List family — Ranges (`std::ranges`, views)
Models "zero or more values," with lazy composition — closest thing C++ has to the List monad.
- **Operations:** `views::transform` (map), `views::join` (flatten), `views::filter`
- **Context:** lazy pipelines, `xs | views::transform(f) | views::join` ≈ `bind`

#### 6. Ownership / Reference-counted box family — `unique_ptr<T>`, `shared_ptr<T>`
Not monadic in the algebraic sense (no standard `map`/`bind`), but conceptually "a box around a value" with lifetime semantics.
- **Operations:** `*`, `->`, custom deleters; no built-in `transform`
- **Context:** RAII, ownership modeling

#### 7. Type-erased box family — `std::any`, `std::function`
Boxes that erase static type information.
- **Operations:** `std::any_cast`, invocation (`operator()`)
- **Context:** heterogeneous containers, callback storage

##### Summary Table

| Family | Type | Models | Key Ops | Added In | Typical Use |
|---|---|---|---|---|---|
| Maybe | `std::optional<T>` | value or nothing | `transform`, `and_then`, `or_else`, `value_or` | C++17 (ops in C++23) | optional/nullable data |
| Either/Result | `std::expected<T,E>` | value or typed error | `transform`, `and_then`, `transform_error`, `or_else` | C++23 | error handling w/o exceptions |
| Sum type | `std::variant<Ts...>` | one of several types | `std::visit` | C++17 | tagged unions, ASTs |
| Async | `std::future<T>` / coroutines | value available later | `get()`, `co_await` | C++11 / C++20 | concurrency, async tasks |
| List/Sequence | Ranges/views | zero or more values | `transform`, `join`, `filter` | C++20 | lazy sequence pipelines |
| Ownership | `unique_ptr`/`shared_ptr` | owned resource | `*`, `->` | C++11 | RAII, lifetime mgmt |
| Type-erased | `std::any`, `std::function` | erased type/callable | `any_cast`, `operator()` | C++17 / C++11 | heterogeneous storage, callbacks |

The two that behave most like textbook monads (with real `map`/`bind` chaining) are **`std::optional`** and **`std::expected`** — they're the ones people usually mean when they talk about "monadic boxes" in modern C++.

## Examples and use cases
#### Example 1 — The Identity Box (show the gears)

Extending the single-value `Box<T>` to a monad shows the mechanics with nothing else in the way. For a box that holds exactly one value, `bind` is very simple — unwrap, apply, and the result is already a single box (no nesting to flatten, because `f` hands back a `Box<B>` directly).

```cpp
#include <utility>   // std::move

template <typename T>
class Box {
    T value_;
public:
    explicit Box(T v) : value_(std::move(v)) {}

    template <typename F>                          // FUNCTOR
    auto transform(F f) const {
        return Box<decltype(f(value_))>( f(value_) );   // Box of whatever f returns
    }
    const T& get() const { return value_; }
};

template <typename T>                              // pure / return
Box<T> pure_box(T v) { return Box<T>(v); }

// MONAD: bind  --  f : A -> Box<B>, so the result already IS Box<B>
template <typename A, typename F>
auto bind(const Box<A>& m, F f) {
    return f(m.get());
}

// join, for completeness:  Box<Box<A>> -> Box<A>
template <typename A>
Box<A> join(const Box<Box<A>>& bb) { return bb.get(); }
```

Usage — chaining value-dependent steps, each returning a *new* box:

```cpp
auto half      = [](int x){ return Box<int>{ x / 2 }; };          // int -> Box<int>
auto increment = [](int x){ return Box<int>{ x + 1 }; };          // int -> Box<int>

Box<int> r = bind( bind( Box<int>{20}, half ), increment );        // Box<int> 11
//                        20 → Box{10}      10 → Box{11}
```

For the Identity box, `bind` just hands the value to the function and returns the result. It shows the bare plumbing, with no behavior of its own — that starts only once the box has structure.

#### Example 2 — `std::vector` (the list monad)

A vector is a box of many values, read as "a value that could be any one of these". Its `bind` is the famous `flatMap` / `concatMap`: apply `f` (which returns a vector) to every element, then concatenate all the resulting vectors. Crucially, `f` may return vectors of different lengths depending on the input value. This value-dependence is precisely what makes it a monad.

```cpp
#include <vector>

// MONAD bind for vectors: map each element to a vector, then concatenate
template <typename A, typename F>
auto bind(const std::vector<A>& xs, F f) {
    decltype(f(xs.front())) out;                       // f returns vector<B>; start empty
    for (const auto& x : xs) {
        auto ys = f(x);
        out.insert(out.end(), ys.begin(), ys.end());   // flatten = concat
    }
    return out;
}

// join for vectors:  vector<vector<A>> -> vector<A>
template <typename A>
std::vector<A> join(const std::vector<std::vector<A>>& xss) {
    std::vector<A> out;
    for (const auto& xs : xss)
        out.insert(out.end(), xs.begin(), xs.end());
    return out;
}
```
Usage:
```cpp
std::vector<int> outer{1, 2, 3};

auto rows = bind(outer, [](int n){
    std::vector<int> inner;
    for (int k = 1; k <= n; ++k) inner.push_back(k);
    return inner;                       // length varies with n!
});
// rows == { 1,  1, 2,  1, 2, 3 }
```
For each `n`, the list `[1 .. n]` is returned, whose length depends on `n`.
```
 outer: [1, 2, 3]      f(n) = [1 .. n]

   f(1) = [1]
   f(2) = [1, 2]          ← inner length is decided BY the outer value
   f(3) = [1, 2, 3]

   bind = concat :  [1] ++ [1,2] ++ [1,2,3]  =  [1, 1,2, 1,2,3]
```

In C++ standard library, the list monad's `bind` is, almost verbatim, `transform` (the functor) followed by `join` (flatten), and C++ ships both as range adaptors. C++20's `std::views::join` is the flattening step:

```cpp
#include <ranges>

auto range = [](int lo, int hi){ return std::views::iota(lo, hi + 1); };  // [lo, hi]

// bind via the standard library:  bind = join ∘ transform
auto rows2 = outer
           | std::views::transform([](int n){ return range(1, n); })  // → range of ranges
           | std::views::join;                                        // flatten
// iterating rows2 yields 1, 1, 2, 1, 2, 3
```

#### Example 3 — `Writer` / `Logged`

The `Writer` is used for accumulating a side-output — a log, an audit trail, a running cost — alongside the main value. It threads that accumulation through a chain automatically, with no mutable global and no manual plumbing.

```cpp
#include <vector>
#include <string>

template <typename T>
struct Logged {
    T value;
    std::vector<std::string> log;
};

template <typename T>                          // pure: value, empty log
Logged<T> pure_logged(T v) {
    return { v, {} };
}

// MONAD bind: run f on the value, then APPEND f's log after ours
template <typename A, typename F>
auto bind(const Logged<A>& m, F f) {
    auto next = f(m.value);                    // Logged<B>
    next.log.insert(next.log.begin(),          // put ours first…
                    m.log.begin(), m.log.end());   // …ahead of theirs
    return next;
}
```

Usage — a small calculation that records each step:

```cpp
auto addThree = [](int x){
    return Logged<int>{ x + 3, { "added 3 -> " + std::to_string(x + 3) } };
};
auto timesTwo = [](int x){
    return Logged<int>{ x * 2, { "doubled  -> " + std::to_string(x * 2) } };
};

Logged<int> result = bind( bind( pure_logged(5), addThree ), timesTwo );
// result.value == 16
// result.log   == { "added 3 -> 8", "doubled  -> 16" }
```

```
 pure_logged(5)            { value: 5,  log: [] }
        │  bind addThree
        ▼
                           { value: 8,  log: ["added 3 -> 8"] }
        │  bind timesTwo
        ▼
                           { value: 16, log: ["added 3 -> 8", "doubled -> 16"] }
                                   value flows down;  logs concatenate
```

#### Example 4 (deep dive, skippable) — the `State` monad

This is the most abstract monad and the one that shows the box need not be a container at all. The **State monad** models a computation that threads a piece of mutable-looking state through a sequence — *purely*, with no actual mutation. Its "box" is a function `S → (A, S)`. Given the current state, it returns a result plus the next state.

```cpp
#include <utility>       // std::pair, std::make_pair
#include <functional>    // std::function

template <typename S, typename A>
using State = std::function<std::pair<A, S>(S)>;    // the box: a stateful step

template <typename S, typename A>                    // pure: value, state untouched
State<S, A> pure_state(A a) {
    return [a](S s) { return std::make_pair(a, s); };
}

// MONAD bind: run m to get (a, s'), then run f(a) starting from s'
template <typename S, typename A, typename F>
auto bind_state(State<S, A> m, F f) {
    return [m, f](S s) {
        auto [a, s1] = m(s);      // 1) run the first step
        auto next    = f(a);      // 2) CHOOSE the next step from the value a
        return next(s1);          // 3) run it on the updated state
    };
}
```

The single most important line is `auto next = f(a);`: the next computation is selected using the value `a`, produced by the previous one. That is monadic dependency in its purest form — the structure of step two literally comes from the result of step one.

A concrete use: a fresh-identifier generator (a "gensym") that threads a counter through a chain without any global variable.

```cpp
State<int, int> freshId() {                              // value = current id, state = next
    return [](int counter) { return std::make_pair(counter, counter + 1); };
}

#include <tuple>
auto program =
    bind_state(freshId(), [](int id1){
      return bind_state(freshId(), [id1](int id2){
        return bind_state(freshId(), [id1, id2](int id3){
          return pure_state<int>( std::make_tuple(id1, id2, id3) );
        });
      });
    });

auto [ids, finalCounter] = program(0);
// ids          == (0, 1, 2)
// finalCounter == 3
```

```
 state (counter) flows left→right; each id depends on the prior increment

   0 ──freshId──► id1=0, state=1 ──freshId──► id2=1, state=2 ──freshId──► id3=2, state=3
                                                                              │
                                                              pure: bundle (0,1,2), state=3
```

No mutable counter exists anywhere; the state is *threaded* through the chain by `bind`. This is how purely-functional code expresses "stateful" algorithms, and it is the same `bind`/flatten pattern as the list and `Writer` monads — only the box is now "a function awaiting a state." (The `std::function` representation here has real overhead and is for illustration; production code would template the callable or avoid the abstraction in hot paths — the same honesty caveat that applied to the future examples in earlier lessons.)

#### Example 5 — `std::shared_ptr<T>` (the Maybe monad)

The `bind` runs the next step **only if** the pointer is non-null; that step gets the unwrapped value and may itself return null. `join` pulls out the inner pointer, null-safely.

```cpp
#include <memory>

template <typename T>
std::shared_ptr<T> join(const std::shared_ptr<std::shared_ptr<T>>& pp) {
    if (!pp) return nullptr;
    return *pp;                          // inner ptr (itself maybe null)
}

template <typename T, typename F>       // f : T → shared_ptr<U>
auto bind(const std::shared_ptr<T>& p, F f) {
    if (!p) return decltype(f(*p)){};    // empty in, empty out
    return f(*p);                        // f picks the next box (may be null)
}
// pure_ptr(x) == std::make_shared<T>(x)
```

A dependent chain where any step may bail out:

```cpp
std::shared_ptr<int> halve(int x) {     // fails on odd input
    if (x % 2 != 0) return nullptr;
    return std::make_shared<int>(x / 2);
}

auto chain = bind(std::make_shared<int>(20), [](int x){
    return bind(halve(x), [](int y){    // y exists only if halve(x) succeeded
        return halve(y);                 // this step depends on y
    });
});
// 20 → 10 → 5  ⇒ shared_ptr<int> holding 5
// from 12: 12 → 6 → 3, then halve(3) → null ⇒ whole chain is null
```

#### Example 6 — The Reader monad

A function `X → T` is a box: "a value awaiting an environment." As a monad, `bind` reads a value from the environment, hands it to `f` (which builds a **new** reader based on that value), and runs the new reader in the **same** environment.

```cpp
auto bindReader = [](auto reader, auto f) {
    return [reader, f](auto env) {
        auto a = reader(env);    // read a value from the environment
        return f(a)(env);        // f builds a new reader from a; run it in the same env
    };
};
// pure_reader(v) == [v](auto){ return v; }   (ignores the environment)

struct Config { int base; int factor; };

auto getBase = [](const Config& c){ return c.base; };                            // X → int
auto scaleBy = [](int b){ return [b](const Config& c){ return b * c.factor; }; };// int → (X → int)

auto computed = bindReader(getBase, scaleBy);    // a reader: Config → int
auto v        = computed(Config{5, 3});          // read base=5, then 5 * 3 ⇒ 15
```

The applicative Reader fed the same environment to several *fixed* readers and merged them. The monadic Reader lets the second reader be *chosen by* the value the first one produced — dependence again.

#### Example 7 — `std::future<T>` (the sequential async monad)

Here, the `bind` operation is performed by `.then`: the next async operation is created from the first one's result, which forces the two to run in sequence.

```cpp
#include <future>

template <typename T, typename F>       // f : T → future<U>
auto bind(std::future<T> fut, F f) {
    return std::async(std::launch::deferred,
        [fut = std::move(fut), f = std::move(f)]() mutable {
            T a = fut.get();             // wait for the first
            return f(a).get();           // build the second FROM a, then await it
        });
}
// join(future<future<T>>) == bind(ffut, identity)
```

This monadic `bind` cannot even *create* the second future until the first finishes, so it is strictly sequential.

## Drawbacks
* C++ lacks higher-kinded types, so it is not possible to abstract over a type constructor `F` of kind `* → *`; the boxes do not share a uniform arity. What ships is per-type — `views::transform + views::join` for ranges, `let_value` for senders, `co_await` for coroutines, member `and_then` on the sum-type family — none of them instances of a shared concept.  
* The target data type should be deduced every time.
* The monad pattern is real and useful but remains thoroughly ad hoc and per-type — a recurring shape to recognize, not one named interface every box implements.