#include <iostream>
#include <vector>
#include <string>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <execution>

struct Tx {
    std::string user;
    double amount;
    bool approved;
};

struct Metrics {
    double total = 0.0;
    double average = 0.0;
    double max = -1e9;
    double min = 1e9;
    double median = 0.0;
    int count = 0;
    int high_value = 0;
    int small = 0;
    int medium = 0;
    int large = 0;
    bool all_positive = true;
    bool any_very_large = false;
    bool none_negative = true;
    std::vector<double> running;
    std::vector<double> sorted_desc;
};

Metrics compute_metrics(const std::vector<Tx>& transactions) {
    // 🔹 Lazy pipeline
    auto pipeline =
        transactions
        | std::views::filter([](const Tx& t){ return t.approved; })
        | std::views::transform([](const Tx& t){ return t.amount * 0.98; }) // 2% fee
        | std::views::filter([](double amt){ return amt >= 10; });

    Metrics metrics;

    // 🔹 Materialize for parallel algorithms (needed for execution policies)
    std::vector<double> amounts(pipeline.begin(), pipeline.end());
    metrics.sorted_desc = amounts; // save for descending sort later

    metrics.count = amounts.size();

    if(metrics.count == 0) return metrics;

    // 🔹 Total revenue (parallel transform_reduce)
    metrics.total = std::transform_reduce(
        std::execution::par,
        amounts.begin(), amounts.end(),
        0.0,
        std::plus<>(),
        [](double amt){ return amt; }
    );

    metrics.average = metrics.total / metrics.count;

    // 🔹 Max / Min (parallel)
    metrics.max = *std::max_element(std::execution::par, amounts.begin(), amounts.end());
    metrics.min = *std::min_element(std::execution::par, amounts.begin(), amounts.end());

    // 🔹 Boolean checks (parallel)
    metrics.all_positive = std::all_of(std::execution::par, amounts.begin(), amounts.end(),
                                       [](double amt){ return amt > 0; });
    metrics.any_very_large = std::any_of(std::execution::par, amounts.begin(), amounts.end(),
                                         [](double amt){ return amt > 2000; });
    metrics.none_negative = std::none_of(std::execution::par, amounts.begin(), amounts.end(),
                                         [](double amt){ return amt < 0; });

    // 🔹 Category counts & high-value counts (single pass)
    std::ranges::for_each(amounts, [&](double amt){
        metrics.high_value += (amt > 1000);
        if(amt < 100) metrics.small++;
        else if(amt < 1000) metrics.medium++;
        else metrics.large++;
    });

    // 🔹 Running revenue (prefix sum)
    metrics.running.resize(amounts.size());
    std::inclusive_scan(std::execution::par, amounts.begin(), amounts.end(),
                        metrics.running.begin(), std::plus<>());

    // 🔹 Descending sort & median
    std::ranges::sort(metrics.sorted_desc, std::greater<>());
    auto mid = metrics.sorted_desc.begin() + metrics.sorted_desc.size() / 2;
    std::ranges::nth_element(metrics.sorted_desc, mid);
    metrics.median = *mid;

    return metrics;
}

int main() {
    std::vector<Tx> transactions = {
        {"Alice", 1200, true},
        {"Bob", 5, true},
        {"Charlie", 300, false},
        {"Diana", 80, true},
        {"Eve", 2500, true},
        {"Frank", 15, true},
        {"George", 1800, true}
    };

    Metrics m = compute_metrics(transactions);

    std::cout << "Total Revenue: " << m.total << "\n";
    std::cout << "Average Transaction: " << m.average << "\n";
    std::cout << "Max Transaction: " << m.max << "\n";
    std::cout << "Min Transaction: " << m.min << "\n";
    std::cout << "Median Transaction: " << m.median << "\n";
    std::cout << "All Positive: " << (m.all_positive ? "Yes" : "No") << "\n";
    std::cout << "Any >2000: " << (m.any_very_large ? "Yes" : "No") << "\n";
    std::cout << "None Negative: " << (m.none_negative ? "Yes" : "No") << "\n";
    std::cout << "High-Value Transactions (>1000): " << m.high_value << "\n";
    std::cout << "Category Counts -> Small: " << m.small
              << ", Medium: " << m.medium
              << ", Large: " << m.large << "\n\n";

    std::cout << "Running Revenue: ";
    for(double r : m.running) std::cout << r << " ";
    std::cout << "\nSorted Descending Transactions: ";
    for(double s : m.sorted_desc) std::cout << s << " ";
}
