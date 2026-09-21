# UI Toolbox (C++)

Shared, backend-agnostic UI components for the `embedded-pro` C++ toolboxes — extracted from the
Qt simulator/tool trees of `e-foc`, `numerical-toolbox-cpp`, `robotics-toolbox-cpp` and
`neural-network-toobox-cpp`, which had drifted into byte-identical copies of the same widgets with
no shared theme.

## What this is

A small rendering and control abstraction plus the widgets built on it. The portable half knows
nothing about Qt: it draws through a `Canvas` interface of ~15 primitives and reports input through
plain structs. Qt is the first backend; a second toolkit can be added without touching a widget.

```text
ui/core      geometry, colour, font, Canvas, input, PaintedView, Callback   — no Qt
ui/theme     colour/font roles, chart metrics, Light + Instrument themes    — no Qt
ui/charts    ChartCore, AxisTransform (linear + log10), interaction         — no Qt
ui/scope     ScopeCore, RingBuffer, edge triggering                         — no Qt
ui/scene     Vector3, orbit camera, ground grid and axis triad             — no Qt
ui/backend/recording  records draw calls — the test harness                 — no Qt

ui/backend/qt         QtCanvas, QtPaintedWidget, QtTheme — Qt6 Widgets
```

Still to come: `ui/model` (FormSpec, TableModel), `ui/sim`,
`ui/shell` (AppShell, FormView, TableView) and `ui/backend/svg`.

Everything but `ui/backend/qt` builds and is unit-tested on Linux, macOS and Windows **with no Qt
installed**. That is the point of the split, and it is enforced in CI rather than by convention.

## Build

```sh
cmake --preset host
cmake --build --preset host-Debug
ctest --preset host
```

To build the Qt backend as well — `QtCanvas`, `QtPaintedWidget`, `QtTheme` and their tests — use the
`host-qt` preset, which needs `qt6-base-dev` and `libgl1-mesa-dev`:

```sh
cmake --preset host-qt
cmake --build --preset host-qt-Debug
ctest --preset host-qt
```

`UI_BUILD_QT_BACKEND` is opt-in and off by default, and is deliberately *not* inferred from a
consumer's `*_BUILD_SIMULATOR` flag — e-foc configures its host tools (and therefore Qt) on every
host build, while the toolboxes only do so under their simulator option.

## Contributing

Read [AGENTS.md](AGENTS.md) first. The portability tiers and the four enforced invariants are not
advisory — the `guardrails` CI job fails the build on violation.

## License

MIT. See [LICENSE](LICENSE).
