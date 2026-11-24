# ui-cpp — Agent Rules

<!-- shared:begin 0.5.0 sha256:9aee198041110762 -->
Canonical rules for Claude, Copilot and sub-agents. Sections marked *(shared)* come from embedded-pro/embedded-agent-kit:
edit them there, not here. *(repo)* sections belong to this repository. *Exceptions* overrides a shared rule.

## Behaviour *(shared)*

- **Be as terse as possible.** No preamble, summary or narration. Report file paths and pass/fail.
- Everything you write — code, docs, agent files, PR text — uses the fewest words that stay unambiguous.
- Prose (docs, comments, agent files, commit and PR text) follows ASD-STE100 Simplified Technical English:
  one word, one meaning; active voice; one instruction per sentence; sentences ≤ 20 words (procedures) or ≤ 25 (descriptions).
- Ambiguous request: ask. Clear request: act.
- Do only what was asked. No unrelated refactors, no extra features.
- Batch reads, don't re-read, prefer targeted edits.
- Never change a shared section. Add a deviation to *Exceptions* only when the user approves it.

## Workflow *(shared)*

1. Multi-file or architectural change: update the design document first. Get approval before you write code.
2. Read *Domain* and *Build*. Read each `.github/instructions/*.instructions.md` whose `applyTo` matches a file you change.
3. Find the nearest existing pattern. Follow it.
4. Per behaviour: write a failing test, write the minimum code that passes it, refactor with all tests green.
5. Add each new file to its `CMakeLists.txt`.
6. Update the documentation for each behavioural change.
7. Run the *Build* commands until build and tests pass. Never disable, skip or weaken a test to get green.
8. Non-trivial change: run the `reviewer` agent, or ask the user to run it. Fix each CRITICAL and WARNING finding.
9. Report changed files, build pass/fail, test pass/fail.

## Memory *(shared)*

Scope: production code. Tests and host tools may use the heap.

- Forbidden: `new`/`delete`/`malloc`/`free`, `make_unique`/`make_shared`,
  `std::vector`/`string`/`deque`/`list`/`map`/`set`, recursion.
- Use: `infra::BoundedVector<T>::WithMaxSize<N>`, `infra::BoundedString::WithStorage<N>`,
  `infra::BoundedDeque`, `infra::BoundedList`/`IntrusiveList`, `std::array`, `std::optional`, `std::variant`.

## Interfaces & errors *(shared)*

- No virtual destructors: each adds a deleting destructor and thunks to flash.
  Interface = pure virtual methods + `protected: ~I() = default;` (non-virtual).
- Public `virtual ~Base() = default;` only on a base destroyed through a base pointer:
  `delete`/`std::unique_ptr<Base>`, `infra::PolymorphicVariant<Base, ...>`, `infra::OptionalForPolymorphicObjects<Base, ...>`.
  Put it on that base, not on every interface.
- Not needed for `std::variant`, `std::optional<Concrete>`, `infra::SharedPtr`: they destroy the concrete type.
- No exceptions. `std::optional`, `bool` or status enum; `really_assert` for preconditions. Never swallow an error.
- Constructor injection; no global state.

## Optimisation *(shared)*

- No `#pragma GCC optimize`, no `optimize` attribute: GCC won't inline across differing options.
  The consumer sets flags per translation unit.
- `OPTIMIZE_FOR_SPEED` only on hot functions defined in a header.
- Hot paths: no heap, no virtual dispatch, no blocking.

## Style *(shared)*

- `.clang-format` is authoritative. Allman, 4 spaces, `{}` init.
- PascalCase types/methods; camelCase members and enum values; lowercase namespaces.
  No `I` prefix; implementations end in `Impl`.
- `#pragma once` only.
- Fixed-width integers. `const`/`constexpr`-correct. Functions ≤ 30 lines (hard limit 50).
- Comments: non-obvious *why* only, one line. No `TODO`/`FIXME`, commented-out code, or restating the code.
  Fix or delete a comment the change makes stale.

## Testing *(shared)*

- `TEST_F` for fixtures, `TYPED_TEST` for types, plain `TEST()` for stateless checks.
- `StrictMock` only. `EXPECT_NEAR` with explicit tolerance. Fixture in an anonymous namespace, macros outside.
- Heap allowed in tests.

## Documentation *(shared)*

- Update docs before or with any behavioural change.
- One fact, one place: link, never restate.
- Visuals: Mermaid or ASCII.
<!-- shared:end -->

## Domain *(repo)*

- Shared, backend-agnostic UI components for host tools and GUIs of the `embedded-pro` repositories.
  Consumers: `e-foc`, `e-robot-arm`, `numerical-toolbox-cpp`, `robotics-toolbox-cpp`, `neural-network-toobox-cpp`.
- Source root: `ui/`: `core/`, `theme/`, `model/`, `shell/`, `charts/`, `scope/`, `scene/`, `stage/`, `widgets/`,
  `terminal/`, `backend/qt/`, `backend/recording/`. Examples: `examples/`.
