# Extending the Boost.Range library
The Boost.Range library is structured in a way that the standard containers will work seamlessly with it. Extending the Boost.Range library is a great way to make any custom data structure, adaptor, or algorithm work just as well with it.

Extending Boost.Range can be done in three ways:
1. Extending a custom container
2. Extending a custom algorithm
3. Extending a custom adaptor


In this module, we will see how to extend a custom data structure.


# Extending a custom container
Extending a container involves:
- Making your custom type compatible with `boost::begin/end`
- Optionally adding traits (`range_iterator`, `range_value`, etc.)
- Enabling it to work with adaptors (`filtered`, `transformed`, etc.)

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
For types that cannot be modified, such as C structs, third-party classes, legacy code, or user-defined types (UDTs), two free-standing functions must be specified by a class for it to be useable as a certain Range concept:
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

To fully integrate the unmodifiable/UDT with Boost, the metafunctions should also be specialized for the type, defined in `boost/range.hpp`:
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

```
#include <boost/range.hpp>
#include <iterator>         // for std::iterator_traits, std::distance()

namespace Foo
{
    template< class T >
    struct Pair
    {
        T first, last;
    };

	template< class T >
	inline T range_begin( Pair<T>& x ) { return x.first; }

	template< class T >
	inline T range_begin( const Pair<T>& x ) { return x.first; }

	template< class T >
	inline T range_end( Pair<T>& x ) { return x.last; }

	template< class T >
	inline T range_end( const Pair<T>& x ) { return x.last; }
} // namespace 'Foo'

namespace boost
{
	template< class T >
	struct range_mutable_iterator< Foo::Pair<T> >
	{
		typedef T type;
	};

	template< class T >
	struct range_const_iterator< Foo::Pair<T> >
	{
		typedef T type;
	};

} // namespace 'boost'

#include <vector>

int main(int argc, const char* argv[])
{
	typedef std::vector<int>::iterator  iter;
	std::vector<int>                    vec;
	Foo::Pair<iter>                     pair = { vec.begin(), vec.end() };
	const Foo::Pair<iter>&              cpair = pair;
	//
	// Notice that we call 'begin' etc with qualification.
	//
	iter i = boost::begin( pair );
	iter e = boost::end( pair );
	i      = boost::begin( cpair );
	e      = boost::end( cpair );
	boost::range_difference< Foo::Pair<iter> >::type s = boost::size( pair );
	s      = boost::size( cpair );
	boost::range_reverse_iterator< const Foo::Pair<iter> >::type
	ri     = boost::rbegin( cpair ),
	re     = boost::rend( cpair );

	return 0;
}
```

Sources:
* [Extending Boost.Range](https://www.boost.org/doc/libs/latest/libs/range/doc/html/range/reference/extending.html)
* [Using boost::range](https://www.boost.org/doc/libs/latest/libs/numeric/odeint/doc/html/boost_numeric_odeint/odeint_in_detail/using_boost__range.html)
