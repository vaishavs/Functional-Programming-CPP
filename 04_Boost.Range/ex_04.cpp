/* Extending Boost.Range for UDT */
#include <iostream>
#include <vector>
#include <string>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

struct Log {
    std::string level;
    std::string message;
};

class LogStore {
    std::vector<Log> logs;

public:
    using iterator = std::vector<Log>::iterator;
    using const_iterator = std::vector<Log>::const_iterator;

    LogStore() {
        logs = {
            {"INFO", "Startup"},
            {"ERROR", "Disk failure"},
            {"INFO", "Running"},
            {"ERROR", "Crash"}
        };
    }

    iterator begin() { return logs.begin(); }
    iterator end()   { return logs.end(); }

    const_iterator begin() const { return logs.begin(); }
    const_iterator end() const   { return logs.end(); }
};

int main() {
    LogStore store;

    auto errors =
        store
        | boost::adaptors::filtered([](const Log& l) {
            return l.level == "ERROR";
        })
        | boost::adaptors::transformed([](const Log& l) {
            return l.message;
        });

    for (const auto& msg : errors) {
        std::cout << msg << std::endl;
    }
}
