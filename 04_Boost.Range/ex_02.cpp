/*
Employee Analytics Pipeline

The flow of this example is as follows:
1. Filter only active employees
2. Extract salaries
3. Apply a bonus transformation
4. Compute total payroll
5. Display processed results
 */

#include <iostream>
#include <vector>
#include <string>
#include <numeric>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

using namespace std;
namespace adaptors = boost::adaptors;

// ----------------------------
// Domain Model
// ----------------------------
struct Employee {
    int id;
    string name;
    double salary;
    bool active;
};

// ----------------------------
// Utility Printer
// ----------------------------
template <typename Range>
void print_range(const Range& r, const string& label) {
    cout << label << ":\n";
    for (const auto& val : r) {
        cout << val << " ";
    }
    cout << "\n\n";
}

// ----------------------------
// Main Pipeline
// ----------------------------
int main() {

    vector<Employee> employees = {
        {1, "Alice", 50000, true},
        {2, "Bob", 60000, false},
        {3, "Charlie", 70000, true},
        {4, "David", 55000, true},
        {5, "Eve", 65000, false}
    };

    // 1. Filter active employees
    auto active_employees = employees 
        | adaptors::filtered([](const Employee& e) {
            return e.active;
        });

    // 2. Extract salaries
    auto salaries = active_employees
        | adaptors::transformed([](const Employee& e) {
            return e.salary;
        });

    // 3. Apply 10% bonus
    auto boosted_salaries = salaries
        | adaptors::transformed([](double s) {
            return s * 1.10;
        });

    // 4. Compute total payroll
    double total = accumulate(
        boosted_salaries.begin(),
        boosted_salaries.end(),
        0.0
    );

    // 5. Print results
    vector<double> result;
    boost::copy(boosted_salaries, back_inserter(result));

    print_range(result, "Boosted Salaries");
    cout << "Total Payroll: " << total << endl;

    return 0;
}
