# Bonus: `std::expected` and `std::optional`
For much of its history, C++ offered no clean way to represent non-existent values. A function that searched for something and might not find it had only awkward options available: return a null pointer and hope the caller remembered to check it, return a sentinel value like -1 and hope that value never collided with a legitimate result, or throw an exception and accept the performance cost and control-flow disruption that came with treating an ordinary, expected outcome as if it were catastrophic. Each of these approaches worked, but each also left room for silent failure. A forgotten null check became a crash. A misunderstood sentinel became a bug. An overused exception became a maintenance headache. The language had the tools to build correct programs, but not the vocabulary to make absence and failure impossible to ignore.

This is the problem that `std::optional` and, later, `std::expected` were designed to solve. Neither type introduces new computational power to the language; nothing achievable with them was previously impossible. Instead, they introduce clarity. They take an implicit, easily forgotten possibility and turn it into an explicit part of a function's signature, one that the type system itself insists be acknowledged before the underlying value can be touched.

## std::optional (since C++17)

`std::optional<T>` represents a value of type `T` that may be present, or it may not be. That is the entire premise. It does not explain why a value might be missing, nor does it distinguish between different kinds of absence. It simply acknowledges that absence is possible and asks the calling code to check before proceeding.

Consider a function that may or may not produce a result:

```cpp
#include <optional>

std::optional<int> find_value(bool condition) {
    if (condition) {
        return 42;
    }
    return std::nullopt; // no value
}

auto result = find_value(true);
if (result.has_value()) {
    std::cout << result.value();
}

// Alternative checks
if (result) {
    std::cout << *result; // dereference like a pointer
}
```

The `find_value` either hands back a meaningful integer or explicitly returns `std::nullopt`, a constant representing emptiness. Nothing about this return type allows the caller to accidentally treat a missing value as though it were real; the check, whether written as `has_value()` or as the object used directly in a boolean condition, becomes a natural and necessary step in the flow of the program rather than an easily skipped afterthought.

`std::optional` offers a small cast of supporting functions, each suited to a different way of interacting with uncertainty. 
- `has_value()` — returns true if a value is present. This is the most explicit and readable way to check state, and is functionally equivalent to using the object directly in a boolean context.
- `value()` — returns the contained value; throws `std::bad_optional_access` if empty. This function is useful when absence should be treated as a programming error worth stopping execution for, since it converts a missing value into an exception rather than undefined behavior.
- `value_or(default)` — returns the value if present, otherwise a fallback. This is convenient for cases where a sensible default can be substituted without needing to branch explicitly.
- `operator*` and `operator->` — access the value without bounds checking (undefined behavior if empty). These operators mirror pointer syntax and are efficient, but they place full responsibility on the caller to have already verified that a value exists.
- `reset()` — clears the contained value, returning the `optional` to the empty state and destroying any previously held object.
- `emplace(args...)` — constructs a new value in place using the given constructor arguments, avoiding an intermediate temporary object and any unnecessary copying or moving.


This makes `optional` a natural fit wherever a function's result is legitimately allowed to be nothing: a search through a container that might not find a match, a configuration setting that might deliberately be left unset, a class member whose initialization must wait until later in an object's lifetime. In each of these cases, the absence of a value is not an error condition, only a normal branch in the story a program is telling.

But `optional`'s narrative has a limit. It can say that something is missing, but it cannot say why. If a function might fail for several distinct reasons, `optional` collapses all of them into the same undifferentiated emptiness. It also carries a small cost, usually the overhead of one extra byte or word to track whether a value is present, with some specializations like `optional<bool>` handling this internal bookkeeping with additional care.

## std::expected (since C++23)

Where `std::optional` speaks only of presence and absence, `std::expected<T, E>`, speaks of success and failure, and it insists on explaining the failure when it happens. An `expected<T, E>` object holds either a value of type `T`, representing success, or an error of type `E`, representing a specific, describable reason things went wrong. This is the natural next chapter in the story: once a program acknowledges that operations can fail, the next question is inevitably why, and `expected` exists to answer it.

```cpp
#include <expected>

std::expected<int, std::string> parse_number(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (...) {
        return std::unexpected("invalid number");
    }
}

auto result = parse_number("123");
if (result.has_value()) {
    std::cout << *result;
} else {
    std::cout << "Error: " << result.error();
}
```

In this example, `parse_number` attempts a conversion that might reasonably fail. Rather than letting an exception escape and disrupt the surrounding code, the function catches the failure internally and translates it into a `std::unexpected` object carrying a human-readable explanation. The caller, in turn, is offered a complete account of what happened, either the parsed integer or a message describing precisely what went wrong, and can choose how to respond accordingly.

The member functions of `expected` echo those of `optional` but extend the vocabulary to accommodate error information. 
- `has_value()` — returns true if a valid result is present, following the same convention as `optional`.
- `value()` — returns the value; throws `std::bad_expected_access<E>` if not present. The thrown exception carries the stored error object, so information is not lost even if the exception path is taken.
- `error()` — returns the stored error, but is only valid to call when `has_value()` is false; calling it when a valid value is present is undefined behavior.
- `value_or(default)` — returns the value or a fallback, similar in spirit to the equivalent function on `optional`.
- `and_then()`, `or_else()`, `transform()`, `transform_error()` — monadic operations that allow chaining a sequence of operations together, where each step only executes if the previous one succeeded, without requiring explicit branching or manual error checks at every stage.

Where `expected` distinguishes itself most is in its monadic operations, `and_then()`, `or_else()`, `transform()`, and `transform_error()`, which allow chains of fallible operations to be composed without descending into nested conditionals:

