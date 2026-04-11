#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

using namespace std;
using namespace boost::adaptors;

int main() {
    vector<int> nums = {1, 2, 3, 4, 5};

    // Bug 0: Dangling range reference
    // Hint: auto& binds to a temporary adaptor; when the statement ends, the range is gone.
    const auto& squares = nums | transformed([](int x) { return x * x; }); // dangling

    // Bug 1: Sort on a temporary adaptor range
    // Hint: boost::range::sort expects a real container, not a lazy, temporary range.
    boost::range::sort(squares); // wrong range type

    // Bug 2: Wrong adaptor order (logic error)
    // Hint: transform runs on the original nums first; to filter after squaring, swap adaptors.
    auto filtered_squares = nums
        | transformed([](int x) { return x * x; })
        | filtered([](int x) { return x >= 10; }); // logic wrong

    // Bug 3: Copy into a range (not a container)
    // Hint: boost::range::copy needs an output iterator or container; you cannot pass a range object.
    vector<int> copy_vec;
    boost::range::copy(filtered_squares, nums); // wrong destination

    // Bug 4: Mutable lambda in transformed
    // Hint: Boost.Range adaptors are lazy and may re‑evaluate; mutating state is unsafe.
    int offset = 0;
    auto offsetted = nums | transformed([&offset](int x) { return x + offset++; }); // bad

    // Bug 5: Iterator into a temporary range
    // Hint: temp_range is temporary; its iterators are dangling when used in the loop.
    auto&& temp_range = nums | filtered([](int x) { return x % 2 == 0; });
    auto it1 = boost::begin(temp_range);
    auto it2 = boost::end(temp_range);
    for (auto it = it1; it != it2; ++it) { // iterators may point into dead range
        cout << *it << " ";
    }
    cout << endl;

    // Bug 6: Fictional Boost.Range function
    // Hint: apply_filtered is not a Boost.Range function; this line is made up.
    // boost::range::apply_filtered(nums, [](int x) { return x > 3; }); // fictional

    // Bug 7: dropped on an empty range
    // Hint: dropped(1) on an empty range leads to UB if you dereference begin.
    auto dropped = nums | filtered([](int x) { return x > 100; }) | dropped(1); // likely empty
    cout << "First after drop: " << *boost::begin(dropped) << endl; // may crash

    // Bug 8: remove_if vs remove_erase_if
    // Hint: remove_if moves but does not shrink the container; remove_erase_if actually erases.
    boost::range::remove_if(nums, [](int x) { return x % 2 == 1; }); // does not shrink

    // Bug 9: unique_copy with const range and wrong destination
    // Hint: unique_copy needs an output iterator; a const range cannot be the destination.
    const auto& const_nums = nums;
    vector<int> unique_vec;
    boost::range::unique_copy(const_nums, back_inserter(unique_vec)); // const viol. / confusion

    // Bug 10: remove_erase_if on an iterator_range
    // Hint: remove_erase_if expects a container with erase(); iterator_range is not enough.
    boost::range::remove_erase_if(boost::make_iterator_range(nums.begin(), nums.end()),
        [](int x) { return x < 0; }); // wrong argument type

    // Bug 11: sliced on a temporary container (dangling)
    // Hint: The vector in the initializer is temporary; the sliced range is dangling.
    const auto& sliced_ref = vector<int>{1, 2, 3, 4, 5} | sliced(1, 4); // dangling

    // Bug 12: Lambda in transformed capturing local by reference
    // Hint: If the lambda is used later, the captured local no longer exists.
    {
        int local = 42;
        auto with_local = nums | transformed([&local](int x) { return x + local; }); // UB if used later
    }

    // Bug 13: Range over a temporary container from a lambda
    // Hint: The returned vector is temporary; the range is invalid after the call.
    auto make_range = []() {
        return vector<int>{1, 2, 3} | filtered([](int x) { return x > 1; });
    };
    auto&& r = make_range(); // r is a reference to a temporary

    // Bug 14: Assuming random access on transformed range (operator[])
    // Hint: transformed ranges are not guaranteed random access; operator[] may not be valid.
    auto indexed = nums | transformed([](int x) { return x * x; });
    cout << "Indexed access: " << indexed[2] << endl; // not guaranteed random access

    // Bug 15: Copy into transformed range (nonsensical)
    // Hint: boost::range::copy works into containers, not into another transformed range.
    vector<int> tmp_vec;
    boost::range::copy(nums,
        tmp_vec | transformed([](int x) { return x * x; })
    ); // wrong destination

    // Bug 16: Wrong overload for std::for_each
    // Hint: std::for_each expects iterator pairs; you cannot pass a range object directly.
    std::for_each(squares, std::greater<int>{}); // wrong overload

    // Bug 17: Dereferencing range::find result without checking end
    // Hint: find may return end; dereferencing end is undefined behavior.
    auto found_range = nums | filtered([](int x) { return x > 10; });
    auto pos = boost::range::find(found_range, 11);
    cout << "Found: " << *pos << endl; // may be end

    // Bug 18: Using erase on a range adaptor
    // Hint: erase is a container member; you cannot call it on a generic range adaptor.
    (nums | filtered([](int x) { return x > 3; })).erase( // nonsense
        nums.begin(), nums.end()
    );

    // Bug 19: Assuming range adaptors are copyable / moveable
    // Hint: Some adaptors are not well‑copyable; storing them in containers is unsafe.
    vector<decltype(squares)> vec_of_ranges; // potentially invalid type

    // Bug 20: std::random_shuffle on a transformed range
    // Hint: std::random_shuffle is deprecated and assumes random access, not a transformed range.
    std::random_shuffle(squares); // wrong range category

    // Bug 21: Reversing a temporary range and binding a reference
    // Hint: reversed range is lazy; binding a reference to it is fragile.
    const auto& reversed_ref = nums | reversed; // fragile lifetime
    for (int x : reversed_ref) {
        cout << x << " ";
    }
    cout << endl;

    // Bug 22: Using drop on a non‑sized range
    // Hint: drop expects a sized range; filtered ranges are not always sized.
    auto dropped2 = nums | filtered([](int x) { return x % 2 == 0; }) | dropped(100); // may be empty
    cout << "dropped2 front: " << *boost::begin(dropped2) << endl; // may crash

    // Bug 23: count_if with wrong predicate type
    // Hint: count_if expects a predicate returning bool, not a comparison object like std::greater.
    auto count = boost::range::count_if(nums, std::greater<int>{}); // wrong predicate type

    // Bug 24: Applying sort on a temporary range in a scope
    // Hint: local_range is temporary; applying sort on it is undefined behavior.
    {
        auto&& local_range = nums | filtered([](int x) { return x > 2; });
        boost::range::sort(local_range); // local_range is temporary; range is destroyed
    } // local_range dies; sort modifies a dead range (UB)

    // Bug 25: Reusing range over a moved container
    // Hint: Moving a container invalidates iterators and ranges to the old storage.
    vector<int> moved_nums = move(nums);
    auto moved_squares = moved_nums | transformed([](int x) { return x * x; });
    // If nums is moved from and later used, behavior is undefined.

    return 0;
}
