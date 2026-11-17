#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <string>
#include <utility>
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
        return stored_book;
    }

    void save(const budget::core::book &book) override
    {
        stored_book = book;
        ++save_count;
    }

    budget::core::book stored_book;
    int load_count{};
    int save_count{};
};

auto run_cli(std::vector<std::string> owned_arguments, fake_book_repository &repository) -> int
{
    std::vector<char *> arguments;
    std::ranges::transform(owned_arguments, std::back_inserter(arguments),
                           [](auto &argument) { return argument.data(); });
    return budget::cli::run(static_cast<int>(arguments.size()), arguments.data(), repository);
}

auto create_month(fake_book_repository &repository, std::string month) -> int
{
    return run_cli({"budget", "month", "new", std::move(month)}, repository);
}

auto create_category(fake_book_repository &repository, std::string name, std::string month, std::string allocated = {})
    -> int
{
    auto arguments =
        std::vector<std::string>{"budget", "category", "create", std::move(name), "--month", std::move(month)};
    if (!allocated.empty())
    {
        arguments.emplace_back("--allocated");
        arguments.push_back(std::move(allocated));
    }
    return run_cli(std::move(arguments), repository);
}
} // namespace

TEST_CASE("category create creates a category with its allocated amount")
{
    fake_book_repository repository;
    REQUIRE(create_month(repository, "2026-10") == 0);
    const auto saves_before_create = repository.save_count;

    CHECK(create_category(repository, "Housing", "2026-10", "1200") == 0);

    const auto categories = repository.stored_book.list_categories(std::chrono::year{2026} / std::chrono::October);
    const auto budgets = repository.stored_book.list_category_budgets(std::chrono::year{2026} / std::chrono::October);
    REQUIRE(categories.has_value());
    REQUIRE(budgets.has_value());
    REQUIRE(categories->size() == 1);
    REQUIRE(budgets->size() == 1);
    CHECK(categories->front().name == "Housing");
    CHECK(budgets->front().category_id == categories->front().id);
    CHECK(budgets->front().allocated == 1200);
    CHECK(repository.save_count == saves_before_create + 1);
}

TEST_CASE("category create rejects empty names without saving")
{
    fake_book_repository repository;
    REQUIRE(create_month(repository, "2026-10") == 0);
    const auto saves_before_create = repository.save_count;

    CHECK(create_category(repository, "   ", "2026-10") == 2);
    CHECK(repository.save_count == saves_before_create);
}

TEST_CASE("category create rejects duplicate names without saving")
{
    fake_book_repository repository;
    REQUIRE(create_month(repository, "2026-10") == 0);
    REQUIRE(create_category(repository, "Housing", "2026-10") == 0);
    const auto saves_before_duplicate = repository.save_count;

    CHECK(create_category(repository, "Housing", "2026-10") == 1);
    CHECK(repository.save_count == saves_before_duplicate);
}

TEST_CASE("category create rejects malformed months without loading the book")
{
    fake_book_repository repository;

    CHECK(create_category(repository, "Housing", "2026-1") == 2);
    CHECK(repository.load_count == 0);
    CHECK(repository.save_count == 0);
}

TEST_CASE("category list handles an empty month")
{
    fake_book_repository repository;
    REQUIRE(create_month(repository, "2026-10") == 0);
    const auto saves_before_list = repository.save_count;

    CHECK(run_cli({"budget", "category", "list", "--month", "2026-10"}, repository) == 0);
    CHECK(repository.load_count == 2);
    CHECK(repository.save_count == saves_before_list);
}

TEST_CASE("category list loads categories without saving")
{
    fake_book_repository repository;
    REQUIRE(create_month(repository, "2026-10") == 0);
    REQUIRE(create_category(repository, "Housing", "2026-10", "1200") == 0);
    const auto loads_before_list = repository.load_count;
    const auto saves_before_list = repository.save_count;

    CHECK(run_cli({"budget", "category", "list", "--month", "2026-10"}, repository) == 0);
    CHECK(repository.load_count == loads_before_list + 1);
    CHECK(repository.save_count == saves_before_list);
}

TEST_CASE("category list rejects a missing month")
{
    fake_book_repository repository;

    CHECK(run_cli({"budget", "category", "list", "--month", "2026-10"}, repository) == 1);
    CHECK(repository.load_count == 1);
    CHECK(repository.save_count == 0);
}

TEST_CASE("category remove deletes a category from the requested month")
{
    fake_book_repository repository;
    REQUIRE(create_month(repository, "2026-10") == 0);
    REQUIRE(create_category(repository, "Housing", "2026-10") == 0);
    const auto saves_before_remove = repository.save_count;

    CHECK(run_cli({"budget", "category", "remove", "1", "--month", "2026-10"}, repository) == 0);

    const auto categories = repository.stored_book.list_categories(std::chrono::year{2026} / std::chrono::October);
    const auto budgets = repository.stored_book.list_category_budgets(std::chrono::year{2026} / std::chrono::October);
    REQUIRE(categories.has_value());
    REQUIRE(budgets.has_value());
    CHECK(categories->empty());
    CHECK(budgets->empty());
    CHECK(repository.save_count == saves_before_remove + 1);
}
