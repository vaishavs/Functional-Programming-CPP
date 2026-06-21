# The box model
An Applicative Functor (often just called an "`Applicative`") is a very practical design pattern that helps with writing cleaner, safer code when dealing with wrapped values—like `std::optional`, `std::expected`, or smart pointers.

The "box" analogy is the most effective way to understand functional programming patterns. In C++, a box is not a cardboard container; it is a computational context. It is a set of strict rules governing how the item inside is allowed to be handled, moved, or altered.

## Components of the Box model
* **Data**: These are the naked C++ types such as `int`, `float`, `std::string`, or class types. This is the data/raw material on which operations are performed. They are easy to manipulate but lack any protective infrastructure.
* **Functions**: These are the operations and transformations applied on data. It acts like the machine in a factory that alters the raw material. They can be ordinary functions or [higher-order functions](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/03_HOF.md).
* **The Context**: This is a wrapper around the raw value that provides extra context to that value. In the factory analogy, when a raw material is put into a box, it is no longer just a material; it is a material bound by the rules of logistics. Different boxes carry different contexts.
  * `std::optional`: This is like The "Schrödinger" Box. The rule is Uncertainty. The box either contains the value, or it is completely empty. It is a box with the rule: *"I might contain a value, or I might be empty."*
  * `std::vector`: This is like The "Pallet" Box. The rule is Multiplicity. The box contains zero, one, or thousands of items. It is a box with the rule: *"I contain zero, one, or many values."*
  * `std::expected<T, E>`: This is like The "Customs" Box. The rule is Strict Accountability. This box contains either the intended value, or a detailed error message. It is a box with the rule: *"I contain either a value, or an error note explaining why the value is invalid."*

[![diagram-0-box-model.jpg](https://i.postimg.cc/k5Xr9mWY/diagram-0-box-model.jpg)](https://postimg.cc/zyM2S4tT)

# The Mechanisms
Often, it is needed to work on values that come wrapped in some kind of context. The naive way to work with such values is to unwrap them, do the computation, and rewrap. But that is tedious and it forces the programmer to handle the wrapper's meaning by hand every single time (checking for absence, propagating errors, looping over elements). In order to address this, there are 3 mechanisms in C++, each with increasing complexity:

### 1. Functor
A Functor applies a standard, normal function to a wrapped value. (See [functors](https://github.com/vaishavs/Functional-Programming-CPP/blob/main/02_Designing_HO_Funcs/01_intro.md#functors)). It handles the safety checks of a container internally. A single operation is usually called as `map`. Abstractly, `map` takes a function from `a` to `b`, and a `context-of-a`, and produces a `context-of-b`. The decisive feature is that the supplied function knows nothing about the context. It's a plain function operating on plain values; the functor machinery is what slips it inside the wrapper and back out. 

[![diagram-1-functor.jpg](https://i.postimg.cc/yYQY2LqD/diagram-1-functor.jpg)](https://postimg.cc/mt9sHNg4)

What makes something a lawful functor is two requirements that together pin down what mapping is allowed to do.:
* mapping the identity function (the function that returns its argument unchanged) must leave the whole structure unchanged.
* mapping one function and then mapping a second is the same as mapping the composition of the two in a single pass.

Taken together, these laws say that mapping only ever transforms the contents and never disturbs the structure. It cannot add or drop elements from a list, cannot turn an absent value into a present one, cannot reorder anything, cannot fail. Shape preserved, contents changed—that is the entire essence of a functor.
### 2. Applicative
An applicative lets you feed both wrapped inputs into that function and get a wrapped result. The function is itself in a box, and it is applied to a boxed argument. A Functor's `map` takes a one-argument function. But in case of using a multi-argument function, using a functor would be very limiting. For example:
```
auto stuck = a.transform([](int x){
    return [x](int y){ return x + y; };   // partially-applied add
});
// stuck has type: optional<  (int -> int)  >
//                 a FUNCTION, trapped inside a box
```
A functor cannot operate on multiple boxes at once. This exact gap is what the applicative operation fills:

[![diagram-2-applicative.jpg](https://i.postimg.cc/d3Qwxy4d/diagram-2-applicative.jpg)](https://postimg.cc/zbQsLVmX)

So the two steps chained together solve the problem:
```
ap( a.transform(add_curried), b )   // optional{8}
```
The payoff is combining. 

In practice, this is done using a friendly helper called lift (a.k.a. `liftA2`, `liftA3`, …) that takes an ordinary multi-argument function and the boxes, and just does the right thing:
```
auto add = [](int a, int b){ return a + b; };
lift(add, std::optional{3}, std::optional{5});   // optional{8}
```

This lift move — "apply an ordinary N-argument function to N boxed values" — is the single most important thing applicatives give. Also, the boxes are *independent* — the programmer can use all of them before knowing any of their contents. The structure is known before anything runs, so applicative computations can be analysed, optimised, and even run in parallel.

One more thing worth noticing is that every applicative is automatically a functor too. If it can combine boxes, it can certainly handle the one-box case. So, an applicative is just a more capable functor. 

### 3. Monads
Sometimes it cannot be decide what the next computation even is until the value inside the current box is seen. For example: "look up a user id, and then — using that id — look up their account." The second box (the account lookup) literally doesn't exist until you have the id. That need is what a Monad adds. It lets a later step depend on the result of an earlier one. Here, the function takes a plain value but hands back another box.

[![diagram-3-monad.jpg](https://i.postimg.cc/FzzwSgTq/diagram-3-monad.jpg)](https://postimg.cc/vxRPF9FL)

When a monad runs a function, that function receives the actual value from the *previous* step, and it gets to decide—based on that value—what to do next. The plan is **not** fixed ahead of time anymore; it unfolds as the values come through. Also, every monad is an applicative (and therefore a functor).

# Overall picture
The cleanest way to keep them straight is by the kind of function that is being applied.
* Functor: value in, value out, with internal box handling; changes what's in the box
* Applicative: the function is itself in a box, which is what combines several boxed inputs; no dependency between steps
* Monad: the function produces its own box; each step can depend on the results before it

```
Functor      map :  (A -> B) -> box<A> -> box<B>
      │          apply a PLAIN function inside a box
      │  (add pure + ap)
      ▼
  Applicative   ap  :  box<(A -> B)> -> box<A> -> box<B>
      │          combine SEVERAL INDEPENDENT boxes with one function
      │  (add bind)
      ▼
    Monad       bind:  box<A> -> (A -> box<B>) -> box<B>
                 each step may DEPEND on the previous value
```

Each rung includes the powers of the one above and adds more. Every monad is an applicative; every applicative is a functor. However, the reverse is **not** true — which is the point of having three names instead of one.

```
  power:   ▓▓░░░░░░░   →   ▓▓▓▓▓░░░░   →   ▓▓▓▓▓▓▓▓▓
            functor         applicative      monad
            (each level contains the one before it)
```
