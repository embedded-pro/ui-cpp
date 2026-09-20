---
description: "Author unit tests for one ui/ component — TEST_F, StrictMock, RecordingCanvas draw-call assertions, no display."
agent: "unit-tester"
argument-hint: "Name the ui/ component to author unit tests for (e.g. ChartCore, Log10Axis, QtCanvas)"
model: "Claude Sonnet 5"
---

Author (or extend) the unit tests for the named **ui** component in
`ui/<area>/test/Test<Name>.cpp`. Follow the `unit-tester` workflow: read the component's public
interface; decide how its behaviour is observable — a painted view through the `RecordingCanvas`
draw-call stream (command kinds and counts, polyline point counts, label strings, pen colours by
theme role), pure logic directly with `EXPECT_NEAR` against an independently derived value; write
one `TEST_F` per distinct property; wire CMake with `ui_add_test()`; build, run, and report paths +
pass/fail.

Build and test with `cmake --preset host && cmake --build --preset host-Debug && ctest --preset host`,
or the `host-qt` preset for anything under `ui/backend/qt`.

Rules: `AGENTS.md` + `.github/instructions/testing.instructions.md` (StrictMock only, never plain
`TEST()`, no redundant cases, no Qt in a Tier 1 test). Tiers: `doc/portability.md`.

Component to cover:
