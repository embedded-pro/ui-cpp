# UI Toolbox — Canonical Rules

Shared, backend-agnostic UI components for the `embedded-pro` C++ toolboxes.
Consumed by `e-foc`, `numerical-toolbox-cpp`, `robotics-toolbox-cpp`, `neural-network-toobox-cpp`.

## Scope of this repository

This repository targets **host tools and GUI**, not firmware. That changes two of the canonical
toolbox rules, deliberately and explicitly:

### Memory — heap is permitted, with one hard exception

Unlike the library repos, heap allocation is **allowed** in `ui/backend/**` and `ui/shell/**`:
Qt owns its widget tree with `new`, and fighting that buys nothing.

**But** code in `ui/core/`, `ui/theme/`, `ui/model/`, `ui/widgets/` and `ui/sim/` must be
**allocation-free after construction** — preallocate buffers, take `std::span` inputs, reuse
member vectors across frames. This is the rule that keeps a non-desktop backend viable, and
unlike a blanket "no heap" it is actually testable.

Never allocate per-frame inside a `Paint()` implementation.

### Tests — UI code is tested here

The consumer repos exclude their UI from coverage. This repository does not. Tier 1 (below) is
covered by unit tests that need no Qt and no display.

## Portability tiers

Every component is Tier 1, 2 or 3. The tier is not advisory — it is enforced in CI.

**Tier 1 — genuinely backend-agnostic.** No `<Q...>` include, no `Qt6::` link, builds and tests
on Linux, macOS and Windows with no Qt installed.
Everything under `ui/` **except** the toolkit backends — today `ui/core/`, `ui/theme/`, `ui/charts/`
and `ui/backend/recording/`. The CI check is phrased as that exclusion, so a directory added later
is policed without anyone remembering to list it.

**Tier 2 — interface abstracted, only a Qt implementation is reasonable.**
`FormView`, `TableView`, `AppShell`, `Alert`. Declared in `ui/shell/`, implemented in `ui/backend/qt/`.

**Tier 3 — Qt-only, no abstraction attempted.** Lives under `ui/backend/qt/` or stays in the consumer.
Requires a one-line justification in `doc/portability.md`.

## Enforced invariants

1. No `<Q...>` include and no `Qt6::` link anywhere in a Tier 1 directory.
2. Qt-specific code lives only under `ui/backend/qt/`.
3. `setStyleSheet(` may be called in exactly one file: `ui/backend/qt/QtTheme.cpp`.
4. No per-sample virtual calls in a `Paint()` — batch into `DrawPolyline`.

1–3 are checked by the `guardrails` CI job. Breaking them fails the build, not review.

## Dependencies

**Tier 1 has zero external dependencies** — not emil, not Qt, not fmt. This is deliberate: it is
what lets the core build anywhere and keeps the repo reusable outside this org.

emil is **optional**. Its helpers are used when a consumer provides them
(`if (COMMAND emil_clangformat_directories)`), with plain CMake equivalents otherwise. The four
consumers pin three different emil revisions; depending on it here would import that skew.

Formatting goes through `ui/core/Format.hpp`, which wraps `std::format_to_n` into a caller-owned
buffer. Inside `Paint` use `FormatBuffer`, never `std::format` directly: `std::format` returns a
`std::string` and therefore allocates.

## Style

- Allman braces, 4-space indent, `{}` init. `.clang-format` is authoritative.
- PascalCase types and methods, camelCase members, lowercase namespaces.
- Interfaces: plain role names, **no `I` prefix** (`Canvas`, not `ICanvas`) — matching
  `e-foc/core/foc/interfaces/` (`Controllable`, `PhaseCurrentsObservable`).
- Interfaces declare `virtual ~Name() = default`, never `= 0`.
- No exceptions — `std::optional` or status enums.
- No comments except license headers, `NOLINT`, and a brief note on genuinely non-obvious geometry.
- Functions ≤ ~30 lines. `const`/`constexpr`-correct. Fixed-width ints.

## Geometry convention

`Rect` is float-based and **`Bottom() == y + h`**, not Qt's integer `QRect::bottom() == y + height() - 1`.
Ported widgets shift by one pixel against their Qt originals. This is intended; baselines were
regenerated once, deliberately. See `doc/canvas.md`.

## Be terse

Minimal prose; report file paths + pass/fail.
