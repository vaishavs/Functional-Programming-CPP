# Extending the Boost.Range library
The Boost.Range library is structured in a way that the standard containers will work seamlessly with it. Extending a library is a great way to make any custom data structure work just as well with the it.

Extending Boost.Range involves:
- Making your custom type compatible with `boost::begin/end`
- Optionally adding traits (`range_iterator`, `range_value`, etc.)
- Enabling it to work with adaptors (`filtered`, `transformed`, etc.)

## Extending a custom container
To make a custom container compatible, it is required to provide the following member functions:
* `begin()`
* `end()`
and the member types:
* `iterator`
* `const_iterator`

#### Step 1: Creating a Custom Container
The first step involves creating a container that contains `begin()` and `end()` iterators.
```
#include <vector>

// Fixed-capacity ring buffer — acts as a range automatically.
template<typename T, std::size_t N>
class RingBuffer {
    T    data_[N]{};
    std::size_t size_ = 0;
public:
    // ── These two are all Boost.Range needs ──────────────────────────
    T*       begin()       { return data_;          }
    T*       end()         { return data_ + size_;  }
    const T* begin() const { return data_;          }
    const T* end()   const { return data_ + size_;  }
};
```

#### Step 2: Adding a trait
For types that cannot be modified, such as C structs, third-party classes, legacy code, or user-defined types (UDTs) two free-standing functions must be specified by a class for it to be useable as a certain Range concept:
* `range_begin` 
* `range_end`

For example:
```
#include <boost/range.hpp>
#include <iterator>         // for std::iterator_traits, std::distance()

namespace Foo
{
	// A sample data structure. The 'Pair' will work as a range when the stored elements are iterators.
	template< class T >
	struct Pair {
		T first, last;
	};

	// The required functions. These should be defined in the same namespace as 'Pair'
	template< class T >
	inline T range_begin( Pair<T>& x ) { return x.first; }

	template< class T >
	inline T range_begin( const Pair<T>& x ) { return x.first; }

	template< class T >
	inline T range_end( Pair<T>& x )  return x.last; }

	template< class T >
	inline T range_end( const Pair<T>& x ) { return x.last; }
}
```
The `range_begin` / `range_end` hooks must live in the exact same namespace as the type.

To fully integrate the **unmodifiable/UDT** with Boost, the metafunctions should also be specialized for the type, defined in `boost/range.hpp`:
* `boost::range_mutable_iterator`
* `boost::range_const_iterator`

```
#include <boost/range.hpp>
#include <iterator>         // for std::iterator_traits, std::distance()

// Specialize metafunctions. 
// The range.hpp header must be included and the ‘boost’ name space must be opened.
namespace boost
{
	template< class T >
	struct range_mutable_iterator< Foo::Pair<T> >  { typedef T type; };

	template< class T >
	struct range_const_iterator< Foo::Pair<T> >  { typedef T type; };
}
```
Now, this type can be used with adaptors and algorithms as usual.

## Extending an algorithm
The construction of a custom algorithm within the Boost.Range ecosystem is achieved by moving beyond simple iterator pairs and embracing the library's trait-based architecture. It is through this architecture that the algorithm is ensured to work not just with standard containers, but also with adapted ranges, filtered views, and even raw arrays.

The following guide details the construction of a `find_last` algorithm—by which the last occurrence of a value in a range is located—using the professional conventions found within the Boost source tree.

#### Step 1: Defining the Range Traits
The determination of the correct types for iterators and values, without the assumption that the input is a specific container like `std::vector`, is the first requirement for a robust range algorithm. These pieces of information metadata are extracted through header-only traits provided by Boost. Through the use of `boost::range_iterator<Range>::type`, the algorithm is automatically adjusted according to whether a const or non-const range is being processed.
```
#include <boost/range/functions.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/traversal.hpp>

template<typename ForwardRange, typename Value>
inline typename boost::range_iterator<ForwardRange>::type
find_last(ForwardRange& rng, const Value& val)
{
    // Step 2 & 3 logic goes here
}
```

