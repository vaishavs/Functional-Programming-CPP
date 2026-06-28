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
```
   F<(A → B)>          F<A>                          F<B>
   ┌──────────┐      ┌──────┐                      ┌────────┐
   │   (f)    │  ⊛   │  a   │  ───────────────────►│  f(a)  │
   └──────────┘      └──────┘                      └────────┘
       a boxed         a boxed                       boxed
      function          value                        result
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

#### `pure` (also called `unit` or `lift`) — put a naked value into a box

```
pure : A   yields   F<A>
```
The `pure(x)` builds the *simplest possible* box containing `x`. It adds a value but adds *no extra structure*. is the "minimal, do-nothing" way to enter the box: it adds the box's structure but no extra effect (no second list element, no error, no log).
```
        3                     ┌─────────────┐
   (naked value)   pure ──▶   │  ╔═══════╗  │
                              │  ║   3   ║  │
                              │  ╚═══════╝  │
                              │    F<int>   │
                              └─────────────┘
```
#### `ap` (written `<*>`) — apply a boxed function to a boxed value
```
 ap : F<(B → C)>  and  F<B>   yields   F<C>
```

```
   ┌───────────────┐       ┌───────────────┐          ┌───────────────┐
   │  ╔═════════╗  │       │  ╔═════════╗  │          │  ╔═════════╗  │
   │  ║ (x⇒x+3) ║  │  <*>  │  ║    2    ║  │  ──────▶ │  ║    5    ║  │
   │  ╚═════════╝  │       │  ╚═════════╝  │          │  ╚═════════╝  │
   │  F<int→int>   │       │     F<int>    │          │     F<int>    │
   └───────────────┘       └───────────────┘          └───────────────┘
       boxed fn                boxed value                boxed result
```

## The laws  
An applicative must satisfy four laws, the analogues of the functor laws one level up. 
1. *Identity*: `ap(pure(id), v) = v`. It adds no structure beyond holding a value.
2. *Homomorphism*: `ap(pure(f), pure(x)) = pure(f(x))` — applying a pure function to a pure argument is the same as doing the application on plain values and injecting the result. Pure-on-pure never manufactures any effect or extra structure.
3. *Interchange*: when the value side is pure, the order of evaluation can be swapped. The law says a pure argument carries no effects, so it doesn't matter whether the effectful `u` or the pure `y` is considered to act first.
4. *Composition*: `ap(ap(ap(pure(compose), u), v), w) = ap(u, ap(v, w))`. The law says combining the function-carrying boxes `u` and `v` first, then applying to `w`, agrees with applying in the nested order. Effects compose in one unambiguous order. But just like with functors, these are a contract the compiler never checks, and they presuppose pure functions.
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

What makes this family worth studying is that the same data shape admits a *second*, equally lawful product. Instead of stopping at the first problem, you can choose to gather every problem.

#### The shape family
This is a single range type that supports two different products. Reading "combine" as *align by position*, the product is `views::zip` — walk both ranges in lockstep and pair element by element. Reading it as *multiply*, the product is `views::cartesian_product` — pair every element of the first with every element of the second.

The two readings force two different `pure`s, and the reason is the identity law. For the aligning product, `pure x` must be the infinite `views::repeat(x)`: zipping a range against an endless supply of `x` leaves the original range's length untouched, which is exactly what the law demands. Swap in a one-element range and the zip would truncate everything down to length 1 — the law would visibly break. For the multiplying product, `pure x` must instead be the singleton `views::single(x)`, because pairing every element against a single element again preserves length and leaves the other operand's structure intact. Fix the product and `pure` is no longer a free choice; the law pins it down. The two are different answers to "what does it mean to combine two lists," and the catalogue lists them separately because they genuinely are two different applicatives over the same type.

#### The function family

`Reader` is the family of functions from a fixed environment `R` — a value you don't have yet, but will once you supply an `R`. `pure x` is the constant function that ignores its input and returns `x`; the product hands the *same* `r` to both functions and pairs their results:

```cpp
auto product = [f, g](R r){ return std::pair{ f(r), g(r) }; };
```

In C++ this needs no library type at all — a lambda or `std::function<T(R)>` is the whole implementation. Its practical role is threading a shared environment (configuration, dependencies, a context object) through a computation without passing it explicitly at every step: each piece is written as a function of `R`, the product keeps them all reading from one consistent `r`, and the environment is supplied once at the end.

#### The monoid family — Writer and Const
A monoid is just a type that comes with two things:
1. **A way to combine two values into one** — call it `⊕`. It has to be associative: `(a ⊕ b) ⊕ c` equals `a ⊕ (b ⊕ c)`.
2. **An "identity" value** — a do-nothing element where combining with it changes nothing: `identity ⊕ x == x` and `x ⊕ identity == x`.

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

**Build the operation you'll actually use** — `lift2`, "apply a binary function across two boxes":

```cpp
template <typename Bin, typename A, typename B>
auto lift2(Bin f, const Box<A>& a, const Box<B>& b) {
    // map the curried f over a  →  Box<B→C>, then ap with b  →  Box<C>
    auto curried = map([f](A x){ return [f, x](B y){ return f(x, y); }; }, a);
    return ap(curried, b);
}

