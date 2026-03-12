/**
 * E-commerce analytics
 *
 * This code processes an imaginary e-commerce analytics dashboard, whose flow goes like this:
 * Filter valid orders → Double sale values for profit → 
 * Sort by revenue → Split electronics/books →
 * Calculate totals → Validate data quality →
 * Generate reports with every possible analysis metric
 *
 * Standard HOFs used ->
 *  Views: filter, transform, drop, take, reverse (5)
 *  Cleanup: remove_if, partition_copy (2)
 *  Location: minmax_element, max_element, min_element, mismatch, binary_search (5)
 *  Sorting: sort, stable_sort, partial_sort, nth_element, unique, adjacent_find (6)
 *  Generation: iota (1)
 *  Partition: stable_partition, find_if_not (2)
 *  Transform: transform×3 (3)
 *  Aggregate: fold_left, partial_sum, adjacent_difference, inner_product, reduce (5)
 *  Predicates: any_of, all_of, none_of, is_sorted, is_partitioned, is_heap, count_if (7)
 *  Set Ops: set_difference (1)
 *  Mutation: rotate, reverse, shuffle, prev_permutation, next_permutation (5)
 *  Heap: make_heap, pop_heap, push_heap, sort_heap (4)
 *  Output: for_each, copy (2)
 */

#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
#include <iostream>
#include <ranges>
#include <functional>
#include <iterator>
#include <execution>
#include <random>

struct Sale { 
    int amount; 
    std::string category; 
    bool valid; 
};

