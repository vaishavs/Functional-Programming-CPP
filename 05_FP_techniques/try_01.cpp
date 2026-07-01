// Exercise 7: DEBUG — a broken "functor-style" transformation
//
// SYMPTOM:
// This program is supposed to print the doubled value of each optional in
// `readings`, printing "(no reading)" for any missing entry. Instead, it
// crashes (throws std::bad_optional_access) partway through the list.
//
// YOUR TASK:
// Find the bug and fix it. Do not add a manual if/else around a call to
// .value() — instead, fix `double_reading` so it uses transform(), which
// is the whole point of using std::optional as a functor: it should be
// impossible to crash on an empty optional in the first place.
//
// HINT: Look closely at double_reading(). It calls .value() unconditionally.
//       .value() throws if the optional is empty — that is the bug.
// HINT: Replace the body with something using `.transform(...)`, which
//       safely handles the empty case for you and returns another
//       std::optional automatically.
// HINT: transform's lambda only needs to describe what happens to the
//       value WHEN IT EXISTS — you don't write any empty-case logic at all.

#include <optional>
#include <vector>
#include <iostream>

std::optional<double> double_reading(std::optional<double> reading) {
    // BUG IS HERE:
    double v = reading.value(); // <-- throws if `reading` is empty!
    return v * 2.0;
}

int main() {
    std::vector<std::optional<double>> readings = {
        3.5, std::nullopt, 7.0, std::nullopt, 1.25
    };

    // Expected output (once fixed):
    // 7
    // (no reading)
    // 14
    // (no reading)
    // 2.5

    for (const auto& r : readings) {
        auto doubled = double_reading(r);
        if (doubled.has_value()) {
            std::cout << *doubled << "\n";
        } else {
            std::cout << "(no reading)\n";
        }
    }
}