auto sum = lift2([](int x, int y){ return x + y; }, Box<int>{2}, Box<int>{3}); // Box<int>{5}
```

`lift2` (and its siblings `lift3`, `lift4`, …) is the ergonomic face of `ap`. In real C++ you'll reach for a `liftN`/`zip_with` far more often than raw `ap` — but every `liftN` *is* `ap` underneath. (Formally, `lift2(f, a, b) == ap(map(f, a), b)`.)

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

This gives you "combine several maybe-present values" semantics — the classic motivation usually shown with `optional`, here delivered by a different box. Note its character: it **short-circuits** to empty on the first missing input. (Hold that thought; §5 builds a box that deliberately does *not* short-circuit.)

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

Recall from the functor guide that a function `R(X)` is itself a box ("an `R` for every environment `X`"). As an *applicative*, `ap` feeds the **same environment** to both the boxed-function and the boxed-value, then applies one to the other:

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

This is how you assemble many computations that each read from a shared configuration/context and combine their results — without threading the context through by hand. The "effect" the Reader applicative adds is *dependence on an ambient environment*.

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

The crucial observation: `fa` and `fb` **do not depend on each other**, so they overlap in time. A *monadic* version (`fa.then([](A a){ return makeFutureB(a); })`) could only create the second future *after* the first finished — forcing them to run **in sequence**. That contrast is the whole point of the next section.

#### 7. Validation
This is the example that makes people care about applicatives. A monadic error type (like the std maybe/either boxes) **short-circuits**: the moment one step fails, the rest are skipped, so you learn about **one** error at a time. A form with five bad fields makes the user fix-and-resubmit five times.

An applicative-only `Validation` box instead **collects every error**, because `ap` always inspects *both* sides before deciding:

```cpp
#include <variant>
#include <vector>
#include <string>

template <typename T>
struct Validation {
    std::variant<std::vector<std::string>, T> v;   // index 0 = errors, 1 = value
    bool ok() const { return v.index() == 1; }
    const T& get() const { return std::get<1>(v); }
    const std::vector<std::string>& errs() const { return std::get<0>(v); }
};

template <typename T> Validation<T> valid(T x) {
    return { std::variant<std::vector<std::string>, T>(std::in_place_index<1>, std::move(x)) };
}

// Combine two validations with a binary function — accumulating errors from BOTH.
template <typename F, typename A, typename B>
auto validate2(F f, const Validation<A>& a, const Validation<B>& b)
    -> Validation<std::invoke_result_t<F, A, B>>
{
    using R = std::invoke_result_t<F, A, B>;
    if (a.ok() && b.ok())
        return valid<R>(f(a.get(), b.get()));

    std::vector<std::string> all;                  // gather everything
    if (!a.ok()) all.insert(all.end(), a.errs().begin(), a.errs().end());
    if (!b.ok()) all.insert(all.end(), b.errs().begin(), b.errs().end());
    return Validation<R>{ std::move(all) };
}
```

```cpp
struct User { std::string name; int age; };
auto err = [](std::string m){ return std::vector<std::string>{ std::move(m) }; };

Validation<std::string> checkName(std::string n) {
    if (n.empty()) return Validation<std::string>{ err("name is empty") };
    return valid(std::move(n));
}
Validation<int> checkAge(int a) {
    if (a < 0) return Validation<int>{ err("age is negative") };
    return valid(a);
}

auto makeUser = [](std::string n, int a){ return User{std::move(n), a}; };

auto good = validate2(makeUser, checkName("Ada"), checkAge(36));  // ok: User{"Ada", 36}
auto bad  = validate2(makeUser, checkName(""),    checkAge(-1));  // invalid
// bad.errs() == { "name is empty", "age is negative" }   ← BOTH reported at once
```

> This is precisely what a short-circuiting monad (e.g. `std::expected` chained with `and_then`) **cannot** do: it would stop at `"name is empty"` and never check the age. `Validation` reports both because `ap` is non-sequential — it never lets one box's failure prevent it from examining the other. The price: `Validation` is **applicative but not a monad** (a lawful `bind` would reintroduce short-circuiting). The "weakness" *is* the feature.

## Drawbacks
* C++ has no higher-kinded types, so — exactly as with functors — there is no standard `Applicative` interface and no way to write one constraint over "all boxes." 
* `pure` is return-type-polymorphic, which C++ can't express directly. In C++, the target box must be named: `pure_box`, `pure_vec`, `make_shared`, `valid`, a ready `future`, etc. You generally write a per-box `pure` (and often pass the box type as a template argument when context can't supply it).
* Some `pure`s want laziness (ZipList's infinite `repeat`), which a finite eager container can't provide — hence `std::views::repeat`.
