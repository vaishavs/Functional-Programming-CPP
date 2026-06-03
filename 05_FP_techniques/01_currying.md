Currying is a transformation that splits a function of n arguments into a composition of n functions of one argument using a partial function evaluation, ie., fixing some function arguments (see lambdas and [std::bind](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/04_bind.md). 

Currying transforms f(a, b, c) into f(a)(b)(c) — each call returns a new function until all arguments have been supplied.

### Manual currying
The most direct way is to hand-write nested lambdas, capturing each argument on the go:

```
// Manual currying
auto add = [](auto a) {
    return [a](auto b) {
        return a + b;
    };
};

auto add5 = add(5);   // returns a lambda
add5(3);              // 8
add(10)(20);          // 30
```

With `std::bind` and lambdas, the same effect can be achieved as above:

```
int add(int a, int b) { return a + b; }

// std::bind — verbose, harder to read
auto add5 = std::bind(add, std::placeholders::_1, 5);

// Lambda — clean, obvious
auto add10 = [](int a, int c) { return add(a, 10); };

add5(3);              // 8
add10(5);            // 15

int x = add(1)(2)(3);   // 6
auto add1 = add(1);     // partially applied: a closure waiting for b
auto add1_2 = add1(2);   // waiting for c
int y = add1_2(3);       // 6

// Prefer lambdas in modern C++. 
// They're more readable, easier to inline, and compile faster.
```

It is worth noting that `add(1)`, `add(1)(2)`, and `add` all have different, unnameable types. Each is a distinct compiler-generated closure type. They can be stored in auto, or erased into `std::function` if a concrete type is needed (at the cost of an allocation and indirect call).

The `std::bind` is flexible (it can reorder and skip arguments via placeholders) but verbose and historically a source of subtle copy/reference bugs. C++20 added the cleaner `std::bind_front`, which fixes leading arguments, and C++23 added `std::bind_back` for trailing ones. Hence, for most real code, `std::bind_front`/`std::bind_back` or a small lambda is preferable; full currying is rarely the most readable choice.

```
auto f = std::bind_front(add3, 1, 2);   // C++20: fixes a=1, b=1; f(3) -> 6
auto g = std::bind_back(add3, 3);       // C++23: fixes the last arg

```

One of the most important mental shifts when learning currying is to stop thinking of functions as calculators and start thinking of them as factories.

Instead of thinking:

`Function + Arguments = Result`

think:

`Function + Some Arguments = New Function`

This is a fundamentally different computational model. Consider the above example. It does not just add two numbers. It creates an addition machine. The function acts like a factory that manufactures other functions. Every curried function is effectively a small object with stored configuration, thus avoiding side effects and creating small, reusable functions.

### Generic Curry Helper (Since C++17)
With the advanced metaprogramming capabilities of C++17, specifically variadic templates and `if constexpr`, a generic curry helper can be created. This wrapper patiently collects arguments one by one until the underlying function's signature is fully satisfied, at which point it executes.
```
#include <functional>

#include <type_traits>

template <typename F, typename... Args>
auto curry(F f, Args... args) {
    if constexpr (std::is_invocable_v<F, Args...>) {
        return f(args...);                       // enough args: call it
    } else {
        return [f, args...](auto x) {            // not yet: capture and wait
            return curry(f, args..., x);
        };
    }
}

int add3(int a, int b, int c) { return a + b + c; }

// usage:
auto r  = curry(add3)(1)(2)(3);   // 6
auto p  = curry(add3)(1)(2);      // partial: closure waiting for c
auto r2 = p(3);                   // 6
```

#### The mechanism
The function `curry(add3)` checks `is_invocable_v<F> (callable with zero args?)`. It isn't, so it returns a lambda. Calling that lambda with 1 recurses into `curry(add3, 1)`, which checks `is_invocable_v<F, int>` — still false for a 3-arg function — and so on, until three arguments have accumulated and the call finally fires.
This is clever and works for ordinary functions and most function objects, but you should understand exactly where it breaks:
* Default arguments fool the arity check. For `int g(int a, int b = 0)`, the expression `is_invocable_v<G, int>` is true, because `g(x)` is a legal call. So `curry(g)(5)` calls `g(5)` immediately and never accepts a second argument. The "try to call" strategy can't distinguish "you gave me enough" from "the rest have defaults."
* Overloaded functions can't be deduced. Passing a bare overload-set name leaves `F` unable to bind to a single type. This must be disambiguated with a cast (`static_cast<int(*)(int,int)>(&f)`) or the call should be wrapped in a lambda.
* Variadic and heavily templated callables confuse `is_invocable`, since "callable with these args" may always be true.
* Move-only argument types don't survive, because this version copies (`Args... args` by value, then re-copies on every recursive step).


### Template-Based (arity-explicit) Partial Application (Since C++17)
For developers seeking absolute maximum performance without relying solely on lambdas, C++17 allows for template-based partial application. By leveraging `std::tuple` and `std::apply`, bound arguments can be stored at compile time and unpacked alongside the remaining arguments when the function is finally invoked.

```
#include <tuple>
#include <functional>
#include <cmath>

template<typename F, typename... Bound>
class partial_t {
    F func;
    std::tuple<Bound...> bound_args;
public:
    partial_t(F f, Bound... args)
        : func(f), bound_args(args...) {}

    template<typename... Rest>
    auto operator()(Rest&&... rest) const {
        return std::apply([&](auto&... b) {
            return func(b..., std::forward<Rest>(rest)...);
        }, bound_args);
    }
};

// Helper factory to deduce template arguments
template<typename F, typename... Args>
auto partial(F f, Args... args) {
    return partial_t<F, Args...>(f, args...);
}

// Usage Example
auto power = [](double base, int exp) { return std::pow(base, exp); };
auto square = partial(power, 2.0);  // base is fixed to 2.0, exp is left free
square(10);  // Evaluates to 2^10 = 1024
```

Here, the number of steps is a compile-time constant rather than something inferred from invocability, so it's unambiguous and predictable.

#### A move-correct, perfect-forwarding version (C++20)
For production use with heavy or move-only callables and arguments, a forward is preferred over a copy, and the call is routed through `std::invoke` so the same code handles plain functions, function objects, and pointers to members. C++20's init-capture pack expansion (`[...args = ...]`) makes this expressible:

```
#include <functional>
#include <type_traits>
#include <utility>

template <typename F, typename... Args>
auto curry(F&& f, Args&&... args) {
    if constexpr (std::is_invocable_v<F, Args...>) {
        return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    } else {
        return [f = std::forward<F>(f),
                ...args = std::forward<Args>(args)]
               (auto&& x) mutable {
            return curry(std::move(f),
                         std::move(args)...,
                         std::forward<decltype(x)>(x));
        };
    }
}
```

The mutable is required because each step moves out of its own captures into the next call. The tradeoff is that such a closure is effectively single-use: once you've called it and moved its state onward, calling it again sees moved-from members. For repeated partial application you'd capture by value instead and accept the copies. In C++17, where you can't expand a pack in an init-capture, you achieve the same by storing the accumulated arguments in a std::tuple member (as in the arity-explicit version above) rather than as a capture pack.


## Real-World Use Cases
### Algorithms with Predicates
Partial application helps to adapt complex conditions into simple predicates.
```
#include <algorithm>
#include <vector>

std::vector<int> nums = {1, 3, 7, 2, 9, 4};
int threshold = 5;

// Partially apply the threshold logic into a unary predicate
auto above = [threshold](int x) { return x > threshold; };

// 7, 9 are above 5. count is 2.
auto count = std::count_if(nums.begin(), nums.end(), above);
```
### Callbacks and Event Handlers
When interacting with logging systems or UI events, a generalized function is often used that to specialize for specific contexts to avoid repetitive boilerplate.
```
#include <iostream>
#include <string>

void log(const std::string& prefix, const std::string& msg) {
    std::cout << "[" << prefix << "] " << msg << "\n";
}

// Creating specialized loggers via partial application
auto error = [](const std::string& m) { log("ERROR", m); };
auto info  = [](const std::string& m) { log("INFO",  m); };

error("Connection timed out");   // Outputs: [ERROR] Connection timed out
info("System booted");           // Outputs: [INFO]  System booted
```

### Data Pipelines and Composition
By partially applying transformation rules, highly readable data pipelines can be composed using modern C++ fold expressions.
```
#include <vector>
#include <algorithm>

// A generic pipeline executor
auto pipeline = [](auto data, auto... transforms) {
    ((data = transforms(data)), ...);
    return data;
};

std::vector<int> data = {5, 3, 8, 1, 9, 2};

// Partially applied transformations
auto sorted = [](auto v) { std::sort(v.begin(), v.end()); return v; };
auto take3  = [](auto v) { v.resize(3); return v; };

auto result = pipeline(data, sorted, take3);
// Result is now: {1, 2, 3}
```

Spurce:
https://www.youtube.com/watch?v=zVLLdGlbCSw
