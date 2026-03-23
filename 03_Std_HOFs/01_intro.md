# Components of standard HOFs
The implementation of standard higher order functions in C++ is based on the following components:
## Callable entities
These are the function types such as function pointers, functors, lambdas, etc., that are passed around (see [03_HOF.md](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/03_HOF.md)). They are used to provide specialized functional logic, such as:
* Predicate: A function that takes one or two arguments and returns a bool.
* Generator: A function that takes no arguments and returns values to fill a range.
* Operator: A function that produces specific results based on the inputs provided.
* Execution policy: A function that makes use of multi-core processors. (Since C++17)
## Execution Contexts
These are the containers or ranges on which various operations are performed. They are:
* Containers → ```std::vector```, ```std::list```, ```std::map```, etc.
* Iterators → Mechanism to access elements of a container/range, e.g., ```std::iterator```, etc.
* Ranges → More expressive composition of HOFs that eliminate the need to manually pass begin/end iterators. (Since C++20)
* Seed values → Initial values needed as starting point for certain callables, e.g., for reductions (```std::accumulate``` needs a starting value).
## Algorithms/wrappers
Algorithms are standard higher-order functions that accept callables and apply them on containers/ranges. These are the wrappers that define the logic for how an operation should be executed. They apply the callable on the data. They can be categorized in the following manner:
| Category |	Typical Logic Type|	Purpose	|Example Algorithm| Signature Expectation |
|--------- | ------------------ | ------- | --------------- | --------------------- |
| Non-Modifying |	Predicate(Unary/Binary) |	Searching & Validation |	```find_if```, ```all_of``` | ```(T) -> bool``` |
| Modifying |	Unary/Binary Op |	Transformation |	```transform```, ```generate``` | ```(T) -> U``` |
| Sorting |	Comparator (Binary predicate) |	Custom Ordering |	```sort```, ```stable_sort``` | ```(Acc, T) -> Acc``` |
| Numeric |	Binary Op |	Accumulation/Reduction |	```accumulate```, ```reduce``` | ```(T, T) -> bool``` |
| Parallel |	Execution Policy |	Threading/Concurrency | ```seq```, ```par```, ```par_unseq``` |	```() -> T``` |

To illustrate diagrammatically:

