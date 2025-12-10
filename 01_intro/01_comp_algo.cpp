// 01_comp_algo.cpp
/* NOTE: Available since C++20 */
#include <iostream>
#include <vector> 
#include <iostream>
#include <vector>
#include <ranges> 
#include <iterator>
#include <algorithm>
 
namespace stdr = std::ranges; 
namespace stdv = std::views;
 
int biggest_odd_magnitude_fp(auto&& rng)
{
  return stdr::max( // 3. Maximum
    rng           
    | stdv::transform([](int x) {return std::abs(x); }) // 1. Compute the magnitudes 
    | stdv::filter([](int x) {return x % 2 == 1; }) // 2. Keep the odd values   
    );
}
 
int biggest_odd_magnitude(auto&& rng) 
{
  int candidate = -1;
  for(int x: rng) {
    int magnitude = std::abs(x);
    if(magnitude % 2 == 1) { 
      candidate = (magnitude > candidate) ? magnitude :  candidate; 
    }
  }
  return candidate; 
}
 
int main()
{
  std::vector<int> vec = { 3, 0, 2, -1, 5, -7, 8 };
  int max = biggest_odd_magnitude(vec);
  int max_fp = biggest_odd_magnitude_fp(vec);

  std::cout << "Largest odd magnitude = " << max << std::endl;
  std::cout << "Largest odd magnitude using FP = " << max_fp << std::endl; 

  if(max == max_fp) { 
    std::cout << "Both results are same." << std::endl;
  } else {
    std::cout << "The results are different." << std::endl;
  }
 
  return 0; 
}