#### Step 2: Concept Assertion
Requirements are enforced at compile time before any logic is executed. If the ability to step backward is required by the algorithm (e.g., for performance), an assertion must be made that the `BidirectionalRangeConcept` is satisfied by the range. Cryptic template error messages deep in the compilation stack are prevented by this practice.

`BOOST_CONCEPT_ASSERT((boost::BidirectionalRangeConcept<ForwardRange>));`

#### Step 3: Implementation via Range Accessors
The standalone `boost::begin(rng)` and `boost::end(rng)` functions are used instead of calling `rng.begin()`. This distinction is considered critical; these functions are overloaded so that C-style arrays and pointer pairs, which do not possess member functions, can be handled.

```
template<typename ForwardRange, typename Value>
inline typename boost::range_iterator<ForwardRange>::type
find_last(ForwardRange& rng, const Value& val)
{
    using iter_t = typename boost::range_iterator<ForwardRange>::type;
    
    iter_t result = boost::end(rng);
    iter_t it = boost::begin(rng);
    iter_t last = boost::end(rng);

    for (; it != last; ++it) {
        if (*it == val) {
            result = it;
        }
    }
    return result;
}
```

#### Step 4: Supporting Range Composition (The Pipe Operator)
To allow the algorithm to be used in a functional pipeline (e.g., `my_range | find_last(x)`), a "Range Adaptor" must be implemented. This is accomplished through the creation of a function object where the arguments are stored, and the overloading of operator `|`.
```
namespace detail {
    template<typename T>
    struct find_last_forwarder {
        T value;
        find_last_forwarder(T val) : value(val) {}
    };
}

// The pipe operator overload
template<typename ForwardRange, typename T>
inline typename boost::range_iterator<ForwardRange>::type
operator|(ForwardRange& rng, const detail::find_last_forwarder<T>& f)
{
    return find_last(rng, f.value);
}

// Helper function to create the forwarder
template<typename T>
inline detail::find_last_forwarder<T> find_last(T val)
{
    return detail::find_last_forwarder<T>(val);
}
```

#### Step 5: Putting it Together
With this structure, the custom algorithm is rendered indistinguishable from built-in Boost tools. Type deduction is handled, const-correctness is respected, and the declarative syntax preferred in modern C++ development is supported.
```
#include <vector>
#include <iostream>

int main() {
    std::vector<int> data = {1, 2, 3, 2, 1};
    
    // Usage 1: Direct Call
    auto it1 = find_last(data, 2);
    
    // Usage 2: Pipe Syntax (Functional style)
    auto it2 = data | find_last(2);

    if (it2 != boost::end(data)) {
        std::cout << "Found at index: " << std::distance(boost::begin(data), it2);
    }
}
```
The decoupling of code from specific data structures is ensured by this tiered approach, while the full optimization benefits of the Boost.Range iterator abstractions are gained.

## Extending an adaptor
Internally, a Boost.Range adaptor is constructed by combining a custom iterator, a range that wraps those iterators, and a pipe operator `|`, which allows the `range | adaptor` syntax to be utilized.
To illustrate this process, let us take a custom adaptor named `add_value` that takes a range of numbers and adds a specific constant to each element lazily as the range is traversed.

