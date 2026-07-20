# Callable entities in C++
There are different types of callable entities in C++:
1. Function-like macros
2. Global/Namespace/Member functions
3. Function pointers
4. References to functions
5. Functors
6. Lambdas (Since C++11)

Out of these, function pointers, function references, and functors are object types, i.e., they can be used like regular variables, pointers, and references. This allows the user to call functions dynamically at runtime, pass them as arguments to other functions (callbacks), or store them in arrays for complex logic.

## Function pointers
A function pointer is a variable that stores the memory address of a function. The signature of a function pointer must match the return type, calling convention, and parameter list of the function it points to.

### Global/Namespace function pointer
A global or namespace function pointer is declared as:
```cpp
return_type (*funcPtr)(parameter_types);
```
The the address of a function is then assigned to it.
```cpp
funcPtr = &myFuncName; // '&' is optional; 'funcPtr = myFuncName;' also works
```
Finally, the function is invoked via its pointer:
```cpp
auto result = funcPtr(/* ... */);
```

For example, consider a function
```cpp
int add (int a, int b)
{
    return a+b;
}
```
Its function pointer would be:
```cpp
// 1. Declaration: return_type (*pointer_name)(parameter_types);
int (*funcPtr)(int, int);

// 2. Initialization: Assign the address of a function
funcPtr = &add; // '&' is optional; 'funcPtr = add;' also works

// 3. Invocation: Call the function via the pointer
int result = funcPtr(10, 5); // Equivalent to add(10, 5)
```

Function pointers can also be used with ```typedef```.
```cpp
typedef int (*funcPtr)(int, int);
// ...
funcPtr f = add; // OR, funcPtr f = &add;
int res = f(10, 5);
```

### Member function pointers
Function pointers to member functions are declared in the following manner:
```cpp
ReturnType (ClassName::*ptrName)(Params) = &ClassName::Member;
```
And the invocation is done using ```.*``` or ```->*``` operator.
For example:
```cpp
class Calculator {
public:
    int add(int a, int b) { return a + b; }
    // ...
};

// Function pointer
int (Calculator::*ptr)(int, int);

int main() {
    Calculator calc;
    // Pointer to a member function of class Calculator
    //Equivalent to int (Calculator::*ptr)(int, int) = &Calculator::add;
    ptr = &Calculator::add;
    
    // Call using an object instance
    int res = (calc.*ptr)(5, 5); 
    return 0;
}
```

### Operations allowed
1. Assign an address of a function
2. Compare 2 function pointers
3. Call a function using the pointer
4. Pass the function pointer as an argument to another function
5. Return a function pointer
6. Store them in arrays

## Function references
Just as there is a pointer to a function, an alias (reference) can be created for a function name. The function name binds directly to the reference rather than decaying to a pointer.

**Syntax**
```cpp
return_type (&referenceName)(param_list)
```
The `&` goes next to the name, wrapped in parentheses to bind tighter than the return type.

For example:
```cpp
void greet()
{
    std::cout << "Hello!";
}

int main()
{
    void (&ref)() = greet; // ref is now an alias for greet
    ref();                 // Calls greet()
}
```

A function reference must be initialized when declared (there is no "null" reference), and like all references it cannot be uninitialized or rebound to a different function afterward. Once a reference is bound to an object (or function), it cannot be changed to refer to another one.

Taking the address of a function reference gives you a pointer to the underlying function.
```cpp
int (*p)(int, int) = &fref;        // p points to add
```
The call syntax of a function reference is identical to that of a function pointer — `fref(...)`, `fptr(...)`, and `(*fptr)(...)` all work — but the difference is that a reference binds to a function without that decay.

Function references show up most often in template deduction. Passing a function by reference deduces the function type; passing by value decays it to a pointer:
```cpp
template <typename T> void byRef(T& f);   // T = int(int,int), f is a function reference
template <typename T> void byVal(T  f);   // T = int(*)(int,int), f is a function pointer

byRef(add);   // f is a reference to add
byVal(add);   // f is a pointer to add
```

