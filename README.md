# Budget

Budget is a personal budgeting application for organizing monthly budgets, categories, planned items, and transactions.

The project is in early development. The current executable is a prototype that creates sample budget data in code; an interactive user interface and persistent storage are not implemented yet.

## Design

The application is organized so its budgeting rules do not depend on how a person or another program interacts with it. The internal `budget-core` target contains the domain logic and exposes it through the `budget.core` C++ module. Application frontends can use that interface without depending on the core's internal modules. This leaves room for different interfaces, such as CLI, TUI, or GUI, without making the core depend on any of them.

## Build

From the project root, build the application with build2:

```sh
b
```

On Windows, the executable is `budget/budget.exe`.

## Tests

Run the core unit tests with:

```sh
b test
```

The tests use doctest and exercise the `month` and `book` domain behavior through the public `budget.core` module.

## Requirements

- build2 0.18 or later
- A C++ compiler with C++20 module support
- doctest 2.4.11 (resolved as a build dependency)
