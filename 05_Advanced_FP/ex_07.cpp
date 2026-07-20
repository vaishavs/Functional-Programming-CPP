// ============================================================================
//  CATEGORY THEORY "BOX" MODEL IN C++
//  Functor -> Applicative -> Monad, shown with a tiny online-store checkout:
//  looking up a product, a discount code, and warehouse stock -- any of
//  which might not exist.
// ============================================================================
//
// THE "BOX" METAPHOR
//   Box<T> is a container that may or may not hold a value of type T. (It's
//   really just std::optional<T> underneath -- that IS the classic "Maybe"
//   box from category theory.) Functor, Applicative, and Monad are three
//   different ways to work with what's inside the box *without* manually
//   unwrapping it and writing "if it's there..." checks yourself.
//
//     FUNCTOR      Box<A>  + (A -> B)        -> Box<B>     via  map()
//     APPLICATIVE  Box<A>  + Box<(A -> B)>   -> Box<B>     via  ap()  (+ of())
//     MONAD        Box<A>  + (A -> Box<B>)   -> Box<B>     via  flatMap()
//
//   Rule of thumb for picking one:
//     - transform what's in ONE box with a plain function          -> Functor
//     - combine TWO+ INDEPENDENT boxes with a plain function       -> Applicative
//     - chain a step whose result is ITSELF a box, and which        -> Monad
//       depends on the previous box's unwrapped value
//
//   Each also comes with "laws" that just formalize "this behaves the way
//   you'd expect and doesn't do anything sneaky":
//     Functor laws:     map(id) == id                 (mapping identity changes nothing)
//                        map(f then g) == map(f) then map(g)
//     Applicative laws: of(x) is a "no-op" wrapper -- ap/of interact with
//                        map the way you'd expect (of(f).ap(of(x)) == of(f(x)))
//     Monad laws:        left identity, right identity, associativity --
//                        chaining flatMaps in different groupings agrees
//
// Build & run:
//   g++ -std=c++17 -Wall -Wextra -o box_model_demo box_model_demo.cpp
//   ./box_model_demo
// ============================================================================

#include <iostream>
#include <iomanip>
#include <optional>
#include <string>
#include <sstream>
#include <unordered_map>
#include <type_traits>
#include <utility>

// ----------------------------------------------------------------------------
// THE BOX
// ----------------------------------------------------------------------------
template <typename T>
class Box {
public:
    Box() : storage(std::nullopt) {}                // empty box ("Nothing")
    explicit Box(T v) : storage(std::move(v)) {}     // full box  ("Just x")

    // Applicative's "pure" / "return": lift a plain value into a box.
    static Box<T> of(T v) { return Box<T>(std::move(v)); }
    static Box<T> empty() { return Box<T>(); }

    bool hasValue() const { return storage.has_value(); }
    const T& get() const { return *storage; }   // caller must check hasValue() first

    // ---- FUNCTOR ----
    // map(): transform the value INSIDE the box with a plain function A -> B.
    // If the box is empty, map() does nothing and passes the emptiness
    // straight through -- the caller never has to check for that themselves.
    template <typename F>
    auto map(F&& f) const -> Box<std::invoke_result_t<F, const T&>> {
        using U = std::invoke_result_t<F, const T&>;
        if (!hasValue()) return Box<U>::empty();
        return Box<U>::of(f(get()));
    }

    // ---- APPLICATIVE ----
    // ap(): "this" box holds a FUNCTION (A -> B). Apply it to a boxed
    // argument. Both boxes must be full for the result to be full -- this
    // is how Applicative combines two *independent* boxed computations.
    template <typename A>
    auto ap(const Box<A>& boxedArg) const -> Box<std::invoke_result_t<T, const A&>> {
        using B = std::invoke_result_t<T, const A&>;
        if (!hasValue() || !boxedArg.hasValue()) return Box<B>::empty();
        return Box<B>::of(get()(boxedArg.get()));
    }

