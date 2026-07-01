// Exercise 2: Functor — std::expected::transform
//
// GOAL:
// std::expected::transform(f) behaves like std::optional::transform, but
// on the success channel: if the expected holds a value, f is applied to
// it; if it holds an error, transform leaves the error untouched and
// passes it through — f is never called.
//
// TASK:
// Complete `to_uppercase` so that it uses transform() to uppercase the
// contained string, if the expected represents success.

#include <expected>
#include <string>
#include <iostream>
#include <cctype>

std::expected<std::string, std::string> to_uppercase(
    std::expected<std::string, std::string> input)
{
    // TODO: replace the line below with a call to input.transform(...)
    // that returns an uppercased copy of the string.
    //
    // HINT: transform takes a lambda: [](std::string s) { ... return s; }
    // HINT: loop over the string with a range-based for loop and apply
    //       std::toupper to each character, or use std::transform from
    //       <algorithm> if you prefer.
    // HINT: you do NOT need to check has_value() — if input holds an
    //       error, that error is passed through automatically.

    return input; // <-- placeholder, replace this
}

int main() {
    std::expected<std::string, std::string> good = "hello";
    std::expected<std::string, std::string> bad =
        std::unexpected("input was missing");

    auto result_good = to_uppercase(good);
    auto result_bad = to_uppercase(bad);

    // Expected output:
    // good -> HELLO
    // bad  -> error: input was missing

    std::cout << "good -> "
              << (result_good.has_value() ? *result_good : "error: " + result_good.error())
              << "\n";
    std::cout << "bad  -> "
              << (result_bad.has_value() ? *result_bad : "error: " + result_bad.error())
              << "\n";
}
