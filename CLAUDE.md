# UI Toolbox — Claude Instructions

Canonical rules: **[AGENTS.md](AGENTS.md)** (shared with Copilot).
Portability tiers: `doc/portability.md`. Geometry conventions: `doc/canvas.md`. 3D stage: `doc/scene3d.md`.

Essentials (full detail in AGENTS.md):

- **Host GUI repo** — heap allowed in `ui/backend/**` and `ui/shell/**`; the rest of `ui/` must be
  **allocation-free after construction**. Never allocate per-frame in `Paint()`.
- **Tier 1 is sacred** — no `<Q...>` include, no `Qt6::` link anywhere under `ui/` outside
  `ui/backend/qt/`. CI fails on violation, not review.
- **No `I` prefix on interfaces** — `Canvas`, not `ICanvas`.
- **Zero external deps in Tier 1** — no emil, no Qt, no fmt. emil helpers are optional
  (`if (COMMAND ...)`). Format via `ui/core/Format.hpp` (`std::format_to_n` into a caller-owned
  buffer); bare `std::format` allocates, so never inside `Paint()`.
- **Batch drawing** — no per-sample virtual calls; accumulate into `DrawPolyline`.
- **`setStyleSheet` in exactly one file** — `ui/backend/qt/QtTheme.cpp`. Everywhere else use `theme::`.
- **Rect is float, `Bottom() == y + h`** — deliberately differs from `QRect`.
- **Style** — Allman, 4-space, `{}` init, PascalCase types/methods, camelCase members. No comments
  except non-obvious *why*.
- **Tests** — `TEST_F`, `StrictMock` only, `EXPECT_NEAR`. Tier 1 tests need no Qt and no display.
- **Be terse** — minimal prose; report file paths + pass/fail.