    // ---- MONAD ----
    // flatMap() (a.k.a. bind / chain / >>= in Haskell): like map(), but for
    // when `f` ITSELF returns a box (A -> Box<B>). Using map() with such an
    // `f` would leave you holding a Box<Box<B>> -- a box stuck inside a
    // box. flatMap() calls `f` and returns its box directly, flattening
    // that extra layer away.
    template <typename F>
    auto flatMap(F&& f) const -> std::invoke_result_t<F, const T&> {
        using ResultBox = std::invoke_result_t<F, const T&>;
        if (!hasValue()) return ResultBox::empty();
        return f(get());
    }

private:
    std::optional<T> storage;
};

// Convenience free function built on Functor + Applicative: combine two
// INDEPENDENT boxes with a plain 2-argument function in one step. This is
// exactly what "map() then ap()" gives you -- see demoApplicative() below
// for the manual, step-by-step version this is shorthand for.
template <typename A, typename B, typename F>
auto map2(const Box<A>& boxA, const Box<B>& boxB, F&& f)
    -> Box<std::invoke_result_t<F, const A&, const B&>> {
    using C = std::invoke_result_t<F, const A&, const B&>;
    if (!boxA.hasValue() || !boxB.hasValue()) return Box<C>::empty();
    return Box<C>::of(f(boxA.get(), boxB.get()));
}

// Demo-only helper: print a Box<T> (T must support operator<<).
template <typename T>
void printBox(const std::string& label, const Box<T>& box) {
    std::cout << "  " << label << ": ";
    if (box.hasValue()) std::cout << box.get() << "\n";
    else std::cout << "(empty)\n";
}

// ----------------------------------------------------------------------------
// THE "REAL WORLD" DOMAIN: a tiny online store
// ----------------------------------------------------------------------------
struct Product {
    std::string name;
    double price;
    std::string warehouseCode;
};

// Stand-ins for a real database/API call -- plain in-memory lookup tables.
static const std::unordered_map<std::string, Product> productDb = {
    {"P100", {"Wireless Mouse",      25.00, "WH-EAST"}},
    {"P200", {"Mechanical Keyboard", 90.00, "WH-WEST"}},
    {"P300", {"USB-C Hub",           40.00, "WH-UNKNOWN"}},  // warehouse with no stock entry, on purpose
};
static const std::unordered_map<std::string, double> discountDb = {
    {"SAVE10", 0.10},
    {"SAVE25", 0.25},
};
static const std::unordered_map<std::string, int> stockDb = {
    {"WH-EAST", 42},
    {"WH-WEST", 7},
    // "WH-UNKNOWN" is deliberately missing here
};

// Each lookup returns a Box, because each one might legitimately come back
// empty -- exactly the situation Functor/Applicative/Monad exist to make
// pleasant, instead of a pile of manual null/not-found checks.
Box<Product> findProduct(const std::string& id) {
    auto it = productDb.find(id);
    if (it == productDb.end()) return Box<Product>::empty();
    return Box<Product>::of(it->second);
}
Box<double> findDiscount(const std::string& code) {
    auto it = discountDb.find(code);
    if (it == discountDb.end()) return Box<double>::empty();
    return Box<double>::of(it->second);
}
Box<int> findStock(const std::string& warehouseCode) {
    auto it = stockDb.find(warehouseCode);
    if (it == stockDb.end()) return Box<int>::empty();
    return Box<int>::of(it->second);
}

// ----------------------------------------------------------------------------
// DEMO 1 -- FUNCTOR: transform what's in ONE box
// ----------------------------------------------------------------------------
void demoFunctor(const std::string& productId) {
    std::cout << "\n--- Functor: map()  [" << productId << "] ---\n";
    Box<Product> product = findProduct(productId);

    // Pull the price out and format it as a display label. We never write
    // "if the product exists..." ourselves -- map() takes care of that.
    Box<std::string> label = product.map([](const Product& p) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << p.name << " - $" << p.price;
        return oss.str();
    });
    printBox("label", label);
}