- Documents: tiers and Tier 3 justifications `doc/portability.md`; `Canvas` contract and geometry `doc/canvas.md`;
  3-D stage `doc/scene3d.md`. `scripts/validate-docs.py` checks them.
- Portability tiers. CI enforces them:
  - Tier 1: everything under `ui/` except `ui/backend/qt/`. No Qt. Builds and tests on Linux, macOS and Windows
    with no Qt installed and no display.
  - Tier 2: interface in `ui/shell/` (`FormView`, `RepeatingGroup`, `AppShell`), implemented in `ui/backend/qt/`
    and `ui/backend/recording/`. Do not call the repeating group a table view or give it a `QAbstractItemModel`.
    An alert is two methods on `ShellView`, not a component.
  - Tier 3: Qt only, under `ui/backend/qt/` or in the consumer. Give each a one-line justification in
    `doc/portability.md`.
- Enforced invariants (`guardrails` CI job checks 1–3):
  1. No `<Q...>` include and no `Qt6::` link in a Tier 1 directory.
  2. Qt code lives only under `ui/backend/qt/`.
  3. `setStyleSheet(` only in `ui/backend/qt/QtTheme.cpp`. Elsewhere use `theme::`.
  4. No per-sample virtual call in a `Paint()`: batch into one `DrawPolyline`.
- Tier 1 has zero external dependencies: no emil, no Qt, no fmt. emil helpers are optional
  (`if (COMMAND emil_clangformat_directories)`), with plain CMake otherwise.
- Format text through `ui/core/Format.hpp` (`std::format_to_n` into a caller-owned buffer). In `Paint()` use
  `ui::FormatBuffer`, never `std::format`: it returns a `std::string`.
- No colour, font or margin literal at a call site: `theme::ColorRole`, `theme::FontRole`, `theme::Current().Charts()`.
- A new `Canvas` method gets an implementation in every backend, `RecordingCanvas` included.
- Pass a `std::string_view` to `DrawText` with an explicit length: it is not null-terminated.
- `Rect` is float and `Bottom() == y + h`, not `QRect::bottom() == y + height() - 1`. Ported widgets shift by one
  pixel against their Qt originals; this is intended (`doc/canvas.md`).
- CMake: `ui_add_library()` and `ui_add_test()` (`cmake/`), not raw `add_library`/`add_executable`. A Qt target
  passes `QT` (AUTOMOC). Options: `UI_BUILD_TESTS`, `UI_BUILD_QT_BACKEND` (opt-in, never inferred from a consumer
  flag), `UI_BUILD_QUICK3D_BACKEND`, `UI_BUILD_EXAMPLES`, `UI_ENABLE_COVERAGE`.
- Tests: `ui/<area>/test/Test<Name>.cpp`, registered with `ui_add_test()` through `add_subdirectory(test)`.
  - Test a painted view through its draw-call stream in a `RecordingCanvas`: command kinds and counts, polyline
    point counts, label strings, pen colours by theme role. Never pixels.
  - Test pure logic (axis transforms, interaction, geometry, formatting) with `EXPECT_NEAR` against an independent
    reference, never against the implementation's own output.
  - One behaviour per test. No redundant cases. Cover edge cases: empty data, one sample, zero-width plot area.
  - Extend the Qt conformance suite (`RecordingCanvas` against `QtCanvas`) when a new `Canvas` primitive lands.
- Update `doc/portability.md` for a new Tier 3 component or backend, `doc/canvas.md` when the `Canvas` contract
  changes, and the `README.md` tree when a directory changes.

## Build *(repo)*

```bash
cmake --preset host && cmake --build --preset host-Debug
ctest --preset host
cmake --preset host-qt && cmake --build --preset host-qt-Debug && ctest --preset host-qt
cmake --preset coverage && cmake --build --preset coverage && ctest --preset coverage
python3 scripts/validate-docs.py
```

- `host-qt` builds the Qt backend and examples; use it for a change under `ui/backend/qt/`.
- Without Qt6, `host-single-Debug` configures, builds and tests Tier 1 (the macOS and Windows CI jobs).

## Exceptions *(repo)*

- *Memory*: host GUI library. Heap is allowed in `ui/backend/**` and `ui/shell/**`: Qt owns its widget tree.
- *Memory*: the rest of `ui/` is allocation-free after construction, except setters outside the paint path
  (`SetPanels`, `SetAxisValues`). Never allocate per frame in `Paint()`: keep scratch buffers as members.
- *Memory*: Tier 1 uses `std` containers and `std::span`, not emil containers: Tier 1 has no external dependency.
- *Interfaces & errors*: preconditions use `assert()`, not `really_assert`: Tier 1 has no emil.
- *Style*: comments may also be license headers or `NOLINT`.
- *Testing*: no plain `TEST()`: cppcheck reports `syntaxError` on it. Use `TEST_F`.
- *Testing*: a Tier 1 test links no Qt and needs no display. A `ui/backend/qt` test sets
  `QT_QPA_PLATFORM=offscreen` in its own `main`.
