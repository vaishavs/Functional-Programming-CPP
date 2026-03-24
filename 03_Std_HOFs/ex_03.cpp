/* Works with C++23 and GCC v16 onwards*/
#include <iostream>
#include <vector>
#include <string>
#include <ranges>
#include <algorithm>
#include <functional> // Fixes 'function' is not a member of 'std'

struct Student {
    std::string name;
    int grade;
};

// 1. Custom View
template <std::ranges::view V>
class GradeFilterView : public std::ranges::view_interface<GradeFilterView<V>> {
private:
    // Storing the filtered view as a member to ensure it has a stable address
    using FilteredV = std::ranges::filter_view<V, std::function<bool(const Student&)>>;
    FilteredV filtered_base;

public:
    GradeFilterView() = default;

    // The constructor initializes the filter_view member
    GradeFilterView(V base, int min_grade)
        : filtered_base(std::move(base), [min_grade](const Student& s) {
            return s.grade >= min_grade;
          }) {}

    // begin() and end() now refer to the persistent member
    auto begin() { return std::ranges::begin(filtered_base); }
    auto end()   { return std::ranges::end(filtered_base); }
};

// 2. Deduction Guide
// This converts a container (like std::vector) into a view (like std::ranges::ref_view)
template <typename R>
GradeFilterView(R&&, int) -> GradeFilterView<std::views::all_t<R>>;

int main() {
    std::vector<Student> students = {
        {"Charlie", 95}, {"Alice", 82}, {"Bob", 55}, {"Diana", 70}
    };

    // 3. Projection
    // Sort the students by name using a member-pointer projection
    std::ranges::sort(students, std::ranges::less{}, &Student::name);

    // 4. Using the Custom View
    GradeFilterView honors_students(students, 80);

    std::cout << "Honors Students (Sorted by Name):\n";
    for (const auto& s : honors_students) {
        std::cout << " - " << s.name << ": " << s.grade << "\n";
    }

    return 0;
}   
