#include <iostream>
#include <variant>
#include <memory>
#include <functional>
#include <vector>
#include <ranges>
#include <string>

// Small helper so every section prints "Before" / "After" the same way
template <typename T>
void show(const std::string& label, const T& value) {
    std::cout << label << ": " << value << "\n";
}

// ------------------------------------------------------------
// 1. Box<T> — hand-rolled Maybe monad
// ------------------------------------------------------------
template <typename T>
struct Box {
    bool has_value;
    T value;
    static Box some(T v) { return {true, v}; }
    static Box none()    { return {false, T{}}; }

    template <typename F>
    auto and_then(F f) const { return has_value ? f(value) : decltype(f(value))::none(); }

    void print(const std::string& label) const {
        std::cout << label << ": " << (has_value ? std::to_string(value) : "none") << "\n";
    }
};

Box<int> half_if_even(int x) {
    return x % 2 == 0 ? Box<int>::some(x / 2) : Box<int>::none();
}

int main() {
    // --- 1. Box<T> monad ---
    std::cout << "=== Box<T> (Maybe monad) ===\n";
    Box<int> before = Box<int>::some(20);
    before.print("Before");
    before.and_then(half_if_even).print("After");   // 20 -> 10
    std::cout << "\n";

    // --- 2. std::variant sum type ---
    std::cout << "=== std::variant ===\n";
    std::variant<int, std::string> v = 42;
    show("Before", std::get<int>(v));
    v = "hello";
    show("After", std::get<std::string>(v));
    std::cout << "\n";

    // --- 3. std::shared_ptr ownership box ---
    std::cout << "=== std::shared_ptr ===\n";
    auto w = std::make_shared<int>(7);
    show("Before (refcount)", w.use_count());
    { auto w2 = w; show("After (refcount, extra owner)", w.use_count()); }
    show("After (refcount, scope ended)", w.use_count());
    std::cout << "\n";

    // --- 4. std::function callable box ---
    std::cout << "=== std::function ===\n";
    std::function<int(int)> doubler = [](int x) { return x * 2; };
    show("Before", 21);
    show("After", doubler(21));
    std::cout << "\n";

    // --- 5. ranges/views pipeline ---
    std::cout << "=== ranges/views ===\n";
    std::vector<int> nums = {1, 2, 3, 4, 5};
    std::cout << "Before: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << "\nAfter:  ";
    for (int n : nums | std::views::filter([](int n){ return n % 2 == 0; })
                      | std::views::transform([](int n){ return n * n; }))
        std::cout << n << " ";
    std::cout << "\n";
}
