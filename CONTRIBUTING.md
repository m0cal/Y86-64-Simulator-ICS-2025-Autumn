# Contributing to Project

## How to Contribute

To contribute, follow these steps:

* Set up development environment.
* Create a new branch.
* Make changes on your branch.
* Submit a pull request with clear description.
* Ask others to review the PR. Once approved, merge it.

### Setting Up Development Environment

C++ Version: C++23

We use Meson and Ninja to build the project, and Catch2 for unit testing.

1.  **Install Meson and Ninja:**
    You can install Meson and Ninja using your system's package manager. 

    Please refer to the official installation guides for [Meson](https://mesonbuild.com/Getting-meson.html) and [Ninja](https://ninja-build.org/).

2.  **Install Dependencies:**
    Our project uses Catch2 v3 for testing. We manage dependencies using Meson's wrap feature. When you run `meson setup`, it will automatically download and configure Catch2 v3.
    **Note:** Catch2 v3 is expected. Please refer to [Catch2 v3 documentation](https://github.com/catchorg/Catch2/blob/devel/docs/Readme.md) and use v3 patterns. If you are familiar with Catch2 v2, be aware that there are breaking changes in v3.

3.  **Configure and Build:**
    To configure the project, run Meson from the root directory:
    ```bash
    meson setup build
    ```
    This will create a `build` directory and configure the project. To compile the project, use Ninja:
    ```bash
    ninja -C build
    ```

4.  **Run Tests:**
    After building, you can run the tests using:
    ```bash
    ninja -C build test
    ```

### Commit Messages & Branch Names

Follow the conventional commit and branch naming format when possible:

#### Commits:

* feat: Add new program parser
* fix: Prevent null register pointer dereference
* docs: Update README with parser details

#### Branches:

* feat/add-new-parser
* fix/prevent-null-register-pointer-dereference
* docs/update-README-with-parser-details

### Coding Standard

#### Naming Conventions

| Element                     | Recommended Style                      | Example                              |
| --------------------------- | -------------------------------------- | ------------------------------------ |
| **Functions**               | `snake_case` (lowercase + underscores) | `compute_total()`, `open_file()`     |
| **Variables**               | `snake_case`                           | `total_count`, `file_path`           |
| **Classes / Structs**       | `PascalCase` (capitalize each word)    | `FileReader`, `DataProcessor`        |
| **Constants / Macros**      | `ALL_CAPS` or `kCamelCase`             | `MAX_BUFFER_SIZE`, `kDefaultTimeout` |
| **Namespaces / File Names** | lowercase + underscores                | `my_project`, `file_reader.h`        |

#### Indentation & Braces

| Style           | Recommendation                                                                            |
| --------------- | ----------------------------------------------------------------------------------------- |
| **Indentation** | 4 spaces per level                                                                        |
| **Braces**      | K&R style: opening brace on the same line as the statement; closing brace on its own line |

#### File Extensions

* Header: `.h`
* Source: `.cc`


### Unit Test

To ensure correctness of the project, new features must be accompanied by unit tests.

We use [Catch2](https://github.com/catchorg/Catch2) for our unit tests, managed and run via Meson. All test files should be placed in the `dev_tests/` directory.

#### Write Tests for Your Pull Request

For example, if you have added `calculator.cc` and `calculator.h` in `src/`, they might look like this:

`src/calculator.cc:`

```cpp
#include "calculator.h"

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int multiply(int a, int b) {
    return a * b;
}
```
`src/calculator.h:`
```cpp
#pragma once

int add(int a, int b);
int subtract(int a, int b);
int multiply(int a, int b);
```

Next, write unit tests for these functions in `dev_tests/test_calculator.cc`. Note that you **do not** need to define `CATCH_CONFIG_MAIN` as we link against the Catch2 main library.

`dev_tests/test_calculator.cc:`

```cpp
#include <catch2/catch_test_macros.hpp>
#include "calculator.h"

TEST_CASE("Addition works", "[calculator]") {
    REQUIRE(add(2, 3) == 5);
}

TEST_CASE("Subtraction works", "[calculator]") {
    REQUIRE(subtract(10, 4) == 6);
}

TEST_CASE("Multiplication works", "[calculator]") {
    REQUIRE(multiply(3, 4) == 12);
}
```

#### Register Your Files in `meson.build`

You need to register your new source files and test files in `meson.build` so they are compiled and linked correctly.

1.  Add your source file (e.g., `src/calculator.cc`) to `core_sources`:

    ```meson
    core_sources = files(
      # ... existing files ...
      'src/calculator.cc',
    )
    ```

2.  Add your test file (e.g., `dev_tests/test_calculator.cc`) to `test_files`:

    ```meson
    test_files = files(
      'dev_tests/main_test.cc',
      'dev_tests/test_calculator.cc',
    )
    ```

#### Build and Run Tests

Finally, build everything and run tests in the project root directory:

```bash
meson setup build  # Only needed once
ninja -C build
meson test -C build
```

Please make sure all tests for new features are added and all existing tests pass before initiating a pull request.

For more information about Catch2, please refer to its [Tutorial](https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md).
