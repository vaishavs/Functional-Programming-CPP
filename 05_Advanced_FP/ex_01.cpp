#include <functional>
#include <iostream>
#include <string>

/* Generic Partial Application Helper */
template<typename Func, typename... FixedArgs>
auto partial(Func func, FixedArgs... fixedArgs)
{
    return [=](auto... remainingArgs)
    {
        return std::invoke(func, fixedArgs..., remainingArgs...);
    };
}

/* Function to demonstrate partial application */
int calculatePrice(int quantity, int pricePerItem, int tax)
{
    return quantity * pricePerItem + tax;
}

/* Curried Function */
auto curryCalculatePrice()
{
    return [](int quantity)
    {
        return [quantity](int pricePerItem)
        {
            return [quantity, pricePerItem](int tax)
            {
                return quantity * pricePerItem + tax;
            };
        };
    };
}

int main()
{
    std::cout << "========== Partial Application ==========\n\n";

    // Fix the tax to 50
    auto withTax = partial(calculatePrice, 5);

    std::cout << "Quantity = 5\n";
    std::cout << "Price = 100\n";
    std::cout << "Tax = 20\n";

    std::cout << "Total = "
              << withTax(100, 20)
              << "\n\n";

    // Fix quantity and price
    auto fixedOrder = partial(calculatePrice, 10, 200);

    std::cout << "Quantity = 10\n";
    std::cout << "Price = 200\n";
    std::cout << "Tax = 30\n";

    std::cout << "Total = "
              << fixedOrder(30)
              << "\n\n";

    //-------------------------------------------------------

    std::cout << "========== Currying ==========\n\n";

    auto calculate = curryCalculatePrice();

    // Supply arguments one by one
    std::cout << calculate(5)(100)(20) << "\n\n";

    // Reuse intermediate functions
    auto quantity5 = calculate(5);

    auto quantity5Price100 = quantity5(100);

    std::cout << quantity5Price100(10) << '\n';
    std::cout << quantity5Price100(20) << '\n';
    std::cout << quantity5Price100(30) << "\n\n";

    //-------------------------------------------------------

    std::cout << "========== Comparison ==========\n\n";

    int partialResult = partial(calculatePrice, 5)(100, 20);

    int curriedResult = calculate(5)(100)(20);

    std::cout << "Partial Application : " << partialResult << '\n';

    std::cout << "Currying            : " << curriedResult << '\n';
}
