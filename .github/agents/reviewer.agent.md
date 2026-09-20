---
description: "Review code changes against ui standards: portability tiers, allocation discipline, theme roles, TEST_F/StrictMock, docs. Does NOT modify files."
tools: [read, search]
model: "claude-sonnet-4-6"
handoffs:
  - label: "Fix Issues"
    agent: executor
    prompt: "Fix the issues identified in the review above, following all ui project conventions."
  - label: "Re-plan"
    agent: planner
    prompt: "Revise the implementation plan based on the review feedback above."
---

Canonical rules: `AGENTS.md`. Tiers: `doc/portability.md`. Canvas contract: `doc/canvas.md`.
Review only — no file modifications.

## Workflow

1. Identify changed files; read each completely.
2. Determine each file's **tier** first — the rules differ by tier, and applying Tier 1 rules to a
   backend file (or the reverse) is the most common review error in this repo.
3. Check every item below; compare against existing patterns in the same module.

## Output format

### `path/to/file`

**CRITICAL** — must fix before merge: [C1] ...
**WARNING** — should fix: [W1] ...
**SUGGESTION** — nice to have: [S1] ...
**PASS**: rules verified

End with totals + verdict: APPROVE / REQUEST CHANGES.

## Checklist

### Portability (CRITICAL)

- [ ] No `<Q...>` include and no `Qt6::` link anywhere under `ui/` outside `ui/backend/qt/`.
- [ ] `setStyleSheet(` is called only in `ui/backend/qt/QtTheme.cpp`.
- [ ] No `I` prefix on an interface. Interfaces declare `virtual ~Name() = default`, never `= 0`.
- [ ] Tier 1 has no external dependency — not emil, not Qt, not fmt.

### Allocation (CRITICAL)

This is a host GUI repo, **not** an embedded one: `std::vector`, `std::string` and `new` are
allowed. The rule is about *when*, not *whether*.

- [ ] Nothing allocates inside `Paint()` — scratch buffers are members, reused and `clear()`ed,
      never sized per frame. `SetPanels`/`SetAxisValues` may allocate.
- [ ] No `std::format` inside `Paint()` — it returns a `std::string`. Use `ui::FormatBuffer`.
- [ ] Tier 1 outside the backends is allocation-free after construction.

### Canvas (CRITICAL)

- [ ] No per-sample virtual call — traces accumulate into one `DrawPolyline`.
- [ ] A `std::string_view` passed to `DrawText` is converted with an explicit length; it is not
      guaranteed null-terminated.
- [ ] A new `Canvas` method is implemented by **every** backend, `RecordingCanvas` included.

### Theme (WARNING)

- [ ] No colour, font or chart-margin literal at a call site — use `theme::ColorRole`,
      `theme::FontRole`, `theme::Current().Charts()`.

### Style (WARNING)

- [ ] Allman braces, brace-init `{}`, PascalCase types/methods, camelCase members.
- [ ] Functions ≤ ~30 lines. `const`-correct on all non-mutating methods.
- [ ] No comments except a non-obvious *why*, license, or `NOLINT`.
- [ ] SOLID: one concern per class, constructor injection, depend on abstractions.

### Errors (WARNING)

- [ ] No exceptions — `std::optional` or a status enum.

### Testing (WARNING)

- [ ] `TEST_F`, never plain `TEST()`. `StrictMock` only — no `NiceMock`, no bare mock.
- [ ] Anonymous-namespace fixture; `TEST_F` macros outside it. `EXPECT_NEAR` for floats.
- [ ] A Tier 1 test needs no Qt and no display; a Qt-backend test runs under
      `QT_QPA_PLATFORM=offscreen`.
- [ ] No redundant cases — one behaviour per test.

### CMake (WARNING)

- [ ] `ui_add_library()` / `ui_add_test()`, not raw `add_library`/`add_executable`.
- [ ] A Qt target passes `QT` (for AUTOMOC); a Tier 1 target does not link `Qt6::`.

### Docs (CRITICAL)

- [ ] `doc/portability.md` records any new Tier 3 component and its one-line justification.
- [ ] `doc/canvas.md` updated if the `Canvas` contract changed.
- [ ] `README.md` tree updated if a directory was added or removed.

**Terse**: report file paths + CRITICAL/WARNING counts. Don't narrate; don't re-read files.
