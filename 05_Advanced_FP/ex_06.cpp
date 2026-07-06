#include <optional>
#include <expected>
#include <string>
#include <iostream>
#include <cctype>

// Parsing step: can fail with a specific reason -> std::expected
std::expected<int, std::string> parse_age(const std::string& text) {
    try {
        int age = std::stoi(text);
        if (age < 0) return std::unexpected("age cannot be negative");
        return age;
    } catch (...) {
        return std::unexpected("age is not a number");
    }
}

// Validation step: depends on a successfully parsed age
std::expected<int, std::string> check_adult(int age) {
    if (age < 18) return std::unexpected("must be 18 or older");
    return age;
}

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