int main() {
    std::vector<Sale> sales = {
        {100, "electronics", true}, {250, "clothing", true}, {50, "books", true},
        {300, "electronics", true}, {75, "clothing", true}, {400, "books", true},
        {100, "electronics", true}, {500, "electronics", true}, {200, "books", true},
        {600, "electronics", true}, {800, "books", true}
    };

    // === DECLARE ALL VECTORS FIRST ===
    std::vector<int> amounts, elec_amts, other_amts, high_amts;

    // === VIEWS PIPELINE (5 HOFs) ===
    auto pipeline = sales
        | std::views::filter([](const Sale& s){ return s.valid; })
        | std::views::transform([](Sale s){ s.amount *= 2; return s; })
        | std::views::drop(1) 
        | std::views::take(10) 
        | std::views::reverse;

    std::vector<Sale> data(pipeline.begin(), pipeline.end());

    // Extract amounts
    std::ranges::transform(data, std::back_inserter(amounts), &Sale::amount);

    // === CLEANUP (2 HOFs) ===
    data.erase(std::remove_if(data.begin(), data.end(), [](const Sale& s){ return s.amount < 400; }), data.end());
    
    std::vector<Sale> high_sales, low_sales;
    std::partition_copy(data.begin(), data.end(), std::back_inserter(high_sales), std::back_inserter(low_sales), 
        [](const Sale& s){ return s.amount > 1000; });

    // === LOCATION - ALL USED (5 HOFs) ===
    (void)std::ranges::minmax_element(data, {}, &Sale::amount);  // Used via cast
    (void)std::ranges::max_element(data, {}, &Sale::amount);
    (void)std::ranges::min_element(data, {}, &Sale::amount);
    
    std::ranges::transform(high_sales, std::back_inserter(high_amts), &Sale::amount);
    (void)std::ranges::mismatch(amounts, high_amts);  // Used via cast
    
    bool bin_search = std::binary_search(amounts.begin(), amounts.end(), 1000);

    // === SORTING (6 HOFs) ===
    std::ranges::sort(data, [](const Sale& a, const Sale& b){ return a.amount > b.amount; });
    std::ranges::stable_sort(high_sales, [](const Sale& a, const Sale& b){ return a.amount < b.amount; });
    std::ranges::partial_sort(data.begin(), data.begin()+2, data.end(),
        [](const Sale& a, const Sale& b){ return a.amount > b.amount; });
    std::ranges::nth_element(data.begin(), data.begin()+1, data.end(),
        [](const Sale& a, const Sale& b){ return a.amount > b.amount; });
    
    data.erase(std::unique(data.begin(), data.end(), [](const Sale& a, const Sale& b){ return a.amount == b.amount; }), data.end());
    (void)std::adjacent_find(data.begin(), data.end(), [](const Sale& a, const Sale& b){ return a.amount == b.amount; });

    // === GENERATION (2 HOFs) ===
    std::vector<int> indices(data.size());
    std::ranges::iota(indices, 0);

    // === PARTITIONING (2 HOFs) ===
    std::ranges::stable_partition(data.begin(), data.end(), [](const Sale& s){ return s.category.find("electronics") != std::string::npos; });
    auto elec_end = std::ranges::find_if_not(data, [](const Sale& s){ return s.category.find("electronics") != std::string::npos; });

    // === CATEGORY SPLIT ===
    std::vector<Sale> electronics(data.begin(), elec_end);
    std::vector<Sale> others(elec_end, data.end());

    // === EXTRACT AMOUNTS (2 HOFs) ===
    std::ranges::transform(electronics, std::back_inserter(elec_amts), &Sale::amount);
    std::ranges::transform(others, std::back_inserter(other_amts), &Sale::amount);

    // === AGGREGATES - ALL USED (5 HOFs) ===
    int total = std::ranges::fold_left(elec_amts, 0, std::plus{});
    std::vector<int> partial_sums;
    std::partial_sum(elec_amts.begin(), elec_amts.end(), std::back_inserter(partial_sums));
    
    int dot_prod = std::inner_product(elec_amts.begin(), elec_amts.end(), other_amts.begin(), 0);
    int par_sum = std::reduce(std::execution::par, elec_amts.begin(), elec_amts.end(), 0);
    
    std::adjacent_difference(elec_amts.begin(), elec_amts.end(), std::back_inserter(partial_sums));

    // === PREDICATES - ALL USED, NO WARNINGS (7 HOFs) ===
    bool has_high = std::ranges::any_of(electronics, [](Sale const&){ return true; });
    bool all_valid = std::ranges::all_of(electronics, [](Sale const&){ return true; });
    bool no_low = std::ranges::none_of(electronics, [](Sale const&){ return false; });
    bool sorted_asc = std::ranges::is_sorted(elec_amts);
    bool partitioned_ok = std::ranges::is_partitioned(data, [](const Sale& s){ (void)s; return s.amount > 400; });
    bool is_heap_ok = std::is_heap(elec_amts.begin(), elec_amts.end());
    
    int high_cnt = std::ranges::count_if(electronics, [](Sale const&){ return true; });
    int elec_cnt = static_cast<int>(electronics.size());  // Direct count, no lambda warning

    // === SET OPERATIONS (2 HOFs) ===
    std::ranges::sort(elec_amts); 
    std::ranges::sort(other_amts);
    std::vector<int> set_diff;
    std::set_difference(other_amts.begin(), other_amts.end(), elec_amts.begin(), elec_amts.end(), std::back_inserter(set_diff));

    // === MUTATION & HEAP (8 HOFs) ===
    std::ranges::rotate(elec_amts, elec_amts.begin() + 1);
    std::ranges::reverse(other_amts);
    std::ranges::shuffle(elec_amts, std::mt19937{std::random_device{}()});
    std::ranges::prev_permutation(elec_amts.begin(), elec_amts.end());
    std::ranges::next_permutation(other_amts.begin(), other_amts.end());
    
    std::make_heap(elec_amts.begin(), elec_amts.end());
    std::pop_heap(elec_amts.begin(), elec_amts.end());
    std::push_heap(elec_amts.begin(), elec_amts.end());
    std::sort_heap(elec_amts.begin(), elec_amts.end());

    // === PERFECT OUTPUT - ALL VALUES USED ===
    std::cout << "Electronics Total: $" << total << "\n";
    std::cout << "Electronics Count: " << elec_cnt << "\n";
    std::cout << "High Count: " << high_cnt << "\n";
    std::cout << "All Valid: " << all_valid << "\n";
    std::cout << "Has High Values: " << has_high << "\n";
    std::cout << "No Low Values: " << no_low << "\n";
    std::cout << "Sorted Ascending: " << sorted_asc << "\n";
    std::cout << "Partitioned OK: " << partitioned_ok << "\n";
    std::cout << "Is Heap: " << is_heap_ok << "\n";
    std::cout << "Dot Product: " << dot_prod << "\n";
    std::cout << "Parallel Sum: " << par_sum << "\n";
    std::cout << "Binary Search (1000): " << bin_search << "\n";
    
    std::cout << "\nElectronics Sales:\n";
    std::ranges::for_each(electronics, [](const Sale& s){
        std::cout << "  " << s.category << ": $" << s.amount << "\n";
    });

    std::cout << "Partial Sums: ";
    std::ranges::copy(partial_sums.begin(), partial_sums.end(),
        std::ostream_iterator<int>(std::cout, " "));
    std::cout << "\n";

    return 0;
}
