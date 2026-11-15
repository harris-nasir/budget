#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import budget.core;

#include <chrono>

namespace
{

budget::core::period test_period()
{
    using namespace std::chrono;
    return {
        .id = 42,
        .year_month = year{2025} / month{3},
    };
}

} // namespace

TEST_CASE("month creates categories with budgets")
{
    budget::core::month month{test_period()};

    const auto id = month.create_category("Housing", 1200);
    const auto categories = month.categories();
    const auto budgets = month.category_budgets();

    REQUIRE(id == 1);
    REQUIRE(categories.size() == 1);
    CHECK(categories[0].id == id);
    CHECK(categories[0].period_id == 42);
    CHECK(categories[0].name == "Housing");
    REQUIRE(budgets.size() == 1);
    CHECK(budgets[0].category_id == id);
    CHECK(budgets[0].period_id == 42);
    CHECK(budgets[0].allocated == 1200);
}

TEST_CASE("month creates items")
{
    budget::core::month month{test_period()};

    const auto id = month.create_item("Rent", 7, 1000);
    const auto items = month.items();

    REQUIRE(id == 1);
    REQUIRE(items.size() == 1);
    CHECK(items[0].id == id);
    CHECK(items[0].category_id == 7);
    CHECK(items[0].name == "Rent");
    CHECK(items[0].expected == 1000);
}

TEST_CASE("month creates transactions")
{
    budget::core::month month{test_period()};

    const auto id = month.create_transaction("Payday", budget::core::transaction_type::income, 2500, 3, 9);
    const auto transactions = month.transactions();

    REQUIRE(id == 1);
    REQUIRE(transactions.size() == 1);
    CHECK(transactions[0].id == id);
    CHECK(transactions[0].category_id == 3);
    CHECK(transactions[0].item_id == 9);
    CHECK(transactions[0].type == budget::core::transaction_type::income);
    CHECK(transactions[0].description == "Payday");
    CHECK(transactions[0].amount == 2500);
    CHECK(transactions[0].timestamp.year() == std::chrono::year{2025});
    CHECK(transactions[0].timestamp.month() == std::chrono::March);
}
