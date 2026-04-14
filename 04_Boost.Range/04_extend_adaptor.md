# Extending an adaptor
Internally, a Boost.Range adaptor is constructed by combining a custom iterator, a range that wraps those iterators, and a pipe operator `|`, which allows the `range | adaptor` syntax to be utilized.
To illustrate this process, let us take a custom adaptor named `add_value` that takes a range of numbers and adds a specific constant to each element lazily as the range is traversed.

#### Step 1: Defining the Underlying Iterator
Because a range is defined by its iterators, an iterator must be written to wrap the original iterator and modify the behavior when dereferencing occurs.
The `boost::iterator_adaptor` is utilized for this purpose. The boilerplate code (incrementing, comparing, etc.) is handled by this utility, so only the `dereference()` method needs to be overridden.
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
Note: The fifth template parameter for `iterator_adaptor` is set to `ValueType` rather than `ValueType&`. Because a new value is calculated on the fly `(*base + val)`, it must be returned by value.

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
