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
A range is a representation of a asequence of elements with a begin iterator and an end sentinel. The standard ranges library, defined in the ```<ranges>``` header, is an extension and generalization of the standard algorithms and iterator libraries, which makes it more powerful, flexible, composable, and less error-prone.

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
* ```std::ranges::random_access_range``` → $O(1)$ random element access
* ```std::ranges::contiguous_range``` → elements are laid out contiguously in memory (enables pointer arithmetic)
* ```std::ranges::sized_range``` → `size()` is available in $O(1)$
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

The namespace alias ```std::views``` is provided as a shorthand for ```std::ranges::views```. A view is a lightweight range that works on a container without making internal data copies, unlike a range. A view provides a "window" into an existing range via reference semantics, i.e., it is memory efficient and mutable. In other words, it does not copy the elements of the container, and modifications to the underlying container are reflected in the view and vice-versa.

The view adapters are defined under ```std::views```, such as ```std::views::filter```, ```std::views::transform```, etc., and don’t immediately process data. Instead, they create a view — a lightweight object that does not own or copy data from the container it works on, but just defines how elements should be seen. This allows for lazy evaluation, where the operations are defined immediately but logic is only executed when we actually iterate over the final result. For more on lazy evaluation in C++, read "Functional Programming in C++" by Ivan Cukic or "Learning C++ Functional Programming" by Wisnu Anggoro.

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

A view must satisfy:
```
template<typename V>
concept view = std::ranges::range<V>
            && std::movable<V>
            && std::ranges::enable_view<V>; // opt-in marker
```
And additionally, all these operations must be $O(1)$:
* Move construction
* Move assignment
* Destruction
* Copy construction (if supported)
* Copy assignment (if supported)
This $O(1)$ constraint is the essential rule: a view must **never** copy the underlying data. It only holds a reference/pointer/iterator to it.

Instead of calling algorithms separately and passing iterator pairs each time, operations can now be built in a pipeline style, similar to how data flows through stages. This is called as a "pipeable" workflow.

The general pattern looks like this:
```
Data | View_Adapter | Action
```
* Data → The original source of elements (for example, a `std::vector` of users).
* View Adapter → Describes how the data should be processed.
* Action → The final step where the result is actually consumed (for example, printing or storing values).

The pipe operator ```|``` is used to chain range adaptors. It was chosen deliberately to evoke the Unix pipeline metaphor, where data flows through a series of transformations, each receiving the output of the previous one. The semantic contract of the pipe operator is that the right-hand side is always applied to the left-hand side, producing a new range without modifying the original. This is done using *proxy iterators*.

The first adaptor in the pipeline returns a range structure whose begin iterator will be a smart proxy iterator that points to the first element in the source collection that satisfies the given predicate. And the end iterator will be a proxy for the original collection’s end iterator. The only thing the proxy iterator needs to do differently than the iterator from the original collection is to point only at the elements that satisfy the given predicate.

In a nutshell, every time the proxy iterator is incremented, it needs to find the next element in the original collection that satisfies the predicate. With the proxy iterator, a new temporary collection need not be created. Only a new 'view' of the existing data is created. This new range is a lazy view — it wraps the original rather than copying it. This view, instead of showing original elements as they are, shows them processed. The next adaptor in the pipeline sees only this view.

Chaining pipes creates a tree of wrapper objects rooted at the original range. Each layer adds a small constant amount of overhead per element access. The C++ optimizer can typically inline through all these layers and produce machine code that is nearly as efficient as a hand-written loop with all the logic inlined, particularly with modern compilers and optimization levels.

It must be kept in mind that the full type of a deeply composed pipeline can be extraordinarily long and complex. If a compilation error is generated in a pipeline expression, the error message will typically dump this full nested type, which can be hundreds or thousands of characters long. Learning to read these errors requires practice and a clear mental model of which adaptor corresponds to which layer. Working from the inside out makes these errors tractable. The ```auto``` keyword is essential when working with adapted ranges precisely because the types are so complex and unwriteable by hand. The ```auto``` deduced type will be the full nested (hidden) type, which is correct and efficient. Also, the lifetime of the original range must exceed the lifetime of the view.

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

Views are non‑owning, so it must be ensured that the underlying range outlives the view.
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
Consider an example:
```
std::vector<std::string> names = people | filter(is_female)
| transform(name)
| take(3);
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

The flow is as follows:
1. When `people | filter(is_female)` is evaluated, nothing happens other than a new view being created. Not a single person is accessed from the `people` collection, except potentially to initialize the iterator to the source collection to point to the first item that satisfies the `is_female` predicate.
2. This view is passed to `| transform(name)`. The only thing that happens is that a new view is created. Again, neither a single person is accessed nor the `name` function is called on any of them.
3. Then, `| take(3)` is applied to that result. Again, it creates a new view and nothing else.
4. A vector of strings is constructed from the view which was obtained as the result of the `| take(3)` transformation. To create a vector, the values to put in must be known. This step goes through the view and accesses each of its elements. When the vector of names is to be constructed from the range, all the values in the range have to be evaluated. 


For each element added to the vector, the following things happen:
1. A dereference operator is called on the proxy iterator that belongs to the range view returned by take, i.e., ```take_view::iterator::operator*()```. The proxy iterator created by take passes the request to the proxy iterator created by `transform`.
2. Which calls ```transform_view::iterator::operator*()```. This iterator just passes on the request.
3. It calls ```filter_view::iterator::operator*()```. The proxy iterator defined by the `filter` transformation is dereferenced. It goes through the source collection and finds and returns the first person that satisfies the `is_female` predicate. This is the first time any of the persons in the collection are accessed, and the first time the `is_female` function is called.
4. This iterator advances until predicate is satisfied. The person retrieved by dereferencing the `filter` proxy iterator is passed to the `name` function, and the result is returned to the `take` proxy iterator, which passes it on to be inserted into the `names` vector. When an element is inserted, it goes to the next one, and then the next one, until the end is reached. 
5. It finally calls ```ref_view::iterator::operator*()```, which reads from the vector, and the final value flows back up through `transform`.

This is lazy evaluation at work. Even though the code is shorter and more generic than the equivalent handwritten for loop, it does exactly the same thing and has no performance penalties. Each element of the underlying container is processed on demand, one at a time, with the full pipeline fused together. The compiler typically inlines everything into a tight loop.

From the address‑space perspective:
* ```names``` owns a contiguous buffer.
* ```filter_view``` stores a pointer‑like view into this buffer (```m_base``` ≈ ```names.data()``` and ```names.size()```).
* ```transform_view``` stores only:
    * Another pointer‑like base iterator.
    * A small function‑object ```fn``` (likely just a ```size_t```‑sized lambda).
* ```take_view``` stores:
    * A pointer‑like base iterator.
    * Two ```size_t```s: ```m_count```, ```m_max```.

No extra storage is allocated for the intermediate sequences.

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

This keeps the pipeline design lazy and composable, while giving you owned storage where needed.


Sources:
* https://www.youtube.com/watch?v=HYENjkZvsrM
* https://www.youtube.com/watch?v=Rbl3h0RJuuY
* https://www.youtube.com/watch?v=Q434UHWRzI0
* https://www.youtube.com/watch?v=5iXUCcFP6H4
* Functional Programming in C++ by Ivan Cukic
