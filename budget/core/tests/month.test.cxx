#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import budget.core;

#include <chrono>
#include <tuple>

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

budget::core::date test_date()
{
    using namespace std::chrono;
    return year{2025} / March / day{15};
}

} // namespace

TEST_CASE("month creates categories with budgets")
{
    budget::core::month month{test_period()};

    const auto result = month.create_category("Housing", 1200);
    REQUIRE(result.has_value());
    const auto id = *result;
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

TEST_CASE("month rejects duplicate category names")
{
    budget::core::month month{test_period()};

    const auto first_result = month.create_category("Housing", 1200);
    const auto duplicate_result = month.create_category("Housing", 1500);

    REQUIRE(first_result.has_value());
    REQUIRE(!duplicate_result.has_value());
    CHECK(duplicate_result.error() == budget::core::category_error::duplicate_name);
    CHECK(month.categories().size() == 1);
    CHECK(month.category_budgets().size() == 1);
}

TEST_CASE("month refuses to remove a category with dependents unless forced")
{
    budget::core::month month{test_period()};
    const auto category_result = month.create_category("Housing", 1200);
    REQUIRE(category_result.has_value());
    const auto category_id = *category_result;
    const auto item_id = month.create_item("Rent", category_id, 1000);
    std::ignore = month.create_transaction("Rent payment", budget::core::transaction_type::expense, 1000, test_date(),
                                           category_id, item_id);

    const auto result = month.remove_category(category_id);

    REQUIRE(!result.has_value());
    CHECK(result.error() == budget::core::category_error::in_use);
    CHECK(month.categories().size() == 1);
    CHECK(month.category_budgets().size() == 1);
    CHECK(month.items().size() == 1);
    CHECK(month.transactions()[0].category_id == category_id);
    CHECK(month.transactions()[0].item_id == item_id);
}

TEST_CASE("forced category removal deletes items and preserves unlinked transactions")
{
    budget::core::month month{test_period()};
    const auto category_result = month.create_category("Housing", 1200);
    REQUIRE(category_result.has_value());
    const auto category_id = *category_result;
    const auto item_id = month.create_item("Rent", category_id, 1000);
    std::ignore = month.create_transaction("Rent payment", budget::core::transaction_type::expense, 1000, test_date(),
                                           category_id, item_id);

    const auto result = month.remove_category(category_id, true);

    REQUIRE(result.has_value());
    CHECK(month.categories().empty());
    CHECK(month.category_budgets().empty());
    CHECK(month.items().empty());
    const auto transactions = month.transactions();
    REQUIRE(transactions.size() == 1);
    CHECK_FALSE(transactions[0].category_id.has_value());
    CHECK_FALSE(transactions[0].item_id.has_value());
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

    const auto id = month.create_transaction("Payday", budget::core::transaction_type::income, 2500, test_date(), 3, 9);
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
    CHECK(transactions[0].timestamp.day() == std::chrono::day{15});
}
