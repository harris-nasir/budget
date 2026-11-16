#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import budget.core;

#include <chrono>

TEST_CASE("book creates budgets with increasing period IDs and targeted categories")
{
    using namespace std::chrono;

    budget::core::book book;
    const auto january = year{2025} / January;
    const auto february = year{2025} / February;
    book.create_budget(january);
    book.create_budget(february);

    const auto january_category = book.create_category(january, "Rent", 1200);
    const auto february_category = book.create_category(february, "Food", 500);

    REQUIRE(january_category.has_value());
    REQUIRE(february_category.has_value());
    CHECK(*january_category == 1);
    CHECK(*february_category == 1);

    const auto january_categories = book.list_categories(january);
    const auto february_categories = book.list_categories(february);
    const auto january_budgets = book.list_category_budgets(january);
    const auto february_budgets = book.list_category_budgets(february);
    REQUIRE(january_categories.has_value());
    REQUIRE(february_categories.has_value());
    REQUIRE(january_budgets.has_value());
    REQUIRE(february_budgets.has_value());
    REQUIRE(january_categories->size() == 1);
    REQUIRE(february_categories->size() == 1);
    REQUIRE(january_budgets->size() == 1);
    REQUIRE(february_budgets->size() == 1);
    CHECK((*january_categories)[0].name == "Rent");
    CHECK((*january_budgets)[0].category_id == *january_category);
    CHECK((*january_budgets)[0].allocated == 1200);
    CHECK((*february_categories)[0].name == "Food");
    CHECK((*february_budgets)[0].category_id == *february_category);
    CHECK((*february_budgets)[0].allocated == 500);
}

TEST_CASE("separate books assign period IDs independently")
{
    using namespace std::chrono;

    budget::core::book first_book;
    budget::core::book second_book;
    const auto first_month = year{2025} / January;
    const auto second_month = year{2025} / February;
    first_book.create_budget(first_month);
    second_book.create_budget(second_month);

    const auto first_category_id = first_book.create_category(first_month, "First");
    const auto second_category_id = second_book.create_category(second_month, "Second");

    REQUIRE(first_category_id.has_value());
    REQUIRE(second_category_id.has_value());
    CHECK(*first_category_id == 1);
    CHECK(*second_category_id == 1);
}

TEST_CASE("book category operations report missing months")
{
    using namespace std::chrono;
    budget::core::book book;
    const auto requested_month = year{2025} / January;

    const auto create_result = book.create_category(requested_month, "Rent");
    const auto list_result = book.list_categories(requested_month);
    const auto budgets_result = book.list_category_budgets(requested_month);
    const auto remove_result = book.remove_category(requested_month, 1);

    REQUIRE(!create_result.has_value());
    CHECK(create_result.error() == budget::core::category_error::month_not_found);
    REQUIRE(!list_result.has_value());
    CHECK(list_result.error() == budget::core::category_error::month_not_found);
    REQUIRE(!budgets_result.has_value());
    CHECK(budgets_result.error() == budget::core::category_error::month_not_found);
    REQUIRE(!remove_result.has_value());
    CHECK(remove_result.error() == budget::core::category_error::month_not_found);
}
