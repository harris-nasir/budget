#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <chrono>
#include <string>

import budget.application;
import budget.cli;
import budget.core;

namespace
{
class fake_book_repository final : public budget::application::book_repository
{
  public:
    auto load() -> budget::core::book override
    {
        return book_;
    }

    void save(const budget::core::book &book) override
    {
        book_ = book;
        ++save_count;
    }

    int save_count{};

  private:
    budget::core::book book_;
};

auto run_month_new(std::string date, fake_book_repository &repository) -> int
{
    char program[] = "budget";
    char month_command[] = "month";
    char new_command[] = "new";
    char *arguments[] = {program, month_command, new_command, date.data()};
    return budget::cli::run(4, arguments, repository);
}
} // namespace

TEST_CASE("month new accepts a valid date before 2000")
{
    fake_book_repository repository;

    const auto exit_code = run_month_new("1999-12", repository);

    CHECK(exit_code == 0);
    CHECK(repository.save_count == 1);
    CHECK(repository.load().contains_budget(std::chrono::year{1999} / std::chrono::December));
}

TEST_CASE("month new accepts a valid date and saves it")
{
    fake_book_repository repository;

    const auto exit_code = run_month_new("2000-01", repository);

    CHECK(exit_code == 0);
    CHECK(repository.save_count == 1);
    CHECK(repository.load().contains_budget(std::chrono::year{2000} / std::chrono::January));
}

TEST_CASE("month new rejects a malformed date without saving")
{
    fake_book_repository repository;

    const auto exit_code = run_month_new("2026-1", repository);

    CHECK(exit_code == 2);
    CHECK(repository.save_count == 0);
}
