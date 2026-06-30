## What applicative adds to functor  
An applicative combines several boxes whose structure is fixed in advance and when the computations don't depend on each other's results.

A functor provides one operation, `transform`, which lifts a *unary* function over a *single* box. An applicative adds two operations and, with them, the ability to lift an *n-ary* function over *n* boxes.  
The two additions are pure (inject a plain value into the minimal context, `A → F<A>`) and `ap` (apply a *boxed* function to a *boxed* value).

The reason this matters is the motivating problem applicatives solve: combining *several independent boxes* with a multi-argument function. Suppose that given a two-argument function `f : A → B → C` and two boxes `F<A>` and `F<B>`.
```
g  : A → B → C            (curried: takes an A, returns a function B → C)
fa : F<A>
fb : F<B>
```
Because `g` is curried, applying a `transform` to an `A` returns a function `B → C`. So `transform(g)` over `F<A>` does *not* give `F<C>` — the partial application leaves behind `F<B → C>` **box full of functions**:
```
 transform(g) over F<A>  ─────►  F<(B → C)>      ← a box of half-finished functions
```
A functor knows how to put a *plain* function into a box, not how to apply a *boxed* function. It has no operation that can apply a boxed function to the boxed `F<B>`. 
```
   transform (f : A→B→C) over F<A>  ⇒  F< B → C >    a BOXED function
                                            │
                                            ?  a functor cannot apply a
                                            ?  boxed function to F<B>
                                            ▼
                                      ── stuck ──     (only ap finishes it)
```
That missing operation is exactly **`ap`**:
```
 ap : F<(B → C)>  and  F<B>   yields   F<C>      ← exactly what closes the gap
```
For example:
```
   ┌───┬───┬───┐     ┌────┬────┬────┐
   │ 1 │ 2 │ 3 │     │ 10 │ 20 │ 30 │      function: (+)
   └───┴───┴───┘     └────┴────┴────┘
        │                  │
        └────── combine ───┘
                  │
                  ▼
          ┌────┬────┬────┐
          │ 11 │ 22 │ 33 │
          └────┴────┴────┘
   two boxes joined position-by-position into one box
```
The signature of `ap` against `transform` is:
```
   Functor transform :   (A → B)   →   F<A>   →   F<B>
                         └ plain ┘      box        box
                         the function is OUTSIDE the box

   Applicative ap     :  F<A → B>  →   F<A>   →   F<B>
                         └─ box ─┘      box        box
                         the function is INSIDE a box too
```
In fact, an applicative can combine **any number** of independent boxes.
## Components of an applicative
An applicative provides:

#### `pure` (also called `unit` or `lift`) — wraps a value
This is the simplest operation. It takes one ordinary value and puts it in the box. It is the "minimal, do-nothing" way to enter the box: it adds the box's structure but no extra effect (no second list element, no error, no log).
```
   plain value          boxed value
   ┌─────┐              ┌─────────┐
   │  5  │  ── pure ──► │ [  5  ] │
   └─────┘              └─────────┘
```
That's the whole operation: bare value in, box out.
In C++, this is what it looks like:
```cpp
// Lift a value into the context.
template<
    template<typename> class Context,
    typename T
>
auto pure(T value);
```
Meaning:
```
T → Context<T>
```

#### `ap` (written `<*>`) — combine several boxes with one function
This is the operation that defines an applicative. A functor (the level below) can only touch one box. An applicative combines several boxes at once using a function. The cleanest way to see it: multiple boxes go in, one function joins their contents, one box comes out. The function is supplied plainly and the two boxes after it; the contents are paired up and combined. Combining **more than one box** is the entire point of this operation — and the single thing that separates an applicative from a functor.

```
 ap  :   F<A → B>   ×   F<A>   ──►   F<B>


   F<A → B> ──┐
              ├── ap ──►  F<B>
   F<A>     ──┘
              │
        reach into both boxes, apply the function to the value,
        put the result back in a box of the same kind F
```

