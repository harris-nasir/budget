#include <chrono>

import budget.core;

auto main() -> int
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    budget::core::book book;
    auto october = book.create_budget(year{2026} / month{October});

    auto utilities = october.create_category("utilities", 30'000);
    auto electricity = october.create_item("electricity", utilities, 12'000);

    october.create_transaction("", budget::core::transaction_type::expense, 5000);
    october.create_transaction("", budget::core::transaction_type::expense, 12'000, electricity);
}