[![Copilot-20260214-125707.png](https://i.postimg.cc/RVWj6PPY/Copilot-20260214-125707.png)](https://postimg.cc/D8KCRc3Q)
# Workflow
A standard HOF typically follows this structure:
```
algorithm(execution_policy?, range_begin, range_end, seed?, callable);
```
For example:
```
int sum = std::accumulate(
    vec.begin(),
    vec.end(),
    0,
    [](int a, int b) { return a + b; }
);
```
Here:
* Execution context → ```vec.begin()```, ```vec.end()```
* Seed value → ```0```
* Callable (operator) → ```[](int a, int b) { return a + b; }```
* Algorithm (HOF) → ```std::accumulate```

Standard HOFs in C++ are built on:
* Behavior (Callable entities): Defines what operation should be performed.
* Data (Containers, iterators, ranges): Defines where the operation is applied.
* Control (Execution policy & seed values): Defines how and from what starting point the operation is executed (see [Control parameters](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/03_Std_HOFs/02_note.md#14-control-parameters)).

Implementing a standard higher function involves the following steps:
### Step 1: Define a callable
The first step is to define a callable that contains what the desired logic. It should define what operation should be performed. This essentially involves creating a lambda, functor, or a function pointer.
### Step 2: Choose an algorithm
The next step is to select an algorithm from the standard library that suits our purpose. This step defines how the callable will be applied.
### Step 3: Provide a data/range
The execution context, i.e., the data needs to be supplied, on which the algorithm would operate. This step involves supplying a container or iterator range.

[![Copilot-20260214-144124.png](https://i.postimg.cc/xTP95pGX/Copilot-20260214-144124.png)](https://postimg.cc/VddyLRVw)

# Modern Improvements
With the introduction of Ranges and Views in C++20, working with algorithms became more expressive and easier to read. 
A range is a representation of a asequence of elements with a begin iterator and an end sentinel. The ranges library, defined in the ```<ranges>``` header, is an extension and generalization of the standard algorithms and iterator libraries, which makes it more powerful, composable, and less error-prone.

The ranges library includes:
* range algorithms (function objects that perform immediate computation and execution of ranges ([eager evaluation](https://share.google/aimode/UnfevoWfIYgsIVtjH)))
* range adaptors (function objects applied to views that perform computations at a later time when needed ([lazy evaluation](https://nixiz.github.io/yazilim-notlari/2023/09/10/lazy-evaluation-en))).

A ```std::ranges``` algorithm assumes begin to end by default when passing in a container. There are also variants available for more granular control over a container.

In C++20, ```std::ranges``` are built on concepts:

```std::ranges::range<T>```

where ```T``` must have ```begin(t)``` and ```end(t)``` that return valid iterators.

And they are refined by the iterator categories:
* ```std::ranges::input_range``` → forward‑only, single‑pass.
* ```std::ranges::forward_range``` → bidi‑capable, multiple passes.
* ```std::ranges::bidirectional_range``` → can traverse backwards
* ```std::ranges::random_access_range``` → O(1) random element access
* ```std::ranges::contiguous_range``` → elements are laid out contiguously in memory (enables pointer arithmetic)
* ```std::ranges::sized_range``` → size() is available in O(1)
* ```std::ranges::common_range``` → ```begin(t)``` and ```end(t)``` return the same type (no separate sentinel).

Each concept refines the previous one, progressively requiring more capabilities:
```
range
├── input_range         (single-pass)
│   └── forward_range   (multi-pass)
│       └── bidirectional_range
│           └── random_access_range
│               └── contiguous_range
├── sized_range     (O(1) size())   [orthogonal]
├── common_range    (begin/end same type) [orthogonal]
├── borrowed_range  (iterators outlive range) [orthogonal]
└── view            (O(1) move/copy/destroy) [orthogonal]
```
These concepts are checked statically at compile time. 

It can be said that:
* All containers and container adaptors  are ranges
* Non-owning or borrowed containers like ```std::string_view```, ```std::span```, etc., are borrowed ranges. A range is "borrowed" if iterating over it does not depend on the lifetime of the range object itself.

The namespace alias ```std::views``` is provided as a shorthand for ```std::ranges::views```. A view is a lightweight range that works on a container without making internal data copies, unlike a range. A view provides a "window" into an existing range via reference semantics, i.e., it is:
* memory efficient - it does not copy of the elements of the container it works on
* mutable - modifications to the underlying container are reflected in the view and vice-versa.

A view must satisfy:
```
template<typename V>
concept view = std::ranges::range<V>
            && std::movable<V>
            && std::ranges::enable_view<V>; // opt-in marker
```
And additionally, all these operations must be O(1):
* Move construction
* Move assignment
* Destruction
* Copy construction (if supported)
* Copy assignment (if supported)
This O(1) constraint is the essential rule: a view must **never** copy the underlying data. It only holds a reference/pointer/iterator to it.


The view adapters are defined under ```std::views```, such as ```std::views::filter``` and ```std::views::transform``` for instance, and don’t immediately process data. Instead, they create a view — a lightweight object that does not own or copy data from the container it works on, but just defines how elements should be seen. This allows for lazy evaluation, where the operations are defined immediately but logic is only executed when we actually iterate over the final result. For more on lazy evaluation in C++, read "Functional Programming in C++" by Ivan Cukic or "Learning C++ Functional Programming" by Wisnu Anggoro.

Custom views can also be created by inheriting from ```std::ranges::view_interface```.
```
template<std::ranges::view V>
class my_view : public std::ranges::view_interface<my_view<V>> {
    V base_;
public:
    my_view() = default;
    my_view(V base) : base_(std::move(base)) {}

    auto begin() { return std::ranges::begin(base_); }
    auto end()   { return std::ranges::end(base_); }
};
```

Instead of calling algorithms separately and passing iterator pairs each time, we can now build operations in a pipeline style, similar to how data flows through stages. This is called as a "pipeable" workflow.

The general pattern looks like this:
```
Data | View_Adapter | Action
```
* Data → The original source of elements (for example, a std::vector of users).
* View Adapter → A transformation or filtering step that describes how the data should be processed.
* Action → The final step where the result is actually consumed (for example, printing or storing values).

Consider a traditional STL example:
```
std::vector<User> active_users;
std::copy_if(users.begin(), users.end(), 
             std::back_inserter(active_users), 
             is_active);

std::vector<std::string> names;
std::transform(active_users.begin(), active_users.end(),
               std::back_inserter(names),
               get_name);
```
This approach:
* Requires intermediate containers
* Separates steps instead of expressing a continuous flow
* Can cause memory and performance overhead

With ranges, no intermediate containers are needed. One of the big design wins of ranges is that composition is natural and efficient. The above example would then look like:
```
auto result = users 
| std::views::filter(is_active) 
| std::views::transform(get_name);
```
Here:
* Only the pipeline structure is built.
* At the time of using ```result``` (e.g., for printing each element of ```result```), each view’s iterator advances the base and applies the filter/transform on‑the‑fly.

This reads almost like an English sentence:
Take users → keep only active ones → extract their names.

This contract ensures that:
* Many views can be chained without performance collapse.
* Views can be passed cheaply (by value) into algorithms or functions

Because views are non‑owning, it must be ensured that the underlying range outlives the view.
```
auto make_view() {
    std::vector<int> local = {1, 2, 3};
    auto v = local | std::views::filter([](int x) { return x > 1; });
    return v;   // DANGER: local is destroyed when function returns
}

for (int x : make_view()) { ... } // Undefined behavior!
```
The code above demonstrates a "dangling view." When we try to use the result, the vector ```v``` is already gone, leading to undefined behavior.

To avoid this situation, either:
* Keep the base alive:
```
std::vector<int> data = {1, 2, 3};
auto pipeline = data | std::views::filter(...);
for (int x : pipeline) { }   // OK
```
* Or store the result:
```
std::vector<int> stored;
std::ranges::copy(pipeline, std::back_inserter(stored));
```

* Or materialize with ```std::ranges::to``` (Since C++23 and GCC v16)
```
std::vector<int> vec;
// Works from C++23 and GCC v16 onwards
auto result_vec = std::ranges::to<std::vector<int>>(vec);
```
Let's take an example:
```
std::vector<int> v = {-1, 2, -3, 4, 5, 6};
auto pipeline = v
    | std::views::filter([](int x) { return x > 0; }) // 2,4,5,6
    | std::views::transform([](int x) { return x * x; }) // 4,16,25,36
    | std::views::take(3);

for (int x : pipeline) { /* 4, 16, 25 */ }
```
The type of `pipeline` is something like:
```
take_view
  → transform_view
     → filter_view
        → ref_view
            → vector
```
This is a nested type — a compile-time description of the computation. No elements are touched. 

The flow begins with ```*pipeline.begin()```, following which:
1. ```take_view::iterator::operator*()``` is called
2. Which calls ```transform_view::iterator::operator*()```
3. It calls ```filter_view::iterator::operator*()```
4. Which advances until predicate is satisfied
5. Which calls ```ref_view::iterator::operator*()```
6. Which reads from the vector
7. The value flows back up through transform

Each element of the underlying container is processed on demand, one at a time, with the full pipeline fused together. The compiler typically inlines everything into a tight loop.

From the address‑space perspective:
* ```v``` owns a contiguous buffer: ```[-1, 2, -3, 4, 5, 6]```.
* ```filter_view``` stores a pointer‑like view into this buffer (```m_base``` ≈ ```v.data()``` and ```v.size()```).
* ```transform_view``` stores only:
    * Another pointer‑like base iterator.
    * A small function‑object ```fn``` (likely just a ```size_t```‑sized lambda).
* ```take_view``` stores:
    * A pointer‑like base iterator.
    * Two ```size_t```s: ```m_count```, ```m_max```.

No extra storage is allocated for the intermediate sequence ```[2,4,5,6]``` or ```[4,16,25,36]```.

Instead, the machine code for
```
for (int x : pipeline) { ... }
```
can be optimized into a loop that conceptually looks like:
```
int count = 0;
for (auto it = v.begin(); it != v.end() && count < 3; ++it) {
    if (*it <= 0) continue;
    int x = *it * *it;
    // ... use x
    ++count;
}
```
The compiler inlines ```filter_view::iterator::operator++()``` and ```transform_view::iterator::operator*()``` across the adaptor boundaries, so the indirection cost is very low.

Even though the model is deep,
```
take_view
  → transform_view
     → filter_view
        → std::vector
```
The compiler can:
* Inline ```operator*()``` and ```operator++()``` through the layers.
* Recognize that ```m_pred``` and ```m_fn``` are simple functions and keep them inline.
* Collapse the whole chain into a tight loop with no extra allocation per ```|```.

### When to use views vs containers
Use views when:
* You want zero‑copy pipelines over large or read‑only data.
* The underlying data is stable and long‑lived.
* The transformations are cheap (simple arithmetic, light lambdas).
* You iterate once or a few times.

Use containers when:
* You need persistent storage of the result.
* You iterate many times over the same data.
* The transform/filter is expensive (e.g., calls a heavy function or I/O).
* The data source is ephemeral or you cannot guarantee lifetime.

This keeps the pipeline design lazy and composable, while giving you owned storage where needed.


Sources:
* https://www.youtube.com/watch?v=HYENjkZvsrM
* https://www.youtube.com/watch?v=Rbl3h0RJuuY
* https://www.youtube.com/watch?v=Q434UHWRzI0
* https://www.youtube.com/watch?v=5iXUCcFP6H4
