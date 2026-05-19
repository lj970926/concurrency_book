# Repository Guidelines

## Project Structure & Module Organization

This is a compact C++17 concurrency study project built with CMake.

- `include/`: public headers for the examples and library types, such as `spin_mutex.h`, `lock_free_stack.h`, and hazard-pointer support.
- `src/`: library implementation files. These are compiled into the shared `concurrency_book` library.
- `tests/`: executable, assert-based tests. `tests/CMakeLists.txt` auto-discovers `*.cpp`, `*.cc`, and `*.cxx` files and registers each executable with CTest.
- `scripts/`: standalone memory-ordering examples, built as separate executables.
- `LICENSES/`, `.reuse/`, and `REUSE.toml`: REUSE licensing metadata.

## Build, Test, and Development Commands

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Use these to configure, build the library/tests/scripts, and run all registered tests. To skip tests during configuration:

```bash
cmake -S . -B build -DCONCURRENCY_BOOK_BUILD_TESTS=OFF
```

After building, scripts can be run directly, for example `./build/scripts/seq_cst_order`.

## Coding Style & Naming Conventions

Use C++17 and keep code small, explicit, and focused on concurrency behavior. Follow the existing style: 4-space indentation, braces on the same line for functions and control flow, `snake_case` for functions and local variables, and `CamelCase` for types such as `SpinMutex`. Private data members use a trailing underscore, for example `flag_`.

Prefer standard-library concurrency primitives and atomics. Document non-obvious memory-ordering choices in short comments.

Every source, header, script, and CMake file should include SPDX license metadata consistent with nearby files.

## Testing Guidelines

Tests use simple executables with `assert`, not an external test framework. Add new tests under `tests/` with names ending in `_test.cpp`; CMake will create a matching executable and CTest entry. Keep tests deterministic where possible, but include contention tests when they verify synchronization behavior.

Run `ctest --test-dir build --output-on-failure` before submitting changes.

When you need Python, run `source ~/.venv/bin/activate` first.

## Commit & Pull Request Guidelines

Recent history uses Conventional Commit-style subjects, mainly `feat:` and `fix:`. Keep commit messages imperative and specific, for example `feat: add hazard-pointer stack test` or `fix: correct acquire-release example`.

When Codex creates a commit, include `Co-authored-by: Codex <codex@openai.com>` in the commit message footer.
When Claude Code creates a commit, include `Co-authored-by: Claude <noreply@anthropic.com>` in the footer.

Pull requests should include a short description of the concurrency behavior changed, the tests run, and any important memory-ordering or reclamation assumptions. Link related issues when available. For changes to examples, mention the executable in `scripts/` that demonstrates the behavior.

## Agent-Specific Notes

Do not rewrite unrelated examples or formatting while changing concurrency code. Preserve the learning-oriented structure and avoid large frameworks unless the repository already depends on them.
