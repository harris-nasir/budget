#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import budget.application;
import budget.core;

#include <chrono>

namespace
{
struct fake_book_repository final : budget::application::book_repository
{
    budget::core::book stored_book;
    int load_count{};
    int save_count{};

    auto load() -> budget::core::book override
    {
        ++load_count;
        return stored_book;
    }

    void save(const budget::core::book &book) override
    {
        stored_book = book;
        ++save_count;
    }
};
} // namespace

TEST_CASE("create_category creates a category in the requested month")
{
    using namespace std::chrono;
    const auto requested_month = year{2026} / October;
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month);

    const auto response = budget::application::create_category(
        repository, {.year_month = requested_month, .name = "Housing", .allocated = 1200});

    REQUIRE(response.has_value());
    CHECK(*response == 1);
    CHECK(repository.save_count == 1);

    const auto categories = budget::application::list_categories(repository, {.year_month = requested_month});
    REQUIRE(categories.has_value());
    REQUIRE(categories->categories.size() == 1);
    REQUIRE(categories->budgets.size() == 1);
    CHECK(categories->items.empty());
    CHECK(categories->categories[0].name == "Housing");
    CHECK(categories->budgets[0].category_id == *response);
    CHECK(categories->budgets[0].allocated == 1200);
    CHECK(repository.load_count == 2);
}

TEST_CASE("create_category rejects duplicate names in the same month")
{
    using namespace std::chrono;
    const auto requested_month = year{2026} / October;
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month);
    const auto first_response = budget::application::create_category(
        repository, {.year_month = requested_month, .name = "Housing"});
    REQUIRE(first_response.has_value());

    const auto duplicate_response = budget::application::create_category(
        repository, {.year_month = requested_month, .name = "Housing"});

    REQUIRE(!duplicate_response.has_value());
    CHECK(duplicate_response.error() == budget::application::create_category_error::duplicate_name);
    CHECK(repository.save_count == 1);
}

TEST_CASE("create_category allows the same name in separate months")
{
    using namespace std::chrono;
    const auto first_month = year{2026} / October;
    const auto second_month = year{2026} / November;
    fake_book_repository repository;
    repository.stored_book.create_budget(first_month);
    repository.stored_book.create_budget(second_month);

    const auto first_response = budget::application::create_category(
        repository, {.year_month = first_month, .name = "Housing"});
    const auto second_response = budget::application::create_category(
        repository, {.year_month = second_month, .name = "Housing"});

    REQUIRE(first_response.has_value());
    REQUIRE(second_response.has_value());
    CHECK(*first_response == 1);
    CHECK(*second_response == 1);
    CHECK(repository.save_count == 2);
}

TEST_CASE("list_categories distinguishes an empty month from a missing month")
{
    using namespace std::chrono;
    const auto requested_month = year{2026} / October;
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month);

    const auto empty_month = budget::application::list_categories(repository, {.year_month = requested_month});
    const auto missing_month = budget::application::list_categories(
        repository, {.year_month = year{2026} / November});

    REQUIRE(empty_month.has_value());
    CHECK(empty_month->categories.empty());
    CHECK(empty_month->budgets.empty());
    CHECK(empty_month->items.empty());
    REQUIRE(!missing_month.has_value());
    CHECK(missing_month.error() == budget::application::list_categories_error::month_not_found);
    CHECK(repository.save_count == 0);
}

TEST_CASE("remove_category removes a category and saves the book")
{
    using namespace std::chrono;
    const auto requested_month = year{2026} / October;
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month);
    const auto create_response = budget::application::create_category(
        repository, {.year_month = requested_month, .name = "Housing"});
    REQUIRE(create_response.has_value());

    const auto remove_response = budget::application::remove_category(
        repository, {.year_month = requested_month, .category_id = *create_response});

    REQUIRE(remove_response.has_value());
    CHECK(repository.save_count == 2);
    const auto categories = budget::application::list_categories(repository, {.year_month = requested_month});
    REQUIRE(categories.has_value());
    CHECK(categories->categories.empty());
    CHECK(categories->budgets.empty());
    CHECK(categories->items.empty());
}