// ----------------------------------------------------------------------------
// DEMO 2 -- APPLICATIVE: combine TWO INDEPENDENT boxes
// ----------------------------------------------------------------------------
void demoApplicative(const std::string& productId, const std::string& discountCode) {
    std::cout << "\n--- Applicative: ap() / map2()  [" << productId << ", " << discountCode << "] ---\n";

    Box<double> price    = findProduct(productId).map([](const Product& p) { return p.price; });
    Box<double> discount = findDiscount(discountCode);
    // Neither lookup depends on the other's result -- textbook Applicative case.

    // The manual / "textbook" route: map() a CURRIED version of the
    // combiner over the first box. This is the exact same currying /
    // partial-application pattern from function composition -- map()
    // partially applies `pr`, leaving a boxed function still waiting for
    // `d`, which ap() then supplies.
    auto curriedApplyDiscount = [](double pr) {
        return [pr](double d) { return pr * (1.0 - d); };
    };
    Box<double> finalPriceManual = price.map(curriedApplyDiscount).ap(discount);

    // The everyday shortcut -- identical result, far less ceremony:
    Box<double> finalPrice = map2(price, discount, [](double pr, double d) { return pr * (1.0 - d); });

    printBox("finalPrice (manual map+ap)", finalPriceManual);
    printBox("finalPrice (map2 shortcut)", finalPrice);
}

// ----------------------------------------------------------------------------
// DEMO 3 -- MONAD: chain a DEPENDENT lookup
// ----------------------------------------------------------------------------
void demoMonad(const std::string& productId) {
    std::cout << "\n--- Monad: flatMap()  [" << productId << "] ---\n";
    Box<Product> product = findProduct(productId);

    // We don't know WHICH warehouse to check until we've found the product
    // -- findStock() depends on product.warehouseCode. That dependency is
    // exactly what flatMap() is for.
    //
    // If we used map() here instead:
    //     product.map([](const Product& p){ return findStock(p.warehouseCode); })
    // we'd get a Box<Box<int>> -- a box stuck inside a box. flatMap()
    // flattens that automatically by returning findStock()'s box directly.
    Box<int> stock = product.flatMap([](const Product& p) {
        return findStock(p.warehouseCode);
    });

    printBox("stock", stock);
}

// ----------------------------------------------------------------------------
// DEMO 4 -- ALL THREE TOGETHER: a full checkout summary
// ----------------------------------------------------------------------------
void demoFullCheckout(const std::string& productId, const std::string& discountCode) {
    std::cout << "\n--- Full pipeline  [" << productId << ", " << discountCode << "] ---\n";

    Box<Product> product = findProduct(productId);

    Box<int> stock = product.flatMap([](const Product& p) {                     // MONAD: dependent lookup
        return findStock(p.warehouseCode);
    });

    Box<double> price    = product.map([](const Product& p) { return p.price; });  // FUNCTOR: pluck a field
    Box<double> discount = findDiscount(discountCode);
    Box<double> finalPrice = map2(price, discount, [](double pr, double d) {       // APPLICATIVE: independent combine
        return pr * (1.0 - d);
    });

    Box<std::string> summary = map2(finalPrice, stock, [](double fp, int s) {      // APPLICATIVE again
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Final price: $" << fp << " | In stock: " << s << " units";
        return oss.str();
    });

    printBox("summary", summary);
}

// ----------------------------------------------------------------------------
int main() {
    std::cout << std::fixed << std::setprecision(2);

    demoFunctor("P100");
    demoFunctor("P999");                 // doesn't exist -> prints "(empty)", no crash

    demoApplicative("P100", "SAVE10");
    demoApplicative("P100", "NOPE");     // bad discount code -> whole result is empty

    demoMonad("P100");                   // WH-EAST has a stock entry
    demoMonad("P300");                   // WH-UNKNOWN has NO stock entry -> "(empty)"

    demoFullCheckout("P200", "SAVE25");
    demoFullCheckout("P999", "SAVE25");  // missing product short-circuits the whole pipeline safely

    std::cout << "\n";
    return 0;
}
