#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import budget.application;
import budget.core;

#include <chrono>
#include <optional>

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

auto requested_month() -> std::chrono::year_month
{
    using namespace std::chrono;
    return year{2026} / October;
}

auto category_id_in(fake_book_repository &repository) -> budget::core::category_id
{
    const auto result = repository.stored_book.create_category(requested_month(), "Housing");
    REQUIRE(result.has_value());
    return *result;
}
} // namespace

TEST_CASE("create_item persists a categorized item")
{
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month());
    const auto category_id = category_id_in(repository);

    const auto response = budget::application::create_item(
        repository, {.year_month = requested_month(), .name = "Rent", .category_id = category_id, .expected = 1200});

    REQUIRE(response.has_value());
    CHECK(*response == 1);
    CHECK(repository.load_count == 1);
    CHECK(repository.save_count == 1);
    const auto items = repository.stored_book.list_items(requested_month());
    REQUIRE(items.has_value());
    REQUIRE(items->size() == 1);
    CHECK(items->front().category_id == category_id);
    CHECK(items->front().expected == 1200);

    const auto category_listing = budget::application::list_categories(repository, {.year_month = requested_month()});
    REQUIRE(category_listing.has_value());
    REQUIRE(category_listing->items.size() == 1);
    REQUIRE(category_listing->budgets.size() == 1);
    CHECK(category_listing->items.front().id == *response);
    CHECK(category_listing->items.front().expected == 1200);
    CHECK(category_listing->budgets.front().allocated == 0);
}

TEST_CASE("create_item persists an uncategorized item")
{
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month());

    const auto response = budget::application::create_item(
        repository, {.year_month = requested_month(), .name = "Gift", .category_id = std::nullopt, .expected = 200});

    REQUIRE(response.has_value());
    const auto items = repository.stored_book.list_items(requested_month());
    REQUIRE(items.has_value());
    REQUIRE(items->size() == 1);
    CHECK_FALSE(items->front().category_id.has_value());
    CHECK(items->front().expected == 200);
}

TEST_CASE("create_item does not save invalid requests")
{
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month());
    const auto category_id = category_id_in(repository);

    const auto empty_name = budget::application::create_item(
        repository, {.year_month = requested_month(), .name = " \t", .category_id = category_id, .expected = 10});
    const auto missing_category = budget::application::create_item(
        repository, {.year_month = requested_month(), .name = "Rent", .category_id = 99, .expected = 10});

    REQUIRE(!empty_name.has_value());
    CHECK(empty_name.error() == budget::application::create_item_error::invalid_name);
    REQUIRE(!missing_category.has_value());
    CHECK(missing_category.error() == budget::application::create_item_error::category_not_found);
    CHECK(repository.save_count == 0);
}

TEST_CASE("create_item rejects duplicate names in the same category but allows another category")
{
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month());
    const auto first_category_id = category_id_in(repository);
    const auto second_category_result = repository.stored_book.create_category(requested_month(), "Travel");
    REQUIRE(second_category_result.has_value());

    const auto first_response = budget::application::create_item(
        repository,
        {.year_month = requested_month(), .name = "Rent", .category_id = first_category_id, .expected = 1200});
    const auto duplicate_response = budget::application::create_item(
        repository,
        {.year_month = requested_month(), .name = "Rent", .category_id = first_category_id, .expected = 1300});
    const auto other_category_response = budget::application::create_item(
        repository,
        {.year_month = requested_month(), .name = "Rent", .category_id = *second_category_result, .expected = 100});

    REQUIRE(first_response.has_value());
    REQUIRE(!duplicate_response.has_value());
    CHECK(duplicate_response.error() == budget::application::create_item_error::duplicate_name);
    CHECK(other_category_response.has_value());
    CHECK(repository.save_count == 2);
}

TEST_CASE("list_items supports optional category filtering without saving")
{
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month());
    const auto category_id = category_id_in(repository);
    REQUIRE(
        budget::application::create_item(
            repository, {.year_month = requested_month(), .name = "Rent", .category_id = category_id, .expected = 1200})
            .has_value());
    REQUIRE(
        budget::application::create_item(
            repository, {.year_month = requested_month(), .name = "Gift", .category_id = std::nullopt, .expected = 200})
            .has_value());
    const auto loads_before_list = repository.load_count;
    const auto saves_before_list = repository.save_count;

    const auto all_items = budget::application::list_items(repository, {.year_month = requested_month()});
    const auto category_items =
        budget::application::list_items(repository, {.year_month = requested_month(), .category_id = category_id});

    REQUIRE(all_items.has_value());
    REQUIRE(category_items.has_value());
    CHECK(all_items->size() == 2);
    REQUIRE(category_items->size() == 1);
    CHECK(category_items->front().name == "Rent");
    CHECK(repository.load_count == loads_before_list + 2);
    CHECK(repository.save_count == saves_before_list);
}

TEST_CASE("list_items reports a missing month")
{
    fake_book_repository repository;

    const auto response = budget::application::list_items(repository, {.year_month = requested_month()});

    REQUIRE(!response.has_value());
    CHECK(response.error() == budget::application::list_items_error::month_not_found);
    CHECK(repository.load_count == 1);
    CHECK(repository.save_count == 0);
}

TEST_CASE("remove_item saves successful removals and does not save missing items")
{
    fake_book_repository repository;
    repository.stored_book.create_budget(requested_month());
    const auto category_id = category_id_in(repository);
    const auto item_result = repository.stored_book.create_item(requested_month(), "Rent", category_id, 1200);
    REQUIRE(item_result.has_value());

    const auto response =
        budget::application::remove_item(repository, {.year_month = requested_month(), .item_id = *item_result});

    REQUIRE(response.has_value());
    CHECK(repository.save_count == 1);
    const auto items = repository.stored_book.list_items(requested_month());
    REQUIRE(items.has_value());
    CHECK(items->empty());

    const auto missing_response =
        budget::application::remove_item(repository, {.year_month = requested_month(), .item_id = *item_result});
    REQUIRE(!missing_response.has_value());
    CHECK(missing_response.error() == budget::application::remove_item_error::not_found);
    CHECK(repository.save_count == 1);
}
