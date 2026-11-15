import budget.application;
import budget.cli;
import budget.core;

namespace
{
class in_memory_book_repository final : public budget::application::book_repository
{
  public:
    auto load() -> budget::core::book override
    {
        return book_;
    }

    void save(const budget::core::book &book) override
    {
        book_ = book;
    }

  private:
    budget::core::book book_;
};
} // namespace

auto main(int argc, char *argv[]) -> int
{
    in_memory_book_repository repository;
    return budget::cli::run(argc, argv, repository);
}
