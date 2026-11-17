#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import budget.core;

#include <chrono>
#include <optional>
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

TEST_CASE("month rejects empty and whitespace-only category names")
{
    budget::core::month month{test_period()};

    const auto empty_result = month.create_category("");
    const auto whitespace_result = month.create_category(" \t\r\n");

    REQUIRE(!empty_result.has_value());
    CHECK(empty_result.error() == budget::core::category_error::invalid_name);
    REQUIRE(!whitespace_result.has_value());
    CHECK(whitespace_result.error() == budget::core::category_error::invalid_name);
    CHECK(month.categories().empty());
    CHECK(month.category_budgets().empty());
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
    const auto item_result = month.create_item("Rent", category_id, 1000);
    REQUIRE(item_result.has_value());
    const auto item_id = *item_result;
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
    const auto item_result = month.create_item("Rent", category_id, 1000);
    REQUIRE(item_result.has_value());
    const auto item_id = *item_result;
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

TEST_CASE("month creates categorized and uncategorized items")
{
    budget::core::month month{test_period()};
    const auto category_result = month.create_category("Housing");
    REQUIRE(category_result.has_value());

    const auto categorized_result = month.create_item("Rent", *category_result, 1000);
    const auto uncategorized_result = month.create_item("Gift", std::nullopt, 200);
    const auto items = month.items();

    REQUIRE(categorized_result.has_value());
    REQUIRE(uncategorized_result.has_value());
    REQUIRE(items.size() == 2);
    CHECK(items[0].id == *categorized_result);
    CHECK(items[0].category_id == *category_result);
    CHECK(items[0].name == "Rent");
    CHECK(items[0].expected == 1000);
    CHECK_FALSE(items[1].category_id.has_value());
    CHECK(items[1].name == "Gift");
    CHECK(items[1].expected == 200);
}

TEST_CASE("month validates item names and category assignments")
{
    budget::core::month month{test_period()};
    const auto empty_name = month.create_item("", std::nullopt, 1);
    const auto whitespace_name = month.create_item(" \t", std::nullopt, 1);
    const auto missing_category = month.create_item("Rent", 42, 1000);

    REQUIRE(!empty_name.has_value());
    CHECK(empty_name.error() == budget::core::item_error::invalid_name);
    REQUIRE(!whitespace_name.has_value());
    CHECK(whitespace_name.error() == budget::core::item_error::invalid_name);
    REQUIRE(!missing_category.has_value());
    CHECK(missing_category.error() == budget::core::item_error::category_not_found);
    CHECK(month.items().empty());
}

TEST_CASE("month rejects duplicate item names within the same category assignment")
{
    budget::core::month month{test_period()};
    const auto housing_result = month.create_category("Housing");
    const auto travel_result = month.create_category("Travel");
    REQUIRE(housing_result.has_value());
    REQUIRE(travel_result.has_value());

    const auto first_housing_item = month.create_item("Rent", *housing_result, 1000);
    const auto duplicate_housing_item = month.create_item("Rent", *housing_result, 1200);
    const auto travel_item = month.create_item("Rent", *travel_result, 1000);
    const auto uncategorized_item = month.create_item("Rent", std::nullopt, 300);
    const auto duplicate_uncategorized_item = month.create_item("Rent", std::nullopt, 400);

    REQUIRE(first_housing_item.has_value());
    REQUIRE(!duplicate_housing_item.has_value());
    CHECK(duplicate_housing_item.error() == budget::core::item_error::duplicate_name);
    CHECK(travel_item.has_value());
    CHECK(uncategorized_item.has_value());
    REQUIRE(!duplicate_uncategorized_item.has_value());
    CHECK(duplicate_uncategorized_item.error() == budget::core::item_error::duplicate_name);
    CHECK(month.items().size() == 3);
}

TEST_CASE("month filters items by category")
{
    budget::core::month month{test_period()};
    const auto housing_result = month.create_category("Housing");
    const auto travel_result = month.create_category("Travel");
    REQUIRE(housing_result.has_value());
    REQUIRE(travel_result.has_value());
    REQUIRE(month.create_item("Rent", *housing_result, 1000).has_value());
    REQUIRE(month.create_item("Train", *travel_result, 100).has_value());
    REQUIRE(month.create_item("Gift", std::nullopt, 50).has_value());

    const auto housing_items = month.items(*housing_result);

    REQUIRE(housing_items.size() == 1);
    CHECK(housing_items[0].name == "Rent");
}

TEST_CASE("month requires force to remove an item referenced by a transaction")
{
    budget::core::month month{test_period()};
    const auto category_result = month.create_category("Housing");
    REQUIRE(category_result.has_value());
    const auto item_result = month.create_item("Rent", *category_result, 1000);
    REQUIRE(item_result.has_value());
    const auto item_id = *item_result;
    std::ignore = month.create_transaction("Rent payment", budget::core::transaction_type::expense, 1000, test_date(),
                                           *category_result, item_id);

    const auto refused_result = month.remove_item(item_id);

    REQUIRE(!refused_result.has_value());
    CHECK(refused_result.error() == budget::core::item_error::in_use);
    REQUIRE(month.items().size() == 1);
    const auto forced_result = month.remove_item(item_id, true);
    const auto transactions = month.transactions();

    REQUIRE(forced_result.has_value());
    CHECK(month.items().empty());
    REQUIRE(transactions.size() == 1);
    CHECK(transactions[0].category_id == *category_result);
    CHECK_FALSE(transactions[0].item_id.has_value());
}

TEST_CASE("month reports missing items during removal")
{
    budget::core::month month{test_period()};

    const auto result = month.remove_item(1);

    REQUIRE(!result.has_value());
    CHECK(result.error() == budget::core::item_error::not_found);
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
