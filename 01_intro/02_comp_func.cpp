// 02_comp_func.cpp
/* NOTE: Avaiable since C++23 */ 
#include <iostream> 
#include <expected>
#include <string>
#include <charconv> // For std::from_chars
 
// Define a type alias for an expected integer value or an error string 
using ExpectedInt = std::expected<int, std::string>;

// Function 1: Parses a string to an integer, might fail 
ExpectedInt parseInt(std::string_view sv)
{ 
  int r{};
  auto [ptr, ec] { std::from_chars(sv.data(), sv.data() + sv.size(), r) }; 
  if (ec == std::errc{}) { 
    return r; // Success, return the value 
  } else { 
    return std::unexpected{"Failed to parse integer" + std::string(sv)}; // Error, return unexpected 
  }
}
 
// Function 2: Doubles an integer, might fail if value too large (simple example)
ExpectedInt doubleValue(int n)
{ 
  if (n > 100000) { 
    return std::unexpected{"Value too large to double"}; // Error 
  }
  return n * 2; // Success 
}
 
// Error Handler: Called if any prior operation fails
std::expected<int, std::string> handleError(const std::string& error_msg) 
{
  std::cerr << "Caught an error in or_else: " << error_msg << std::endl;     // You can attempt recovery or re-throw/return another error 
  return std::unexpected{"Fatal error after handling"}; 
}
 
int main() 
{
  std::cout << "--- Successful Chain ---" << std::endl;     // Start with an input string that works 
  auto result_success = parseInt("123")
                        .and_then(doubleValue)    // If success, call doubleValue (returns ExpectedInt) 
                        .or_else(handleError);   // If error at any point, call handleError
 
  if (result_success.has_value()) {
    std::cout << "Final result: " << result_success.value() << std::endl; // Output: 247 
  }
 
  std::cout << "\n--- Failure Chain ---" << std::endl;     // Start with an input string that fails to parse
  auto result_failure = parseInt("foo") 
                        .and_then(doubleValue)   // This step is skipped because parseInt fails 
                        .or_else(handleError);   // This step is executed with the error from parseInt
 
  if (result_failure.has_value()) { 
    std::cout << "Success!" << result_failure.error() << std::endl; 
  } else {
    std::cout << "Error propagated to the end: " << result_failure.error() << std::endl; 
  }
 
  return 0;
}
