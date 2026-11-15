#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import budget.core;

#include <chrono>

TEST_CASE("book creates budgets with increasing period IDs")
{
    using namespace std::chrono;

    budget::core::book book;

    auto january = book.create_budget(year{2025} / January);
    auto february = book.create_budget(year{2025} / February);

    const auto january_category_id = january.create_category("Rent");
    const auto february_category_id = february.create_category("Food");

    CHECK(january_category_id == 1);
    CHECK(february_category_id == 1);
    CHECK(january.categories()[0].period_id == 1);
    CHECK(february.categories()[0].period_id == 2);
    CHECK(january.categories()[0].name == "Rent");
    CHECK(february.categories()[0].name == "Food");
}

TEST_CASE("separate books assign period IDs independently")
{
    using namespace std::chrono;

    budget::core::book first_book;
    budget::core::book second_book;

    auto first_month = first_book.create_budget(year{2025} / January);
    auto second_month = second_book.create_budget(year{2025} / February);

    const auto first_category_id = first_month.create_category("First");
    const auto second_category_id = second_month.create_category("Second");

    CHECK(first_category_id == 1);
    CHECK(second_category_id == 1);
    CHECK(first_month.categories()[0].period_id == 1);
    CHECK(second_month.categories()[0].period_id == 1);
}