```
   ┌───────────────┐       ┌───────────────┐          ┌───────────────┐
   │  ╔═════════╗  │       │  ╔═════════╗  │          │  ╔═════════╗  │
   │  ║ (x⇒x+3) ║  │  <*>  │  ║    2    ║  │  ──────▶ │  ║    5    ║  │
   │  ╚═════════╝  │       │  ╚═════════╝  │          │  ╚═════════╝  │
   │  F<int→int>   │       │     F<int>    │          │     F<int>    │
   └───────────────┘       └───────────────┘          └───────────────┘
   boxed fn F<A → B>               boxed value F<A>                boxed result
```
In C++, this is what it looks like:
```cpp
// Apply a wrapped function to a wrapped value.
template<
    template<typename> class Context,
    typename Function,
    typename Input
>
auto apply(
    Context<Function> wrappedFunction,
    Context<Input> wrappedValue
);
```
Meaning:
```
apply:   Context<Input -> Output>     -> Context<Input> -> Context<Output>
        └─ Context on the function ─┘
```
The *function* itself is wrapped: `Context<Input -> Output>`. It is boxed (inside). This is the one extra wrapper that defines an applicative.
## The laws  
An applicative must satisfy four laws, the analogues of the functor laws one level up. 
1. *Identity*: `ap(pure(id), v) = v`. It adds no structure beyond holding a value.
2. *Homomorphism*: `ap(pure(f), pure(x)) = pure(f(x))` — applying a pure function to a pure argument is the same as doing the application on plain values and injecting the result. Pure-on-pure never manufactures any effect or extra structure.
3. *Interchange*: when the value side is pure, the order of evaluation can be swapped. The law says a pure argument carries no effects, so it doesn't matter whether the effectful `u` or the pure `y` is considered to act first.
4. *Composition*: `ap(ap(ap(pure(compose), u), v), w) = ap(u, ap(v, w))`. The law says combining the function-carrying boxes `u` and `v` first, then applying to `w`, agrees with applying in the nested order. Effects compose in one unambiguous order. But just like with functors, these are a contract the compiler never checks, and they presuppose pure functions.

Consider a generic box type:
```cpp
template<class T>
struct Box {
    T value;
};

template<class T>
Box<T> pure(T x) {
    return {x};
}

template<class F, class T>
auto ap(Box<F> f, Box<T> x) {
    return Box{f.value(x.value)};
}
```
And the helpers:
```cpp
auto id = [](auto x) { return x; };

auto compose = [](auto f) {
    return [=](auto g) {
        return [=](auto x) {
            return f(g(x));
        };
    };
};
```

Then the four Applicative laws in simple terms are:
```cpp
// Identity
ap(pure(id), x) == x;

// Homomorphism
ap(pure(f), pure(x)) == pure(f(x));

// Interchange
ap(u, pure(x)) == ap(pure([=](auto f){ return f(x); }), u);

// Composition
ap(ap(ap(pure(compose), u), v), w)
==
ap(u, ap(v, w));
```

## Types of applicatives
To call a box an "applicative", two tools are needed:
* Wrap — take a plain value and put it in the box.
* Combine — take two boxes and merge them into one box holding both values (a pair). Then map the function over that pair.

The second tool is the heart of it. In C++, it almost always looks like one line:
```cpp
combine(boxA, boxB) | map(f)
```
and every type is just a different `combine` and a different `wrap`.

#### The Identity Box
The Identity box is the simplest possible context: it wraps exactly one value and adds nothing around it. `pure x` is just `Box{x}`, and the product does the obvious thing — given two boxes it hands back a box of the pair, with no decisions to make along the way. Because it carries no extra structure, every other row in the catalogue can be read as "Identity, plus something": plus a failure case, plus multiplicity, plus a dependency on an input, plus an accumulated log, plus asynchrony. That makes it the natural reference point — whatever an applicative *adds* becomes visible precisely as the difference from this do-nothing baseline.

There's no standard-library type for it, and that's expected: a wrapper that does nothing offers nothing over the bare value. `std::optional<T>` is the closest relative in spirit — a single-value container — but it's a genuinely different applicative, because it carries the possibility of holding nothing, and that possibility changes what the product must do.

#### The failure / error family
These boxes hold either a value or an indication that something went wrong, and the family is defined by what the product does when something *has* gone wrong. `std::optional<T>` (the "Maybe" shape) collapses to `nullopt` the moment either operand is empty: combining two values requires both to be present, so a single absence is enough to sink the result. `std::expected<T,E>` behaves the same way but carries a payload describing the failure — when an operand holds an error, that error becomes the result, and the first one encountered is the one that propagates.

#### The shape family
This is a single range type that supports two different products. Reading "combine" as *align by position*, the product is `views::zip` — walk both ranges in lockstep and pair element by element. Reading it as *multiply*, the product is `views::cartesian_product` — pair every element of the first with every element of the second.

The two readings force two different `pure`s, and the reason is the identity law. For the aligning product, `pure x` must be the infinite `views::repeat(x)`: zipping a range against an endless supply of `x` leaves the original range's length untouched, which is exactly what the law demands. Swap in a one-element range and the zip would truncate everything down to length 1 — the law would visibly break. For the multiplying product, `pure x` must instead be the singleton `views::single(x)`, because pairing every element against a single element again preserves length and leaves the other operand's structure intact. Fix the product and `pure` is no longer a free choice; the law pins it down. The two are different answers to "what does it mean to combine two lists," and the catalogue lists them separately because they genuinely are two different applicatives over the same type.

