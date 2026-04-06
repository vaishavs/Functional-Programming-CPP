# Boost.Range
Boost.Range is a C++ library that applies range-based algorithms, adaptors, and utilities on containers directly without manually handling iterators such as ```begin()``` and ```end()```.

In standard C++, in order to work on a container, it was not enough to just pass the container to the algorithm; the beginning and the end also had to be passed. While this is highly flexible, constantly writing ```.begin()``` and ```.end()``` is verbose, visually cluttered, and prone to errors (like accidentally passing ```vec1.begin()``` and ```vec2.end()```). In Boost.Range, a "Range" is conceptually very simple: it is any object that provides a beginning iterator and an ending iterator. Instead of treating the beginning and end of a sequence as two separate things that programmers must manually keep synchronized, Boost.Range treats the entire sequence along with its iterators as a single, unified concept. This means almost all standard C++ containers are naturally treated as Ranges.
```
std::vector<int> vec = {1, 2, 3, 4, 5};

// STL: Requires begin()/end() calls
std::for_each(vec.begin(), vec.end(), [](int x) { std::cout << x << " "; });  // 1 2 3 4 5

// Boost.Range: Direct range passing
boost::for_each(vec, [](int x) { std::cout << x << " "; });  // Same output, shorter code
// Works identically on arrays: 
int arr[] = {1,2,3};
boost::for_each(arr, ...); // (no .data() needed)
```

