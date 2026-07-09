// Exercise: DEBUG — a broken monad chain (this file currently FAILS TO
// COMPILE)
//
// SYMPTOM:
// The chain below is supposed to take a raw score, clamp it into the
// range [0, 100], then convert it to a letter grade — short-circuiting
// with an error if the raw score was negative (which should never
// legitimately happen). Instead, it fails to compile.
//
// YOUR TASK:
// Read the compiler error carefully, find the mismatch, and fix it.
//
// HINT: and_then requires every step's lambda to return an
//       std::expected<U, E> with the SAME error type E as the expected
//       it is called on — not a plain U.
// HINT: Look at the lambda passed to the second `.and_then(...)` call.
//       What type does it currently return? What type should it return?
// HINT: The fix is a one-line change: wrap the plain return value so it
//       becomes a valid std::expected<std::string, std::string>.

#include <expected>
#include <string>
#include <iostream>

std::expected<int, std::string> validate_score(int raw) {
    if (raw < 0) return std::unexpected("score cannot be negative");
    return raw;
}

std::expected<int, std::string> clamp_score(int score) {
    if (score > 100) return 100;
    return score;
}

std::expected<std::string, std::string> grade_from_score(int score) {
    return validate_score(score)
        .and_then(clamp_score)
        .and_then([](int clamped) {
            // BUG IS HERE: this lambda returns a plain std::string, but
            // and_then needs an std::expected<std::string, std::string>.
            if (clamped >= 90) return std::string("A");
            if (clamped >= 80) return std::string("B");
            if (clamped >= 70) return std::string("C");
            return std::string("F");
        });
}

int main() {
    // Expected output (once it compiles):
    // 95  -> A
    // 250 -> A   (clamped to 100 first)
    // 72  -> C
    // -5  -> error: score cannot be negative

    for (int raw : {95, 250, 72, -5}) {
        auto result = grade_from_score(raw);
        std::cout << raw << " -> "
                  << (result.has_value() ? *result : "error: " + result.error())
                  << "\n";
    }
}
