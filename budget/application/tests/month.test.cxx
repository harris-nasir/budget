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
    int save_count{};

    auto load() -> budget::core::book override
    {
        return stored_book;
    }

    void save(const budget::core::book &book) override
    {
        stored_book = book;
        ++save_count;
    }
};
} // namespace

TEST_CASE("create_month creates and saves a requested month")
{
    using namespace std::chrono;
    const auto requested_month = year{2026} / October;

    fake_book_repository repository;

    const auto result = budget::application::create_month(repository, {.year_month = requested_month});

    CHECK(result == budget::application::create_month_result::created);
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

    CHECK(result == budget::application::create_month_result::already_exists);
    CHECK(repository.save_count == 0);
}

TEST_CASE("create_month resets an existing month when force is set")
{
    using namespace std::chrono;
    const auto requested_month = year{2026} / October;

    fake_book_repository repository;
    std::ignore = repository.stored_book.create_budget(requested_month);

    const auto result = budget::application::create_month(repository, {.year_month = requested_month, .force = true});

    CHECK(result == budget::application::create_month_result::reset);
    CHECK(repository.stored_book.contains_budget(requested_month));
    CHECK(repository.save_count == 1);
}

TEST_CASE("create_month rejects a missing month")
{
    fake_book_repository repository;

    const auto result = budget::application::create_month(repository, {});

    CHECK(result == budget::application::create_month_result::missing_month);
    CHECK(repository.save_count == 0);
}