## Functors
A functor is basically a class that overloads the function call operator (`()`), allowing an instance of a class to be called like a function. A functor's type is known at compile-time, so compilers can often inline the function logic directly into the calling code. This makes functors generally faster than function pointers.
For example:
```cpp
#include <iostream>

// A functor
class AdderFunctor {
public:
    int operator() (int a, int b) { return a + b; }
} aF;

int main()
{
    int x = 5, y = 7;
    
    int zf = aF(x, y); // aF.operator()(x,y)
    std::cout << "zf = " << zf << std::endl;
}
```
There are three big advantages over a regular function or a function pointer: 
* They can carry state. A function pointer is just an address; a functor is an object with member variables, so it can remember things between calls or be configured at construction.
* They're cheap to inline. Each functor is a distinct type, so when you pass it to a template the compiler knows exactly which `operator()` to call and can inline it. A function pointer is opaque at the call site and usually cannot be inlined.
* They integrate with the STL.

State doesn't have to be mutable — it can be constructed on the go.
Consider an example:
```cpp
class MultiplyBy {
    int factor;
public:
    explicit MultiplyBy(int f) : factor(f) {}
    int operator()(int x) const { return x * factor; }
};

MultiplyBy times3(3);
times3(10);   // 30
```
Now, `times3` behaves like a specialized `multiply-by-3` function. This creates a family of functions parameterized by factor. In other words, a factory of functions can be created by configuring an additional member of a functor.

A functor can also change its state after multiple calls, the way a function changes global or static variables to record state changes.
```cpp
struct Accumulator {
    int sum = 0;
    int operator()(int x) {   // note: NOT const, it mutates
        sum += x;
        return sum;
    }
};

Accumulator acc;
acc(10);   // 10
acc(20);   // 30
acc(5);    // 35  — state persists in acc.sum
```

The C++ Standard Library provides various built-in functors for common operations in the ```<functional>``` header: 
* Arithmetic: ```std::plus```, ```std::minus```, ```std::multiplies```, ```std::divides```, ```std::modulus```.
* Relational: ```std::equal_to```, ```std::not_equal_to```, ```std::greater```, ```std::less```, ```std::greater_equal```, ```std::less_equal```.
* Logical: ```std::logical_and```, ```std::logical_or```, ```std::logical_not```. 

One interesting modern C++ feature is that Lambdas *are* functors. The compiler generates an anonymous functor (a "closure type") when it encounters a lambda. The capture list becomes member variables, the body becomes the `operator()` body, and the call operator is const by default.

That is,
```cpp
int threshold = 10;
auto pred = [threshold](int x) { return x > threshold; };
```

is conceptually equivalent to:
```cpp
class __anonymous {
    int threshold;
public:
    explicit __anonymous(int t) : threshold(t) {}
    bool operator()(int x) const { return x > threshold; }
};
auto pred = __anonymous(threshold);
```

Adding mutable makes it non-const so the body can modify captured-by-value members:
```cpp
auto counter = [n = 0]() mutable { return ++n; };   // mutable: can change n
counter();  // 1
counter();  // 2
```
That's exactly the `Accumulator` pattern from earlier, written compactly. Captures by reference (`[&x]`) become reference members.


A functor can also be made generic to support any data type:
```cpp
struct Print {
    template <typename T>
    void operator()(const T& x) const {
        std::cout << x << '\n';
    }
};

Print p;
p(42);        // works
p("hello");   // works
p(3.14);      // works
```

Its equivalent lambda is:
```cpp
auto print = [](const auto& x) { std::cout << x << '\n'; };
```

To evaluate a functor at compile-time, `operator()` can be constexpr and marked `noexcept`. For example:
```cpp
struct Square {
    constexpr int operator()(int x) const noexcept { return x * x; }
};
constexpr int nine = Square{}(3);   // evaluated at compile time
```

## Modern C++ Alternatives (2025 Context)
While raw function objects are efficient, modern C++ (C++11 and later) provides more flexible alternatives: 
* ```std::function```: A type-safe wrapper that can store function pointers, lambdas, or functors.
* Lambdas: Anonymous functions that can be passed directly to other functions without declaring a named function (internally converted to a functor by the compiler).
* ```std::invoke```: A universal way to call any callable (added in C++17) that simplifies the syntax for member function pointers.
* ```std::function_fref```: A type-safe function reference

Reference: https://www.youtube.com/watch?v=i7-jWzWOBbk&t=79s
