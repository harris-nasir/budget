#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

import budget.application;
import budget.core;

#include <chrono>
#include <tuple>

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

TEST_CASE("list_months returns no months for an empty book")
{
    fake_book_repository repository;

    const auto response = budget::application::list_months(repository);

    REQUIRE(response.has_value());
    CHECK(response->empty());
    CHECK(repository.save_count == 0);
}

TEST_CASE("list_months returns every stored month in order without a filter")
{
    using namespace std::chrono;
    const auto first_month = year{2026} / October;
    const auto second_month = year{2026} / November;

    fake_book_repository repository;
    std::ignore = repository.stored_book.create_budget(first_month);
    std::ignore = repository.stored_book.create_budget(second_month);

    const auto response = budget::application::list_months(repository);

    REQUIRE(response.has_value());
    const auto &months = *response;
    REQUIRE(months.size() == 2);
    CHECK(months[0].period().year_month == first_month);
    CHECK(months[1].period().year_month == second_month);
    CHECK(repository.save_count == 0);
}

TEST_CASE("list_months returns the requested month and later months when only from is provided")
{
    using namespace std::chrono;
    const auto first_month = year{2026} / October;
    const auto second_month = year{2026} / November;
    const auto third_month = year{2026} / December;

    fake_book_repository repository;
    std::ignore = repository.stored_book.create_budget(first_month);
    std::ignore = repository.stored_book.create_budget(second_month);
    std::ignore = repository.stored_book.create_budget(third_month);

    const auto response = budget::application::list_months(repository, {.from = second_month});

    REQUIRE(response.has_value());
    const auto &months = *response;
    REQUIRE(months.size() == 2);
    CHECK(months[0].period().year_month == second_month);
    CHECK(months[1].period().year_month == third_month);
    CHECK(repository.save_count == 0);
}

TEST_CASE("list_months returns earlier months through to when only to is provided")
{
    using namespace std::chrono;
    const auto first_month = year{2026} / October;
    const auto second_month = year{2026} / November;
    const auto third_month = year{2026} / December;

    fake_book_repository repository;
    std::ignore = repository.stored_book.create_budget(first_month);
    std::ignore = repository.stored_book.create_budget(second_month);
    std::ignore = repository.stored_book.create_budget(third_month);

    const auto response = budget::application::list_months(repository, {.to = second_month});

    REQUIRE(response.has_value());
    const auto &months = *response;
    REQUIRE(months.size() == 2);
    CHECK(months[0].period().year_month == first_month);
    CHECK(months[1].period().year_month == second_month);
    CHECK(repository.save_count == 0);
}

TEST_CASE("list_months includes both endpoints of a requested range")
{
    using namespace std::chrono;
    const auto first_month = year{2026} / October;
    const auto second_month = year{2026} / November;
    const auto third_month = year{2026} / December;

    fake_book_repository repository;
    std::ignore = repository.stored_book.create_budget(first_month);
    std::ignore = repository.stored_book.create_budget(second_month);
    std::ignore = repository.stored_book.create_budget(third_month);

    const auto response = budget::application::list_months(repository, {.from = first_month, .to = second_month});

    REQUIRE(response.has_value());
    const auto &months = *response;
    REQUIRE(months.size() == 2);
    CHECK(months[0].period().year_month == first_month);
    CHECK(months[1].period().year_month == second_month);
    CHECK(repository.save_count == 0);
}

TEST_CASE("list_months rejects a range whose from month follows to")
{
    using namespace std::chrono;
    fake_book_repository repository;

    const auto response = budget::application::list_months(
        repository, {.from = year{2026} / November, .to = year{2026} / October});

    REQUIRE(!response.has_value());
    CHECK(response.error() == budget::application::list_months_error::from_after_to);
    CHECK(repository.load_count == 0);
}

TEST_CASE("create_month creates and saves a requested month")
{
    using namespace std::chrono;
    const auto requested_month = year{2026} / October;

    fake_book_repository repository;

    const auto result = budget::application::create_month(repository, {.year_month = requested_month});

    CHECK(result == budget::application::create_month_response::created);
    CHECK(repository.stored_book.contains_budget(requested_month));
    CHECK(repository.save_count == 1);
}

TEST_CASE("create_month does not save an existing month without force")
{
    using namespace std::chrono;
    const auto requested_month = year{2026} / October;

    fake_book_repository repository;
    std::ignore = repository.stored_book.create_budget(requested_month);
    CHECK(repository.stored_book.contains_budget(requested_month));

    const auto result = budget::application::create_month(repository, {.year_month = requested_month});

    CHECK(result == budget::application::create_month_response::already_exists);
    CHECK(repository.save_count == 0);
}

TEST_CASE("create_month resets an existing month when force is set")
{
    using namespace std::chrono;
    const auto requested_month = year{2026} / October;

    fake_book_repository repository;
    std::ignore = repository.stored_book.create_budget(requested_month);

    const auto result = budget::application::create_month(repository, {.year_month = requested_month, .force = true});

    CHECK(result == budget::application::create_month_response::reset);
    CHECK(repository.stored_book.contains_budget(requested_month));
    CHECK(repository.save_count == 1);
}

TEST_CASE("create_month rejects a missing month")
{
    fake_book_repository repository;

    const auto result = budget::application::create_month(repository, {});

    CHECK(result == budget::application::create_month_response::missing_month);
    CHECK(repository.save_count == 0);
}
