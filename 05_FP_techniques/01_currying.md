Currying is an essential part of all functional programming languages. Currying splits a function of n arguments into a composition of n functions of one argument using a partial function evaluation, ie., fixing some function arguments (see lambdas and [std::bind](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/04_bind.md). 

Currying transforms f(a, b, c) into f(a)(b)(c) — each call returns a new function until all arguments have been supplied.

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

With `std::bind` and lambdas, the same effect is achieved as above:
```
int add(int a, int b) { return a + b; }

// std::bind — verbose, harder to read
auto add5 = std::bind(add, std::placeholders::_1, 5);

// Lambda — clean, obvious
auto add10 = [](int a, int c) { return add(a, 10); };

add5(3);              // 8
add10(5);            // 15

// Prefer lambdas in modern C++. 
// They're more readable, easier to inline, and compile faster.
```

One of the most important mental shifts when learning currying is to stop thinking of functions as calculators and start thinking of them as factories.

Instead of thinking:
`Function + Arguments = Result`

think:
`Function + Some Arguments = New Function`

This is a fundamentally different computational model. Consider the above example. It does not just add two numbers. It creates an addition machine. The function acts like a factory that manufactures other functions. Every curried function is effectively a small object with stored configuration.

### Generic Curry Helper (Since C++17)
With the advanced metaprogramming capabilities of C++17, specifically variadic templates and `if constexpr`, a generic curry helper can be created. This wrapper patiently collects arguments one by one until the underlying function's signature is fully satisfied, at which point it executes.
```
#include <functional>

// A generic curry helper for C++17
template<typename F>
auto curry(F f) {
    return [f](auto... args) {
        if constexpr (std::is_invocable_v<F, decltype(args)...>) {
            return f(args...); // All arguments satisfied, execute
        } else {
            return curry([f, args...](auto... rest) {
                return f(args..., rest...); // Wait for more arguments
            });
        }
    };
}

auto multiply = [](int a, int b, int c) { return a * b * c; };
auto curried  = curry(multiply);

auto step1 = curried(2);       // Fixes a = 2
auto step2 = step1(3);         // Fixes b = 3
auto result = step2(4);        // All args met, executes: 2 * 3 * 4 = 24
```

### Template-Based Partial Application (Since C++17)
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
