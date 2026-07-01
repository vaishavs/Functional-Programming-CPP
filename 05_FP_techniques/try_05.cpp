// Exercise 6: Monad — implementing and_then (bind) for a Logged<T> box
//
// GOAL:
// This exercise uses yet another shape: Logged<T> always holds a value —
// there is no failure state — but it also carries a log of messages
// (a std::vector<std::string>) describing what happened to produce that
// value. Chaining two Logged computations together must MERGE their logs
// using a monoid: vector concatenation, with the empty vector as the
// identity element and appending as the combining operation.
//
// and_then(w, f) should:
//   1. call f(w.value) to get the NEXT Logged<U>
//   2. combine the logs: the entries from `w` first, then the entries
//      from that next Logged, in order (this is the monoid combine)
//   3. return a Logged<U> holding the next value and the combined log
//
// TASK:
// Implement and_then(). Once it works, `pipeline` below (already written
// for you) will correctly chain three logged steps, accumulating the
// full history.

#include <iostream>
#include <string>
#include <vector>

// Logged<T>: a value that always exists, paired with an accumulated log.
template <typename T>
struct Logged {
    T value;
    std::vector<std::string> log;

    static Logged<T> of(T v, std::vector<std::string> l = {}) {
        return Logged<T>{std::move(v), std::move(l)};
    }
};

template <typename T, typename F>
auto and_then(Logged<T> w, F f) -> decltype(f(w.value))
{
    // TODO: implement this function.
    //
    // HINT: `decltype(f(w.value))` is already a Logged<something> — f
    //       returns a Logged on its own, that's what makes this "bind"
    //       rather than "transform".
    // HINT: compute `auto next = f(w.value);` first.
    // HINT: build the combined log by starting from a copy of `w.log`,
    //       then using `.insert(combined.end(), next.log.begin(), next.log.end())`
    //       to append `next`'s entries after it — this is vector
    //       concatenation, the monoid operation for this exercise.
    // HINT: return `ReturnType::of(next.value, combined_log)`.

    using ReturnType = decltype(f(w.value));
    return f(w.value); // <-- placeholder: drops w's log entirely, replace this
}

Logged<int> start(int n) {
    return Logged<int>::of(n, {"start with " + std::to_string(n)});
}

Logged<int> double_it(int n) {
    return Logged<int>::of(n * 2, {"doubled to " + std::to_string(n * 2)});
}

Logged<int> add_ten(int n) {
    return Logged<int>::of(n + 10, {"added 10 to get " + std::to_string(n + 10)});
}

Logged<int> pipeline(int n) {
    // Already implemented for you — once and_then() above works
    // correctly, this chain will behave as described below.
    return and_then(and_then(start(n), double_it), add_ten);
}

int main() {
    auto result = pipeline(5);

    // Expected output:
    // value: 20
    // log:
    //   start with 5
    //   doubled to 10
    //   added 10 to get 20

    std::cout << "value: " << result.value << "\n";
    std::cout << "log:\n";
    for (const auto& line : result.log) {
        std::cout << "  " << line << "\n";
    }
}
