// Exercise 9: DEBUG — a broken applicative combine
//
// SYMPTOM:
// apply2 is supposed to combine two independent std::optional values,
// returning std::nullopt if EITHER one is missing. Combining `width` and
// `height` below works fine when both are present, and correctly returns
// empty when `width` is missing. But when only `height` is missing, the
// program does NOT report "(empty)" — it silently prints some other
// number instead (the exact value is unspecified and may differ between
// compilers or runs).
//
// YOUR TASK:
// Find the bug and fix it. Do not add a workaround at the call site in
// main() — fix apply2 itself so it is safe for ANY combination of
// missing/present inputs.
//
// HINT: Look closely at the condition inside apply2. It checks `a`, but
//       does it check `b` too?
// HINT: Dereferencing an empty std::optional with `*b` is undefined
//       behavior. It will not necessarily throw a catchable exception
//       the way .value() does — it may just silently read whatever
//       bytes happen to be sitting in the optional's internal storage,
//       which is exactly what is happening here.
// HINT: The fix is a one-line change to the condition.

#include <optional>
#include <iostream>

template <typename F, typename T, typename U>
auto apply2(F f, std::optional<T> a, std::optional<U> b)
    -> std::optional<decltype(f(*a, *b))>
{
    // BUG IS HERE: only `a` is checked before dereferencing both a and b.
    if (!a.has_value()) return std::nullopt;
    return f(*a, *b);
}

int main() {
    std::optional<int> width = 10;
    std::optional<int> height = 4;
    std::optional<int> missing_width = std::nullopt;
    std::optional<int> missing_height = std::nullopt;

    // Expected output (once fixed):
    // both present: 40
    // width missing: (empty)
    // height missing: (empty)

    auto print = [](const char* label, std::optional<int> v) {
        std::cout << label << ": "
                  << (v.has_value() ? std::to_string(*v) : "(empty)") << "\n";
    };

    print("both present", apply2([](int w, int h) { return w * h; }, width, height));
    print("width missing", apply2([](int w, int h) { return w * h; }, missing_width, height));
    print("height missing", apply2([](int w, int h) { return w * h; }, width, missing_height));
}
