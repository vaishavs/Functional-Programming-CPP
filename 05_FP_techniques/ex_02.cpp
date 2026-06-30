#include <iostream>

// =============================================================================
//  Box<T> -- a one-value container that demonstrates the three "type classes"
//  at the heart of functional programming: Functor, Applicative, and Monad.
// =============================================================================
template <class T> struct Box {
    // Box is a plain struct with a public member and no hand-written constructor,
    // so the syntax Box{x} just copies x into v. No constructor is necessary.
    T value;

    // -------------------------------------------------------------------------
    // FUNCTOR  --  map
    //
    //   Concept:   Box<T>,  (T -> U)      ==>  Box<U>
    //
    // The return type Box<decltype(f(v))> is built from whatever type f(v)
    // produces -- f might turn an int into a string, so the result type is
    // allowed to differ from T.
    // -------------------------------------------------------------------------
    auto map(auto transform) {
        return Box<decltype(transform(value))>{ transform(value) };
    }

    // -------------------------------------------------------------------------
    // APPLICATIVE  --  ap
    //
    //   Concept:   Box<T -> U>,  Box<T>    ==>  Box<U>
    //
    // Like map, but the FUNCTION is itself inside a box. Here this box is the
    // one holding the function (so its 'value' IS the callable), and the
    // argument 'valueBox' is the box holding the plain value (so valueBox.value
    // is what we feed in). We call value(valueBox.value) and wrap the result.
    //
    // Note: since this box must hold a callable, ap only compiles when called on a box of a function. 
    // Calling ap on a Box<int> is a type error -- which is precisely the guarantee needed from the type system.
    // -------------------------------------------------------------------------
    auto ap(auto valueBox) {
        return Box<decltype(value(valueBox.value))>{ value(valueBox.value) };
    }

    // -------------------------------------------------------------------------
    // MONAD  --  bind
    //
    //   Concept:   Box<T>,  (T -> Box<U>)  ==>  Box<U>
    //
    // The function 'makeBox' here already RETURNS A BOX. If the result is re-
    // wrapped the way map does, a useless Box<Box<U>> is obtained; instead, bind
    // returns makeBox(value) directly, "flattening" the two layers into one.
    // This is what makes step-by-step chaining work:
    //     box.bind(step1).bind(step2)...
    //
    // For this always-full box, flattening is all bind does. In a box that can
    // be empty (an optional), bind is also where a missing value short-circuits
    // the rest of the chain -- same interface, richer behavior.
    // -------------------------------------------------------------------------
    auto bind(auto makeBox) { return makeBox(value); }
};

int main() {
    // CTAD (class template argument deduction) reads {5} and deduces Box<int>.
    Box numberBox{5};

    // FUNCTOR: the lambda is a plain int->int; map runs it inside the box.
    //          5  --(*2)-->  Box<int>{10}
    auto res_map = numberBox.map([](int num){ return num * 2; }).value;
    std::cout << res_map << '\n';        // 10

    // APPLICATIVE: the unary '+' turns the capture-less lambda into a function
    //              pointer int(*)(int), so the box holds a concrete callable and
    //              CTAD deduces Box<int(*)(int)>. .ap(numberBox) applies that
    //              boxed function to the boxed value:  f(5) = 105.
    auto res_ap = Box{+[](int num){ return num + 100; }}.ap(numberBox).value;
    std::cout << res_ap << '\n';  // 105

    // MONAD: the lambda returns a *box* (Box<int>{num*num}); bind flattens it,
    //        so the result is Box<int>{25}, not Box<Box<int>>.
    auto res_bind = numberBox.bind([](int num){ return Box{num * num}; }).value;
    std::cout << res_bind << '\n'; // 25
}