#### Step 1: Defining the Underlying Iterator
Because a range is defined by its iterators, an iterator must be written to wrap the original iterator and modify the behavior when dereferencing occurs.
The `boost::iterator_adaptor` is utilized for this purpose. The boilerplate code (incrementing, comparing, etc.) is handled by this utility, so only the dereference() method needs to be overridden.
```
#include <boost/iterator/iterator_adaptor.hpp>

// Template on the underlying Iterator and the Type of value to add
template <class Iterator, class ValueType>
class add_value_iterator
    : public boost::iterator_adaptor<
        add_value_iterator<Iterator, ValueType>, // CRTP: Pass derived class
        Iterator,                                // Base iterator
        ValueType,                               // Value type
        boost::use_default,                      // Category (inherit from Base)
        ValueType                                // Reference type! (Return by value)
      >
{
public:
    // Default constructor is required for iterators
    add_value_iterator() {}

    // Constructor taking the base iterator and the value to add
    add_value_iterator(Iterator base, ValueType val)
        : add_value_iterator::iterator_adaptor_(base), m_val(val) {}

private:
    // Grant access to boost::iterator_adaptor for core operations
    friend class boost::iterator_core_access;

    ValueType m_val;

    // The dereference is intercepted here.
    ValueType dereference() const {
        return *(this->base()) + m_val;
    }
};
```
Note: The fifth template parameter for iterator_adaptor is set to ValueType rather than ValueType&. Because a new value is calculated on the fly (*base + val), it must be returned by value.

#### Step 2: The Adaptor "Holder" (The Tag) is Created
When the expression `rng | add_value(5)` is processed, `add_value(5)` is evaluated first. A temporary object is returned to hold the state (the number 5) until it can be bound to the range by the pipe operator.
```
// The holder struct
template <typename ValueType>
struct add_value_holder {
    ValueType val;
    add_value_holder(ValueType v) : val(v) {}
};

// The generator function
template <typename ValueType>
add_value_holder<ValueType> add_value(ValueType v) {
    return add_value_holder<ValueType>(v);
}
```

#### Step 3: Overloading the Pipe Operator (operator `|`)
The interaction between a Range and the Holder is defined by overloading operator `|`. A range is taken on the left and an `add_value_holder` is taken on the right. A `boost::iterator_range`, populated with the custom iterators, is then returned.
```
#include <boost/range/iterator_range.hpp>

// Non-const range version
template <typename SinglePassRange, typename ValueType>
inline boost::iterator_range<
    add_value_iterator<typename boost::range_iterator<SinglePassRange>::type, ValueType>
>
operator|(SinglePassRange& rng, const add_value_holder<ValueType>& holder) {
    
    typedef typename boost::range_iterator<SinglePassRange>::type iterator_t;
    typedef add_value_iterator<iterator_t, ValueType> custom_iter_t;

    return boost::make_iterator_range(
        custom_iter_t(boost::begin(rng), holder.val),
        custom_iter_t(boost::end(rng), holder.val)
    );
}

// Const range version
template <typename SinglePassRange, typename ValueType>
inline boost::iterator_range<
    add_value_iterator<typename boost::range_iterator<const SinglePassRange>::type, ValueType>
>
operator|(const SinglePassRange& rng, const add_value_holder<ValueType>& holder) {
    
    typedef typename boost::range_iterator<const SinglePassRange>::type iterator_t;
    typedef add_value_iterator<iterator_t, ValueType> custom_iter_t;

    return boost::make_iterator_range(
        custom_iter_t(boost::begin(rng), holder.val),
        custom_iter_t(boost::end(rng), holder.val)
    );
}
```

#### Step 4: Verifying the Implementation
The custom adaptor can now be used in the same manner as `boost::adaptors::transformed`.
```
#include <iostream>
#include <vector>

int main() {
    std::vector<int> numbers = {1, 2, 3, 4, 5};

    // The custom adaptor is applied!
    auto adapted_range = numbers | add_value(10);

    for (int n : adapted_range) {
        std::cout << n << " ";
    }
    // Output: 11 12 13 14 15 

    std::cout << "\n";
    
    // The original range remains unmodified
    for (int n : numbers) {
         std::cout << n << " ";
    }
    // Output: 1 2 3 4 5

    return 0;
}
```


Sources:
* [Extending Boost.Range](https://www.boost.org/doc/libs/latest/libs/range/doc/html/range/reference/extending.html)
* [Using boost::range](https://www.boost.org/doc/libs/latest/libs/numeric/odeint/doc/html/boost_numeric_odeint/odeint_in_detail/using_boost__range.html)
* [Boost Range for humans](https://www.caichinger.com/boost-range/index.html)
