#include <iostream>
#include <memory>
#include <string>

// =============================================================================
// CATEGORY THEORY: THE BOX MODEL (TODO EXERCISES)
// =============================================================================

// A "Box" is a wrapper around a value of type T. 
// In category theory terms, we want to make this Box a Functor and a Monad.
template <typename T>
class Box {
private:
    std::unique_ptr<T> value_ptr;

public:
    // Constructor: Wraps a value inside the Box context.
    // (In Category Theory, this is often called 'pure', 'return', or 'unit').
    explicit Box(T val) : value_ptr(std::make_unique<T>(std::move(val))) {}

    // Utility to read the value (for testing)
    const T& get() const { return *value_ptr; }

    // --- TASK 1: FUNCTOR (fmap / map) ---
    // A Functor requires a mapping operation. It takes a standard function 
    // (which transforms type T to type U) and applies it inside the Box, 
    // returning a new Box<U>.
    //
    // TODO: Implement the map function.
    // 1. Dereference value_ptr to get the inner value.
    // 2. Apply the function 'f' to it.
    // 3. Wrap the result in a new Box and return it.
    
    template <typename Func>
    auto map(Func f) const {
        // Hint: In C++14 and later, 'auto' handles the return type deduction for you!
        // Replace the throw statement with your implementation.
        throw std::runtime_error("map() not implemented");
    }

    // --- TASK 2: MONAD (bind / flatMap) ---
    // A Monad requires a binding operation. Unlike map, the function 'f' here 
    // takes a type T and ALREADY returns a Box<U>. 
    // If we used map, we'd get a Box<Box<U>>. Bind "flattens" it to just Box<U>.
    //
    // TODO: Implement the bind function.
    // 1. Dereference value_ptr to get the inner value.
    // 2. Apply the function 'f' to it.
    // 3. Return the result directly (since 'f' already returns a Box!).
    
    template <typename Func>
    auto bind(Func f) const {
        // Replace the throw statement with your implementation.
        throw std::runtime_error("bind() not implemented");
    }

    // --- TASK 3: APPLICATIVE FUNCTOR (apply / ap) (TODO) ---
    // An Applicative Functor goes one step further than a standard Functor.
    // What if the function we want to apply is ALSO wrapped in a Box?
    //
    // TODO: Implement the apply function.
    // 1. Extract the function from 'func_box' (Hint: use .get()).
    // 2. Extract the inner value from this current box (dereference value_ptr).
    // 3. Apply the extracted function to the extracted value.
    // 4. Wrap the result in a new Box and return it.
    
    template <typename FuncBox>
    auto apply(const FuncBox& func_box) const {
        // Replace the throw statement with your implementation.
        throw std::runtime_error("apply() not implemented");
    }
};

// =============================================================================
// HELPER FUNCTIONS TO TEST OUR CONTEXT
// =============================================================================

// A standard function: int -> int
int square(int x) {
    return x * x;
}

// A standard function: int -> string
std::string int_to_string(int x) {
    return "The number is " + std::to_string(x);
}

// A Monadic function: int -> Box<int>
Box<int> add_ten_in_box(int x) {
    return Box<int>(x + 10);
}

// =============================================================================
// MAIN FUNCTION - TEST YOUR CODE
// =============================================================================

int main() {
    std::cout << "--- CATEGORY THEORY: BOX MODEL ---\n\n";

    Box<int> my_box(5);
    std::cout << "Original Box contains: " << my_box.get() << "\n\n";

    // UNCOMMENT TO TEST TASK 1 (FUNCTOR / MAP)
    /*
    std::cout << "--- Testing Functor (map) ---\n";
    
    auto squared_box = my_box.map(square);
    std::cout << "Squared Box contains: " << squared_box.get() << " \t(Expected: 25)\n";

    auto string_box = squared_box.map(int_to_string);
    std::cout << "String Box contains:  " << string_box.get() << " \t(Expected: The number is 25)\n\n";
    */

    // UNCOMMENT TO TEST TASK 2 (MONAD / BIND)
    /*
    std::cout << "--- Testing Monad (bind) ---\n";
    
    // If we used map here, we would get Box<Box<int>>. 
    // Bind correctly flattens it to Box<int>.
    auto plus_ten_box = my_box.bind(add_ten_in_box);
    std::cout << "Plus Ten Box contains: " << plus_ten_box.get() << " \t(Expected: 15)\n";

    // Monads allow chaining!
    auto chained_box = my_box.bind(add_ten_in_box).bind(add_ten_in_box);
    std::cout << "Chained Box contains:  " << chained_box.get() << " \t(Expected: 25)\n";
    */

    // UNCOMMENT TO TEST TASK 3 (APPLICATIVE / APPLY)
    /*
    std::cout << "--- Testing Applicative (apply) ---\n";
    
    // We wrap a standard lambda function inside a Box context
    auto multiply_by_two = [](int x) { return x * 2; };
    Box<std::function<int(int)>> boxed_function(multiply_by_two);
    
    // We apply the boxed function to our original boxed value (5)
    auto applied_box = my_box.apply(boxed_function);
    
    std::cout << "Applied Box contains: " << applied_box.get() << " \t\t(Expected: 10)\n";
    */

    return 0;
}

/*
Reference:

    // Functor
    template <typename Func>
    auto map(Func f) const {
        // We evaluate f(*value_ptr) to get the raw transformed value,
        // then construct a new Box around it.
        return Box<decltype(f(*value_ptr))>(f(*value_ptr)); 
        
        // Note: Due to C++14 auto return type deduction, you can also 
        // simply write: return Box(f(*value_ptr)); if your compiler 
        // supports Class Template Argument Deduction (CTAD - C++17).
    }

    // Monad
    template <typename Func>
    auto bind(Func f) const {
        // Because 'f' is mathematically defined as T -> Box<U>, 
        // applying 'f' to our inner value immediately yields the correct type.
        return f(*value_ptr);
    }

    // Applicative
    template <typename FuncBox>
    auto apply(const FuncBox& func_box) const {
        // 1. func_box.get() retrieves the raw function.
        // 2. *value_ptr retrieves the raw value.
        // 3. We call the function with the value, and wrap it in a new Box.
        auto func = func_box.get();
        return Box<decltype(func(*value_ptr))>(func(*value_ptr));
    }
 */
