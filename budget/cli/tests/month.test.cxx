#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

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
        ++load_count;
        return book_;
    }

    void save(const budget::core::book &book) override
    {
        book_ = book;
        ++save_count;
    }

    int load_count{};
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

auto run_month_list(std::string from, std::string to, fake_book_repository &repository) -> int
{
    std::vector<std::string> owned_arguments{"budget", "month", "list"};
    if (!from.empty())
    {
        owned_arguments.emplace_back("--from");
        owned_arguments.push_back(from);
    }
    if (!to.empty())
    {
        owned_arguments.emplace_back("--to");
        owned_arguments.push_back(to);
    }

    std::vector<char *> arguments;
    std::ranges::transform(owned_arguments, std::back_inserter(arguments), [](auto &argument) {
        return argument.data();
    });
    return budget::cli::run(static_cast<int>(arguments.size()), arguments.data(), repository);
}
} // namespace

TEST_CASE("month new accepts a valid date before 2000")
{
    fake_book_repository repository;

    const auto exit_code = run_month_new("1999-12", repository);

    CHECK(exit_code == 0);
    CHECK(repository.save_count == 1);
}

TEST_CASE("month new accepts a valid date and saves it")
{
    fake_book_repository repository;

    const auto exit_code = run_month_new("2000-01", repository);

    CHECK(exit_code == 0);
    CHECK(repository.save_count == 1);
}

TEST_CASE("month new rejects a malformed date without saving")
{
    fake_book_repository repository;

    const auto exit_code = run_month_new("2026-1", repository);

    CHECK(exit_code == 2);
    CHECK(repository.save_count == 0);
}

TEST_CASE("month list accepts optional from and to bounds")
{
    fake_book_repository repository;

    CHECK(run_month_new("2026-10", repository) == 0);
    CHECK(run_month_new("2026-11", repository) == 0);
    CHECK(run_month_new("2026-12", repository) == 0);
    CHECK(repository.save_count == 3);

    const auto loads_before_listing = repository.load_count;
    CHECK(run_month_list("", "", repository) == 0);
    CHECK(run_month_list("2026-11", "", repository) == 0);
    CHECK(run_month_list("", "2026-11", repository) == 0);
    CHECK(run_month_list("2026-10", "2026-11", repository) == 0);
    CHECK(repository.load_count == loads_before_listing + 4);
    CHECK(repository.save_count == 3);
}

TEST_CASE("month list rejects a from month after to without loading the book")
{
    fake_book_repository repository;

    CHECK(run_month_list("2026-12", "2026-10", repository) == 2);
    CHECK(repository.load_count == 0);
}
