/**
 * Logistic fleet management
 */

#include <iostream>
#include <vector>
#include <string>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <execution>
#include <functional>

// --- Data Structures ---

struct Telemetry {
    std::string truck_id;
    double miles_driven;
    double fuel_used_gallons;
    int engine_hours;
    bool maintenance_required;
    int safety_score; // 0-100
};

struct FleetReport {
    double total_mileage = 0.0;
    double avg_efficiency = 0.0; // MPG
    double max_miles_single_trip = 0.0;
    int critical_maintenance_count = 0;
    int fleet_size = 0;
    
    bool operational_integrity = true; // All safety scores > threshold
    bool high_utilization_detected = false; // Any truck > X miles
    
    std::vector<double> efficiency_distribution;
    std::vector<double> cumulative_mileage;
};

// --- Higher-Order Function Factories (The "Policy" Engine) ---

// Returns a predicate: Checks if a truck is "Active" based on a score HOF
auto is_active_fleet(int min_safety) {
    return [=](const Telemetry& t) { 
        return !t.maintenance_required && t.safety_score >= min_safety; 
    };
}

// Returns a transformation: Calculates MPG (Miles Per Gallon)
auto calculate_mpg() {
    return [](const Telemetry& t) {
        return t.fuel_used_gallons > 0 ? t.miles_driven / t.fuel_used_gallons : 0.0;
    };
}

// Returns a comparator: Used for sorting HOFs
auto by_efficiency() {
    return std::greater<double>();
}

// --- The Processing Engine ---

FleetReport generate_fleet_report(const std::vector<Telemetry>& logs) {
    // 1. Functional Pipeline (HOF Composition)
    auto active_efficiency_view = logs 

        | std::views::filter(is_active_fleet(70))  // HOF Predicate
        | std::views::transform(calculate_mpg())  // HOF Transformation
        | std::views::filter([](double mpg) { return mpg > 2.0; }); // Noise filter

    // 2. Materialize only what's needed for heavy analysis
    std::vector<double> efficiencies(active_efficiency_view.begin(), active_efficiency_view.end());
    
    FleetReport report;
    report.fleet_size = efficiencies.size();
    if (report.fleet_size == 0) return report;

    // 3. Parallel Reductions (Higher Order Algorithms)
    report.avg_efficiency = std::reduce(std::execution::par, efficiencies.begin(), efficiencies.end(), 0.0) 
                            / report.fleet_size;

    // 4. Extracting raw mileage for specific stats
    auto mileage_view = logs | std::views::transform(&Telemetry::miles_driven);
    std::vector<double> mileages(mileage_view.begin(), mileage_view.end());

    report.total_mileage = std::reduce(std::execution::par, mileages.begin(), mileages.end(), 0.0);
    report.max_miles_single_trip = *std::max_element(std::execution::par, mileages.begin(), mileages.end());

    // 5. Parallel Boolean Policies
    report.operational_integrity = std::all_of(std::execution::par, logs.begin(), logs.end(), 
        [](const Telemetry& t) { return t.safety_score > 50; });

    report.high_utilization_detected = std::any_of(std::execution::par, mileages.begin(), mileages.end(), 
        [](double m) { return m > 800.0; });

    // 6. Stateful Scan (Prefix Sum for Fuel/Mileage Trend)
    report.cumulative_mileage.resize(mileages.size());
    std::inclusive_scan(std::execution::par, mileages.begin(), mileages.end(), 
                        report.cumulative_mileage.begin());

    // 7. Advanced Sorting using HOF Comparator
    report.efficiency_distribution = efficiencies;
    std::sort(std::execution::par, report.efficiency_distribution.begin(), 
              report.efficiency_distribution.end(), by_efficiency());

    return report;
}

int main() {
    std::vector<Telemetry> fleet_logs = {
        {"T-01", 450.5, 45.0, 12, false, 95},
        {"T-02", 820.0, 92.0, 20, false, 88},
        {"T-03", 120.0, 15.0, 4,  true,  40}, // Maintenance required
        {"T-04", 600.0, 55.0, 15, false, 92},
        {"T-05", 950.0, 110.0, 24, false, 85}
    };

    FleetReport r = generate_fleet_report(fleet_logs);

    std::cout << "=== Fleet Operations Report ===\n";
    std::cout << "Active Units:        " << r.fleet_size << "\n";
    std::cout << "Total Fleet Miles:   " << r.total_mileage << "\n";
    std::cout << "Avg Fuel Efficiency: " << r.avg_efficiency << " MPG\n";
    std::cout << "Operational Safety:  " << (r.operational_integrity ? "PASS" : "FAIL") << "\n";
    std::cout << "High Util. Alert:    " << (r.high_utilization_detected ? "YES" : "NO") << "\n";
    
    std::cout << "\nEfficiency Leaderboard (MPG):\n";
    for(auto val : r.efficiency_distribution) {
        std::cout << " > " << val << "\n";
    }

    return 0;
}