#### The function family

`Reader` is the family of functions from a fixed environment `R` — a value that is not available yet, but will once an `R` is supplied. The `pure x` is the constant function that ignores its input and returns `x`; the product hands the *same* `r` to both functions and pairs their results:

```cpp
auto product = [f, g](R r){ return std::pair{ f(r), g(r) }; };
```

In C++ this needs no library type at all — a lambda or `std::function<T(R)>` is the whole implementation. Its practical role is threading a shared environment (configuration, dependencies, a context object) through a computation without passing it explicitly at every step: each piece is written as a function of `R`, the product keeps them all reading from one consistent `r`, and the environment is supplied once at the end.

#### The monoid family — Writer and Const
A monoid is just a type that comes with two things:
- **A way to combine two values into one** — call it `⊕`. It has to be associative: `(a ⊕ b) ⊕ c` equals `a ⊕ (b ⊕ c)`.
- **An "identity" value** — a do-nothing element where combining with it changes nothing: `identity ⊕ x == x` and `x ⊕ identity == x`.

Some common monoids:
| Type | combine `⊕` | identity |
|---|---|---|
| `std::string` | concatenation | `""` (empty string) |
| `int` under `+` | addition | `0` |
| `std::vector<T>` | append one to the other | `{}` (empty vector) |

In one sentence, a monoid is just "a combine plus a neutral starting value."

Now, `Writer` carries a monoid *along with* a real value; `Const` carries a monoid *instead of* a value.

**Writer — value and a log**

A `Writer` is a pair: `{ the real value, a monoid }`. The classic monoid here is a log (a string, or a vector of messages). The idea is tto compute a normal value but *also* accumulate some running record beside it. The `pure x` wraps a plain value and attaches the monoid's identity as the starting log. The product combines two Writers by pairing the two real values *and* combining the two logs with the monoid's `⊕`.

**Const — only the monoid, the value is ignored**
`Const<M>` is the strange-but-useful sibling. It carries only an `M` (a monoid) — there is a value type in the signature, but it is never actually stored or used. The `pure x` throws the `x` away and returns the monoid's identity. The product ignores any values (there are none to speak of) and just combines the two `M`s with `⊕`.

A `Const` computation produces *no value at all*. Running it does exactly one thing — fold a monoid together across the whole structure. The applicative machinery is still there (it has a `pure` and a product), but since the value side is empty, the *only* observable effect is the monoid accumulating.

Suppose there is a big nested structure and it needs to be summarized *without changing the structure at all*. For the monoid `M`:
- `M = int under +` → the result is a count or a sum
- `M = vector<T>` → the result is collection of everything into one list
- `M = "any true?"` (boolean OR, identity `false`) → the result is a search / "does any match"

Then, running the `Const` applicative over the structure walks the whole thing and folds the chosen monoid across it, handing back just that summary. This is precisely why `Const` is the engine inside lens libraries: a lens that can *modify* a structure can be made to *read* or *summarize* it instead, simply by running it in `Const` — the value-transforming part goes silent, and only the monoid-collecting part runs.

In summary:
- **Writer** = `{ real value, monoid }` → keeps the value *and* accumulates the monoid beside it.
- **Const** = `{ monoid }` only → discards the value entirely and accumulates *nothing but* the monoid.

`Const` is just `Writer` with the value half deleted — which turns "compute a result while logging" into "compute no result, only summarize."

#### The async family

This family consists of senders (`just` / `when_all` / `then`, standardized for C++26 and available through `stdexec`) and the older `std::future`. A box here is a value that will arrive later. `pure x` is `just(x)` for senders, a ready future for `std::future` — is the computation that delivers `x` immediately, with no waiting. The product is the concurrent combine: `when_all` starts both computations, waits for both to finish, and produces their results together. So in this family, running things concurrently *is* the applicative product — independent work fired off together and joined once all of it completes.

Feeding one async result into the *next* — where the second computation depends on the first — is a separate operation, outside the applicative interface this catalogue is concerned with. The applicative product is specifically the independent, run-them-together case.

