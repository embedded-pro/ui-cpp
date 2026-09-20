# GitHub Copilot Instructions — UI Toolbox

Canonical rules: **[AGENTS.md](../AGENTS.md)** (shared with Claude; VS Code auto-loads it).
Code-file specifics: `.github/instructions/` (applyTo-scoped).
Portability tiers: `doc/portability.md`. Canvas contract: `doc/canvas.md`.

Essentials (full detail in AGENTS.md):

- **Host GUI repo** — heap is allowed. What is forbidden is allocating **inside `Paint()`**: scratch
  buffers are members, sized once and reused. `SetPanels`/`SetAxisValues` may allocate.
- **Tier 1 is sacred** — no `<Q...>` include, no `Qt6::` link anywhere under `ui/` outside
  `ui/backend/qt/`. CI fails on violation, not review.
- **No `I` prefix on interfaces** — `Canvas`, not `ICanvas`. `virtual ~Name() = default`, never `= 0`.
- **Zero external deps in Tier 1** — no emil, no Qt, no fmt. Format via `ui/core/Format.hpp`
  (`std::format_to_n` into a caller-owned buffer); bare `std::format` allocates.
- **Batch drawing** — no per-sample virtual calls; accumulate into `DrawPolyline`.
- **`setStyleSheet(` in exactly one file** — `ui/backend/qt/QtTheme.cpp`. Everywhere else use `theme::`.
- **No colour, font or margin literals** at a call site — `theme::ColorRole`, `theme::FontRole`,
  `theme::Current().Charts()`.
- **No comments** except a non-obvious *why* (or license/NOLINT). Allman braces, brace-init,
  PascalCase types/methods, camelCase members.
- **Tests** — `TEST_F`, `StrictMock` only, never plain `TEST()`, no redundant cases. Tier 1 tests
  need no Qt and no display.
- **No exceptions** — `std::optional`/status enums.
- **Be terse** — minimal prose; report file paths + pass/fail.