```cpp
auto result = parse_number("42")
    .and_then([](int n) -> std::expected<int, std::string> {
        return n * 2;
    })
    .transform([](int n) {
        return n + 1;
    });
```

Here, `and_then` applies a function that itself might fail, automatically short-circuiting the chain if an earlier step already went wrong. The `transform` is reserved for steps that cannot fail on their own, simply reshaping a successful value into something new. The result is a sequence of operations that reads almost like a narrative of transformations, each one trusting that the ones before it either succeeded or already handled their own failure.

This makes `std::expected` well suited to situations where failure is not an exceptional event but a routine possibility: parsing untrusted input, performing I/O, validating data, or communicating across an interface where structured error information matters more than a raw exception. It also finds a home in environments where exceptions themselves are disabled or discouraged, such as certain embedded systems or performance-sensitive code paths.

Yet `std::expected`'s more elaborate plot comes with its own costs. As of mid-2026, C++23 support is still not universal across compilers and standard libraries, so its availability cannot always be assumed. Choosing and consistently handling an appropriate error type also requires more upfront design than the simple presence-or-absence model of `std::optional`. And like its predecessor, `std::expected` is not meant to replace exceptions altogether; catastrophic, unrecoverable conditions, such as running out of memory, still belong to the older, more dramatic tradition of exception handling.

## Conclusion

Told side by side, the two types form a natural progression rather than a competition. `std::optional` answers the simplest possible question: does a value exist? `expected` answers a more detailed one: if something went wrong, what exactly happened? Neither is meant to dominate the other, and neither is meant to replace exceptions, which remain the appropriate mechanism for truly exceptional, program-halting conditions.

| Aspect | std::optional | std::expected |
|---|---|---|
| Represents | Value or nothing | Value or error with details |
| Introduced | C++17 | C++23 |
| Error information | None | Carries a typed error (`E`) |
| Common use | Optional data, absence of result | Fallible operations, structured error handling |
| Monadic operations | `and_then`, `or_else`, `transform` (C++23 additions to optional) | `and_then`, `or_else`, `transform`, `transform_error` |

The choice between them comes down to how much explanation a failure deserves. When only presence or absence matters, `std::optional` tells the whole story in the simplest possible terms. When failure can arise from multiple distinguishable causes, and the calling code stands to benefit from knowing which one occurred, `std::expected` takes over the narration.

What unites both types, ultimately, is a shift in philosophy: rather than trusting a programmer to remember an implicit convention, null pointers that might be dereferenced, sentinel values that might be misread, or exceptions used for entirely ordinary outcomes, the type system itself becomes a participant in the story, insisting that missing values and failed operations be acknowledged before the program is allowed to proceed.

## std::optional and std::expected Together — Functor, Applicative, Monad
Consider a scenario: get an optional name, and an expected age (parsed from text, which can fail with a reason).

```cpp
#include <optional>
#include <expected>
#include <string>
#include <iostream>

std::expected<int, std::string> parse_age(const std::string& text) {
    try {
        int age = std::stoi(text);
        if (age < 0) return std::unexpected("age cannot be negative");
        return age;
    } catch (...) {
        return std::unexpected("age is not a number");
    }
}
```

## Functor — transform

Apply a plain function to the value inside, if present.

```cpp
std::optional<std::string> name = "alice";
auto shout = name.transform([](std::string s) {
    for (auto& c : s) c = toupper(c);
    return s;
});
// shout contains "ALICE"
```

## Applicative — combine independent values

`name` (optional) and `age` (expected) are fetched independently, then combined.

```cpp
std::optional<std::string> name = "bob";
std::expected<int, std::string> age = parse_age("30");

if (name.has_value() && age.has_value()) {
    std::cout << *name << " is " << *age << " years old\n";
} else {
    std::cout << "missing info\n";
}
```

Both values are produced separately — one does not depend on the other — but combining them requires both to be present.

## Monad — chain a dependent step

Suppose age must then be checked against a minimum required age, a step depending on the previous result.

```cpp
std::expected<int, std::string> check_adult(int age) {
    if (age < 18) return std::unexpected("must be 18 or older");
    return age;
}

auto result = parse_age("25").and_then(check_adult);

if (result.has_value()) {
    std::cout << "Age accepted: " << *result << "\n";
} else {
    std::cout << "Rejected: " << result.error() << "\n";
}
```

`and_then` only runs `check_adult` if `parse_age` already succeeded; a parse failure skips straight to the error.

## Putting it together
```cpp
int main() {
    // --- Functor: transform the value inside, unconditionally on presence
    std::optional<std::string> name = "bob";
    auto shout = name.transform([](std::string s) {
        for (auto& c : s) c = std::toupper(c);
        return s;
    });
    // shout contains "BOB"

    // --- Monad: chain dependent, fallible steps
    auto validated_age = parse_age("25").and_then(check_adult);
    // parse_age must succeed before check_adult runs at all

    // --- Applicative: combine two independently-obtained results
    if (shout.has_value() && validated_age.has_value()) {
        std::cout << *shout << " is " << *validated_age << " years old\n";
    } else {
        if (!shout.has_value())
            std::cout << "Error: name missing\n";
        if (!validated_age.has_value())
            std::cout << "Error: " << validated_age.error() << "\n";
    }
}
```

Sources:
* https://youtu.be/fTbTF0MUsPA?si=mqMtpHRJcUHIMQRn
* https://youtu.be/cSOzD78yQV4?si=nHfckMtFk4biFnLK
* https://youtu.be/_Ivd3qzgT7U?si=es33rlyjYOX7kwpt
* https://youtu.be/0ojB8c0xUd8?si=Y06vrpdg5K5y6q6f
