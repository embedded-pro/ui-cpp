---
description: "UI Toolbox testing: TEST_F, StrictMock only, anonymous-namespace fixtures, no plain TEST(), draw-call assertions via RecordingCanvas, no redundant cases. Canonical: AGENTS.md."
applyTo: "**/test/**"
---

# UI Toolbox Testing Guidelines

## File Structure

- Test files: `ui/{area}/test/Test{ComponentName}.cpp`
- CMake: `ui_add_test()` in `ui/{area}/test/CMakeLists.txt`, reached via `add_subdirectory(test)`

## Framework

- GoogleTest for assertions — **always `TEST_F`**, never the plain `TEST()` macro (cppcheck
  reports `syntaxError` on it)
- GoogleMock (`testing::StrictMock<>`) only when needed
- A Tier 1 test links no Qt and needs no display, so it runs on the macOS and Windows CI jobs where
  Qt is not installed. A `ui/backend/qt` test runs under `QT_QPA_PLATFORM=offscreen`, which its own
  `main` sets.

## What to assert

Painted components are tested through their **draw-call stream**, not through pixels. Paint into a
`RecordingCanvas` and assert command kinds and counts, polyline point counts, label strings and pen
colours by theme role. This is what catches a silent visual regression, and it is only possible
because the rendering goes through an interface.

```cpp
#include "ui/backend/recording/RecordingCanvas.hpp"
#include "ui/charts/ChartCore.hpp"
#include "ui/charts/LinearAxis.hpp"
#include <gmock/gmock.h>

namespace
{
    using ui::backend::recording::CommandKind;

    class ChartCoreTest
        : public ::testing::Test
    {
    protected:
        ui::backend::recording::RecordingCanvas canvas;
        ui::charts::LinearAxis axis{ ui::charts::LinearAxis::Time() };
        ui::charts::ChartCore chart{ axis, ui::charts::ChartConfig{} };

        static constexpr ui::Rect bounds{ 0.0f, 0.0f, 800.0f, 600.0f };
    };
}

TEST_F(ChartCoreTest, TheSeriesIsDrawnAsOnePolyline)
{
    // Arrange, Act, Assert
    EXPECT_EQ(canvas.CountOf(CommandKind::DrawPolyline), 1u);
}
```

Pure logic — axis transforms, interaction arithmetic, geometry, formatting — is asserted directly
with `EXPECT_NEAR` against an independently derived value.

The Qt backend additionally has a **conformance suite**: it runs one scene through both
`RecordingCanvas` and `QtCanvas` and asserts Qt painted ink where the recording says it should be.
Extend it when a new `Canvas` primitive lands.

## Rules

- Fixture class and type aliases go inside an anonymous `namespace {}`; the `TEST_F` macros go
  **outside** it. cppcheck cannot parse a `TEST_F` nested in a namespace and fails the Linting job
  with `syntaxError`, so this is enforced, not stylistic
- Include `<gmock/gmock.h>` when matchers or mocks are needed, `<gtest/gtest.h>` otherwise
- **ONLY `StrictMock`**: never `testing::NiceMock<>` or a bare mock instantiation — `NiceMock`
  silences unexpected-call warnings and masks test gaps; `StrictMock` makes every interaction explicit
- **No redundant tests** — one behaviour per test, not one test per parameter permutation
- **Independent reference** — never assert an implementation against its own output
- Test genuine edge cases (empty data, a single sample, a zero-width plot area) without duplicating
  coverage
- Use descriptive test names that state the property being asserted
- Allman brace style and PascalCase naming apply to test code too

## TDD Approach

- **Clarify requirements first**: define the use cases, inputs, outputs and edge cases as test cases
  before writing code
- **Write tests before implementation**: tests define the expected behaviour; the implementation
  exists to satisfy them
- **Red-Green-Refactor**: write a failing test, make it pass with minimal code, then refactor while
  keeping it green

## Coverage

`UI_ENABLE_COVERAGE` instruments this project's own targets — emil is optional here, so
`EMIL_ENABLE_COVERAGE` does not apply. Nothing in this repository is a template needing explicit
instantiation to be measured.

```sh
cmake --preset coverage && cmake --build --preset coverage && ctest --preset coverage
```