Boost.Range was so successful and well-loved that the C++ standards committee later adopted these concepts into C++20 as ```std::ranges``` (see [01_intro.md](https://github.com/vaishavs/Functional-Programming-/blob/main/03_Std_HOFs/01_intro.md)). Using a modern compiler (C++20 or newer), almost the exact same code can be written using the standard library (```#include <ranges>```) instead of Boost:
```
// C++20 Standard Library Equivalent
auto std_view = numbers | std::views::filter(is_even) 
                        | std::views::transform(square) 
                        | std::views::reverse;
```


## Range concept hierarchy
Boost.Range relies heavily on concepts. A concept, in the sense used by Boost.Range, is a named set of requirements imposed on a type at compile-time. These requirements are of two types:
* syntactical requirements: what expressions must be valid
* semantical requirements: what those expressions must mean.

A type is said to model a concept if and only if it satisfies all of the concept's syntactic and semantic requirements.

It defines a hierarchy of range concepts mirroring iterator categories:
| Range Concept    | Description |
| ------- | :---: |
| SinglePassRange   | Can be traversed once (like input iterators)    |
| ForwardRange   | Multi-pass, forward traversal    |
| BidirectionalRange | Forward + backward traversal |
| RandomAccessRange | $O(1)$ element access |
	
Boost.Range defines a strict concept hierarchy, each building on the previous one:
``` 
SinglePassRange 
	└── ForwardRange 
			└── BidirectionalRange 
					└── RandomAccessRange
```
Boost.Range provides compile-time concept-checking classes in ```<boost/range/concepts.hpp>```. If the concept fails, it produces undefined behavior at best and silently wrong results at worst. The ```BOOST_CONCEPT_ASSERT``` macro is the primary tool for producing early, meaningful errors rather than late, cryptic ones. The macro requires double parentheses to work correctly, such as `BOOST_CONCEPT_ASSERT((ConceptName<Type>))`. When a concept assertion is added at the beginning of a function template, the compiler checks the concept requirements immediately when it instantiates the function. If the requirements are not satisfied, an error pointing at the assertion is produced, with a clear message about which concept was violated. Without the assertion, the compiler would proceed into the function body, fail on some specific expression deep inside, and report an error that refers to implementation details rather than the interface contract.

## Levels of abstraction in Boost.Range
Even though the documentation does not advertise Boost.Range as a layered architecture, its design naturally forms layers of components building on each other, with increasing levels of abstraction from raw iterators up to generic range‑based algorithms and adaptors, in a conceptual sense.

```
+-----------------------+
| Utility classes       |
+-----------------------+
| Range Algorithms      |
+-----------------------+
| Range Adaptors        |
+-----------------------+
| Range Concepts        |
+-----------------------+
| Iterator Abstraction  |
+-----------------------+
| Underlying Containers |
+-----------------------+
```
#### Layer 1: Iterator abstraction
At the lowest level, Boost.Range relies on the standard iterator model from the C++ Standard Library. It encapsulates iterates into a single conceptual unit called a “range”. In other words, a range is essentially a clean lightweight wrapper around `begin()` and `end()`. 

Be it a standard container, a raw array, or even a custom data structure, as long as it provides iterators, it is treated as a range. Internally, the range still uses iterators, but that detail is hidden from the user. The library takes in a range directly, and automatically uses its iterators behind the scenes.

For example:
```
struct MyContainer {
    MyIterator begin() const;
    MyIterator end() const;
};

// Now works automatically:
MyContainer c;
boost::sort(c);
auto r = c | boost::adaptors::reversed;
```

#### Layer 2: Core Concepts and Traits
This layer consists of traits and iterators. It provides the fundamental definitions and metadata required to treat any sequence as a range. This layer defines what a range is (concepts) and how to extract information about its types (traits), allowing the library to "inspect" a range at compile-time.
* Traits: ```boost::range_iterator<Rng>::type```, ```range_value<Rng>::type / range_reference::type<Rng>```, ```range_category<Rng>::type```, etc., to deduce types at compile-time. These metafunctions serve an analogous role to `std::iterator_traits` in the STL, but operate at the level of the range rather than the iterator.
* Requirements: ```size()```, ```begin(rng)/end(rng)```, ```rbegin()/rend()``` to validate ```[first, last)```.

#### Layer 3: The Free-Standing Functions Layer (universal lazy adaptors)
This layer is universal dispatch layer over concepts: ```boost::begin(rng)```, ```boost::end(rng)```, ```boost::size(rng)``` etc. This layer acts as a bridge between high-level algorithms (like ```boost::find``` or ```boost::for_each```) and low-level data structures (like standard containers, raw arrays, or ```std::pair``` of iterators) to create a consistent, uniform interface for diverse container types. It consists of a set of functions that provide uniform access to any range, working around the inconsistencies of different types. There is a layer of indirection in these functions that hides the differences in the underlying containers. This results in code being written just once, in terms of ranges, and it works across all different sequence types. In other words, the same algorithm that sorts a vector will sort an array or any other container. The same adaptor that filters a list will filter a custom container. The abstraction hides the differences between these types behind a common interface.

The dispatch is handled via template metaprogramming, meaning there is zero runtime overhead. It selects the best-performing implementation based on iterator capabilities. Without this layer, you would need different code to sort a vector vs. sorting a raw array. The dispatch layer allows this:
```
// Works for vector, array, list, etc.
boost::sort(my_range); 
```
Adaptors are **lazy** range transformers. They do *not* store transformed data.

The dispatch layer calls ```boost::begin(my_range)``` and ```boost::end(my_range)```, which are internally overloaded to handle different types, dispatching to the correct implementation.
* ```boost::size(rng)```: For forward+ ranges; $O(1)$ where possible (e.g., vec.size()), else distance.
* ```boost::empty(rng)```: `size()`==0 or `begin()`==`end()`.
* ```boost::distance(rng)```: Forward traversal count.
* ```boost::advance(rng, n)``` / ```boost::prior(rng)```: Iterator movement, category-aware.

For example:
```
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/sliced.hpp>

using namespace boost::adaptors;

std::vector<int> v = {1, 2, 3, 4, 5, 6};

// Compose lazy adaptors with | operator
auto result = v
    | filtered([](int x) { return x % 2 == 0; })  // lazy filter
    | transformed([](int x) { return x * x; })     // lazy transform
    | reversed;                                     // lazy reverse

// Iterates: 36, 16, 4
for (int x : result) std::cout << x << " ";
```
#### Layer 4: Range algorithms
This layer consists of around 70+ algorithm wrappers to STL equivalents using Layer 2 functions, defined in ```<boost/range/algorithm.hpp>```, e.g., ```boost::for_each(rng, f)```, ```boost::copy(rng, out_it)```, ```boost::accumulate(rng, state)```, etc. It provides a suite of generic functions that operate directly and **eagerly** on range objects rather than requiring pairs of iterators. The library organizes algorithms into several headers based on their functionality:
* Standard Library Counterparts: Includes versions of nearly all STL algorithms, such as ```boost::find```, ```boost::copy```, ```boost::sort```, and ```boost::for_each```. These are found in ```<boost/range/algorithm.hpp>```.
* Numerical Algorithms: Range-based versions of ```<numeric>``` functions like ```boost::accumulate``` and ```boost::inner_product```, located in ```<boost/range/numeric.hpp>```.
* Extended Algorithms: Additional utilities not found in the standard library, such as ```boost::push_back``` (adds a range to a container) and ```boost::remove_erase``` (removes elements and shrinks the container in one step). These are typically found in ```<boost/range/algorithm_ext.hpp>```.

For example:
```
#include <boost/range/algorithm.hpp>
#include <boost/range/numeric.hpp>

std::vector<int> v = {3, 1, 4, 1, 5};

boost::sort(v);                          // vs std::sort(v.begin(), v.end())
auto it = boost::find(v, 4);            // vs std::find(v.begin(), v.end(), 4)
boost::for_each(v, [](int x){ ... });
int sum = boost::accumulate(v, 0);
```
#### Layer 5: Helper classes
 These classes provide the infrastructure to turn raw pointers, iterator pairs, or custom data structures into first-class ranges. They act as the primary "glue" when you need to store, return, or pass a range that isn't a standard container. Boost.Range contains several utility classes, some of which are:
* ```boost::iterator_range<It>```: Pairs iterators with range interface (size via distance).
  This is the most general helper class in the library. It is templated on an iterator type and simply wraps a begin and end iterator into a single object that models a Forward Range. It is used to work on a portion of a container. For example:
    ```
    #include <boost/range/iterator_range.hpp>
	#include <vector>

	std::vector<int> val = {0, 1, 2, 3, 4, 5};

	// Create a range from index 1 to 4
	auto sub_range = boost::make_iterator_range(val.begin() + 1, val.begin() + 4);

	// Now sub_range behaves like a container
	bool empty = sub_range.empty();
	auto size = sub_range.size(); // returns 3
    ```
* ```boost::sub_range<Rng>```: Typed subrange wrapper (preserves base traits better).
  This class inherits from ```iterator_range``` but is templated on a Range type (like ```std::vector<int>```) rather than an iterator type. While ```iterator_range``` is a generic wrapper, ```sub_range``` is "type-safe" in regards to the container it came from. It is better at handling const. If a ```sub_range``` is marked const, it automatically uses ```const_iterator``` internally, preventing accidental modification of the underlying container.
* ```boost::any_range<T,Category>```: Type-erased ranges (runtime polymorphism).
  This is a type-erasing helper class that allows you to store or return ranges of a specific value type without exposing the complex underlying template types of adaptors. It hides (erases) those incredibly long complex types which get generated by certain adaptors like ```filter``` or ```transform```.
  ```
  // This function can return ANY range of strings, 
  // regardless of how it was filtered or transformed.
  boost::any_range<std::string, boost::forward_traversal_tag> get_names();
  ```
* Generators: ```irange(n,m)```, ```istream_range<It>(stream)```
  These helpers don't wrap existing containers; they create "virtual" ranges on the fly.
  * ```counting_range``` / ```irange```: Represents a sequence of numbers ```(0, 1, 2, 3...)```. It doesn't actually store the numbers in memory; it generates the next one only when the iterator is incremented. This is highly memory-efficient.
  * ```istream_range```: Wraps a ```std::istream_iterator```. This treats a file or a console input as a standard Boost Range.

### How Layers Interact
Consider an example:
```
boost::sort(
    v 
    | filtered([](int x){ return x > 2; })
);
```
The execution flow happens in the following steps:
1. Container -> ```v``` holds data
2. Iterator Layer -> ```begin(v)```, ```end(v)``` are accessed
3. Concept Layer -> Ensures ```v``` is a valid range by validating ```begin(v)``` and ```end(v)```
4. Adaptor Layer -> ```filtered()``` creates a lazy view
5. Algorithm Layer -> ```boost::sort()``` operates on the view

Think of this as a mental model:
```Data → View Transformations → Algorithm```

In this example:
```v → filter → transform → sort → result```

The right side of operator `|` defines an "adaptor holder" type that captures the adaptor's parameters (the predicate, the function, the stride, etc.) but has not yet been bound to a specific range. In the above example, in `v | filtered(pred)`, the `operator |` overload for the range type and the filtered holder is invoked, producing a new range type that wraps `v` with a filter iterator, so it can be piped through another adaptor.

Each layer has independent headers. Including one does not drag in the others. For example, if only `<boost/range/algorithm.hpp>` is included, algorithms on raw containers are obtained — no adaptor overhead. If `<boost/range/adaptors.hpp>` is added, then the lazy pipeline machinery is obtained.

```
// Only Layer 2 — zero adaptor or algorithm overhead
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

// Only Layer 5 — algorithms on raw containers, no adaptors
#include <boost/range/algorithm.hpp>

// Only Layer 4 — adaptors, but no algorithms needed
#include <boost/range/adaptors.hpp>

// Everything
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>

```

Also, algorithms no longer depend directly on specific container types or iterator details. They operate on the abstract concept of a range, leading to more generic and reusable code.

The layering ensures an that an unsupported operation on a range is not performed. Concept mismatches are caught by the compiler, not at runtime. The error appears at the algorithm call site with a clear message about traversal category, not deep inside an algorithm implementation. Bugs in algorithms can often be fixed without touching adaptors.

New range types can be added at Layer 2, and they work with every adaptor and algorithm in Layers 4–5.
```
Register at Layer 2
        │
        ▼
Layer 3: iterator_range works 
Layer 4: filtered, transformed... 
Layer 5: sort, find, accumulate... 
```

A new adaptor can be introduced without modifying existing algorithms, or the same algorithm can be applied to completely different data structures as long as they can be expressed as ranges. Because the responsibilities of each layer are separated, changes in one layer do not heavily impact others.

Every adaptor returns an `iterator_range` (Layer 3), so it satisfies the same concepts (Layer 1) and can be passed back into algorithms (Layer 5) or wrapped in another adaptor (Layer 4). The adaptor output can be fed back into any layer. For example:

```
std::vector<int> v = {1,2,3,4,5,6,7,8,9,10};

// Adaptor output fed into another adaptor (Layer 4 → Layer 4)
auto step1 = v    | boost::adaptors::filtered([](int n){ return n%2==0; });
auto step2 = step1| boost::adaptors::transformed([](int n){ return n*n; });
auto step3 = step2| boost::adaptors::reversed;

// Adaptor output fed into algorithm (Layer 4 → Layer 5)
long total = boost::accumulate(step3, 0L);

// Adaptor output used as a range utility (Layer 4 → Layer 3)
auto sub = boost::make_iterator_range(
    boost::begin(step3),
    boost::end(step3)
);

// Adaptor output wrapped in any_range for type erasure (Layer 4 → Layer 3)
using View = boost::any_range<int, boost::forward_traversal_tag, int, std::ptrdiff_t>;
View erased = step3;
```
Nothing special is needed at each step — it just works because the layer contract is consistent.

## Common pitfalls   
While Boost.Range is an exceptionally powerful tool for creating clean, functional C++ code, its reliance on lazy evaluation and template metaprogramming introduces specific traps that can lead to "heisenbugs" or significant performance degradation if one is not careful.  
### 1. The Dangling Range (Lifetime Issues)  
This is the most frequent and dangerous pitfall. Because range adaptors are **lazy views** and not containers, they do not own the data they point to; they only hold a reference or an iterator to the original container.  

If a temporary container is created, adapted, and then that adaptor is used after the temporary is destroyed, there is a dangling reference.  
```  
// DANGER: The temporary vector returned by get_data() is destroyed   
// at the end of this line, but 'view' still points to its memory.  
auto view = get_data() | boost::adaptors::filtered(is_even);   
  
for (int x : view) { // UNDEFINED BEHAVIOR: Accessing deleted memory  
    std::cout << x;  
}  
```  
**The Fix:** It must always be ensured that the underlying container outlives any adaptors or `iterator_range` objects derived from it. If it is needed to return a transformed range from a function, it is recommended to copy the results into a new `std::vector` using `boost::copy`.  
  
### 2. Double Processing in Pipelines  
Because adaptors are lazy, the transformation logic is executed **every time** you iterate over the range. If the transformation function is computationally expensive (like a complex math calculation or a database lookup), calling it multiple times can tank performance.  
```  
auto heavy_view = data | boost::adaptors::transformed(expensive_calculation);  
  
// expensive_calculation runs for every element here...  
boost::copy(heavy_view, std::back_inserter(vec1));  
  
// ...and it runs all over again for every element here!  
boost::copy(heavy_view, std::back_inserter(vec2));  
```  
**The Fix:** To iterate over a transformed range more than once, it is recommended to materialize the range into a container first.  
  
### 3. Modifying the Underlying Container  
Range iterators are just wrappers around the container's iterators. If an action is performed that invalidates the container's iterators (like a `push_back` on a `std::vector `which might trigger a reallocation), the range adaptors become instantly invalid.  
```  
std::vector<int> vec = {1, 2, 3};  
auto view = vec | boost::adaptors::filtered(is_odd);  
  
vec.push_back(4); // Potential reallocation!  
// 'view' is now likely pointing to garbage memory.  
```  
### 4. The "Length-Change" Illusion  
Adaptors like filtered change the **perceived** size of the range, but they do not change the complexity of certain operations. For instance, calling `boost::size()` on a filtered range is an $O(N)$ operation, not $O(1)$, because the library has to traverse the entire underlying range to count how many elements pass the filter.  
**The Pitfall:** Writing code that assumes `boost::size(my_range)` is cheap inside a loop. This can accidentally turn an $O(N)$ algorithm into an $O(N^2)$ disaster.  
  
### 5. Type Erasure Overhead with `any_range`  
Sometimes it is necessary to hide the complex template type of a range (e.g., when returning a range from a virtual function). Boost provides `any_range` for this purpose. However, `any_range` uses **Type Erasure** (similar to `std::function`), which involves virtual function calls for every increment and dereference.  
**The Pitfall:** Using `any_range` in high-frequency, performance-critical inner loops. The overhead of virtual calls can be 10x slower than a direct template-based range.  
  
### Summary Table: Pitfall vs. Solution  

| Pitfall | Consequence | Prevention |
| --------------------- | -------------------- | -------------------------------------------- |
| Dangling Reference | Crash / Random data | Don't adapt temporaries. |
| Re-evaluating Logic | High CPU usage | Materialize into a vector if used twice. |
| Iterator Invalidation | Undefined Behavior | Don't resize the source while using a view. |
| Hidden Complexity | $O(N^2)$ performance | Avoid size() on filtered ranges. |
| any_range Usage | Slow execution | Use only at API boundaries, not tight loops. |
  
  
Source: 
[Boost.Range Documentation](http://boost.cowic.de/rc/pdf/boost_range.pdf)
[Boost.Range](https://theboostcpplibraries.com/boost.range)
