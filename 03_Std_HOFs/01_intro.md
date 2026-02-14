# Components of standard HOFs
The implementation of standard higher order functions in C++ is based on the following components:
## Callable entities
These are the function types such as function pointers, functors, lambdas, etc., that are passed around (see https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/03_HOF.md). They are used to provide specialized functional logic, such as:
* Predicate: A function that takes one or two arguments and returns a bool.
* Generator: A function that takes no arguments and returns values to fill a range.
* Operator: A function that produces specific results based on the inputs provided.
* Execution policy: A function that makes use of multi-core processors.
## Execution Contexts
These are the containers or ranges on which various operations are performed. They are:
* Containers → ```std::vector```, ```std::list```, ```std::map```, etc.
* Iterators → Mechanism to access elements of a container/range, e.g., ```std::iterator```, etc.
* Seed values → Initial values needed as starting point for various callables, e.g., for reductions (```std::accumulate``` needs a starting value).
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
Implementing a standard higher function involves the following steps:
### Step 1: Define a callable
The first step is to define a callable that contains what the desired logic. It should define what operation should be performed. This essentially involves creating a lambda, functor, or a function pointer.
### Step 2: Choose an algorithm
The next step is to select an algorithm from the standard library that suits our purpose. This step defines how the callable will be applied.
### Step 3: Provide a data/range
The execution context, i.e., the data needs to be supplied, on which the algorithm would operate. This step involves supplying a container or iterator range.

[![Copilot-20260214-144124.png](https://i.postimg.cc/xTP95pGX/Copilot-20260214-144124.png)](https://postimg.cc/VddyLRVw)

With the introduction of Ranges, the workflow has evolved into a "Pipeable" style: 
* Workflow: Data | View_Adapter | Action
* Example: ```users | std::views::filter(is_active) | std::views::transform(get_name)```

This allows for lazy evaluation, where the workflow is defined immediately but logic is only executed when you actually iterate over the final result. 