| Family | C++ equivalent | `pure x` | product (combine two) |
|---|---|---|---|
| **Trivial** | hand-rolled `Box<T>` | `Box{x}` | pair the two values |
| **Failure / error** | `std::optional<T>` · `std::expected<T,E>` · custom Validation | `optional{x}` · `expected{x}` · `valid{x}` | both present → pair (else `nullopt`) · first error wins · collect **all** errors |
| **Shape** | `std::ranges` + `views::zip` · + `views::cartesian_product` | `views::repeat(x)` · `views::single(x)` | `zip` — align by position · `cartesian_product` — multiply |
| **Function** | `std::function<T(R)>` / lambda | `[x](R){return x;}` | feed both the same `r`, pair results |
| **Monoid** | `std::pair<T,W>` (Writer) · `struct Const<M>{M m;}` | `{x, W{}}` · `{M{}}` (identity elem) | pair values, glue logs · combine the two `M`s, ignore values |
| **Async** | `std::execution` sender · `std::future<T>` | `just(x)` · `make_ready_future(x)` | `when_all` (both) |

Within a cell, the `·` separates the members of that family, in the same order across all three columns — so in **Failure / error**, `optional` ↔ `optional{x}` ↔ pair-else-nullopt, `expected` ↔ `expected{x}` ↔ first-error-wins, Validation ↔ `valid{x}` ↔ collect-all. The **Shape** and **Monoid** rows read the same way (two members each), and **Trivial**/**Function** have just one.

## Examples and use cases
#### 1. The Identity applicative

```cpp
#include <type_traits>
#include <utility>

template <typename T>
struct Box { T value; };

// pure: lift a naked value into the box
template <typename T>
Box<std::decay_t<T>> pure_box(T&& x) { return { std::forward<T>(x) }; }

// ap: apply a boxed function to a boxed value
template <typename F, typename T>
auto ap(const Box<F>& bf, const Box<T>& bx)
    -> Box<std::invoke_result_t<const F&, const T&>>
{
    return { bf.value(bx.value) };
}
```

**Recover `map` for free** (this is the proof that every applicative is a functor):

```cpp
template <typename F, typename T>
auto map(F f, const Box<T>& bx) { return ap(pure_box(f), bx); }   // map = ap ∘ pure

auto squared = map([](int x){ return x * x; }, Box<int>{3});       // Box<int>{9}
```

**Build the operation** — `lift2`, "apply a binary function across two boxes":

```cpp
template <typename Bin, typename A, typename B>
auto lift2(Bin f, const Box<A>& a, const Box<B>& b) {
    // map the curried f over a  →  Box<B→C>, then ap with b  →  Box<C>
    auto curried = map([f](A x){ return [f, x](B y){ return f(x, y); }; }, a);
    return ap(curried, b);
}

auto sum = lift2([](int x, int y){ return x + y; }, Box<int>{2}, Box<int>{3}); // Box<int>{5}
```

`lift2` (and its siblings `lift3`, `lift4`, …) is the ergonomic face of `ap`. Every `liftN` *is* `ap` underneath. (Formally, `lift2(f, a, b) == ap(map(f, a), b)`.)

#### 2. Cartesian product
Applying a list of functions to a list of values applies every function to every value:

```
   funcs:  [ (+1) , (*10) ]          values: [ 1 , 2 , 3 ]

   (+1)  over 1,2,3   →   2, 3, 4
   (*10) over 1,2,3   →   10, 20, 30

   result:  [ 2, 3, 4, 10, 20, 30 ]        (every fn × every value = 2 × 3 = 6 results)
```

```cpp
#include <vector>
#include <functional>

template <typename F, typename T>
auto ap(const std::vector<F>& fs, const std::vector<T>& xs)
    -> std::vector<std::invoke_result_t<const F&, const T&>>
{
    std::vector<std::invoke_result_t<const F&, const T&>> out;
    out.reserve(fs.size() * xs.size());
    for (const auto& f : fs)
        for (const auto& x : xs)
            out.push_back(f(x));          // cartesian product
    return out;
}

// pure for vector = a one-element vector:  pure_vec(x) == {x}

std::vector<std::function<int(int)>> fs = {            // different lambdas need a
    [](int x){ return x + 1; },                        // common callable type, e.g.
    [](int x){ return x * 10; }                        // std::function
};
std::vector<int> xs = {1, 2, 3};
auto r = ap(fs, xs);                                   // {2, 3, 4, 10, 20, 30}
```
#### 3. Maybe applicative

A smart pointer is a box that might be empty. Its `ap` produces a value only when **both** the function-box and the value-box are present; if either is null, the result is null and the function never runs.

```cpp
#include <memory>

template <typename F, typename T>
auto ap(const std::shared_ptr<F>& pf, const std::shared_ptr<T>& px)
    -> std::shared_ptr<std::invoke_result_t<const F&, const T&>>
{
    if (!pf || !px) return nullptr;                 // both required
    return std::make_shared<std::invoke_result_t<const F&, const T&>>((*pf)(*px));
}
// pure_ptr(x) == std::make_shared<T>(x)
```

This gives the "combine several maybe-present values" semantics — the classic motivation usually shown with `optional`, here delivered by a different box. Note its character: it **short-circuits** to empty on the first missing input.

#### 4. Writer

`std::pair` is a functor unconditionally (map the second element, carry the first along). But to be an *applicative*, `ap` must combine **two** first-elements into one — and `pure` must conjure a first-element out of nothing. That requires `A` to be a **Monoid**: an associative `combine` (`+`) plus an identity element (`0`, `""`, empty list).

```cpp
#include <utility>
#include <string>

// Requires A to support combination (here: operator+) — i.e. A is a Monoid.
template <typename A, typename F, typename T>
auto ap(const std::pair<A, F>& pf, const std::pair<A, T>& px)
    -> std::pair<A, std::invoke_result_t<const F&, const T&>>
{
    return { pf.first + px.first,                 // combine the two logs/tags
             pf.second(px.second) };              // apply fn to the two values
}
// pure for Writer needs the Monoid's IDENTITY:  pure_writer(x) == { A{}, x }
//   ("" for string concat, 0 for addition, {} for a list)
```

```cpp
std::pair<std::string, int> a{"got x; ", 2};
std::pair<std::string, int> b{"got y; ", 40};
auto plus = [](int x){ return [x](int y){ return x + y; }; };
auto r = ap(std::pair<std::string, decltype(plus(0))>{"plus; ", plus(2)}, b);
// second == 42, first == "plus; got y; "  — logs are concatenated by ap
```

Lesson: **applicative-ness can carry an algebraic prerequisite.** `pair` shows the boundary — extra power demands extra structure (a Monoid) on the part of the box that has to be *merged*.

#### 5. Reader

A function `R(X)` is itself a box ("an `R` for every environment `X`"). As an *applicative*, `ap` feeds the same environment to both the boxed-function and the boxed-value, then applies one to the other:

```
   (f <*> g)(x)  =  f(x)( g(x) )        both f and g receive the SAME x
```

The readable, `lift2`-style form (both readers see the same `env`, results combined):

```cpp
#include <string>

auto combineReader = [](auto f, auto ra, auto rb) {
    return [f, ra, rb](auto env){ return f(ra(env), rb(env)); };  // share env
};
// pure for Reader = const:  pure_reader(v) == [v](auto){ return v; }  (ignore env)

struct Config { std::string name; int age; };

auto greet = combineReader(
    [](std::string n, int a){ return n + " is " + std::to_string(a); },
    [](const Config& c){ return c.name; },        // reader 1
    [](const Config& c){ return c.age;  });        // reader 2

auto line = greet(Config{"Ada", 36});              // "Ada is 36"
```

This is how many computations are assembled that each read from a shared configuration/context and combine their results — without threading the context through by hand. The "effect" the Reader applicative adds is *dependence on an ambient environment*.

#### 6. Async

Two futures launched independently are already running, possibly **in parallel**. The applicative just waits for both and combines them:

```cpp
#include <future>

template <typename A, typename B, typename F>
auto combineFuture(std::future<A> fa, std::future<B> fb, F f)
    -> std::future<std::invoke_result_t<F, A, B>>
{
    return std::async(std::launch::deferred,
        [fa = std::move(fa), fb = std::move(fb), f = std::move(f)]() mutable {
            A a = fa.get();
            B b = fb.get();                          // both already in flight
            return f(a, b);
        });
}

auto fa = std::async(std::launch::async, []{ return slowFetchA(); });   // runs now
auto fb = std::async(std::launch::async, []{ return slowFetchB(); });   // runs now (parallel)
auto combined = combineFuture(std::move(fa), std::move(fb),
                              [](auto a, auto b){ return merge(a, b); });
```

The crucial observation: `fa` and `fb` **do not depend on each other**, so they overlap in time.

## Drawbacks
* C++ has no higher-kinded types, so — exactly as with functors — there is no standard `Applicative` interface and no way to write one constraint over "all boxes." 
* `pure` is return-type-polymorphic, which C++ can't express directly. In C++, the target box must be named: `pure_box`, `pure_vec`, `make_shared`, `valid`, a ready `future`, etc. Generally, a per-box `pure` is written (and often the box type is passed as a template argument when context can't supply it).
* Some `pure`s want laziness (ZipList's infinite `repeat`), which a finite eager container can't provide — hence `std::views::repeat`.
