# Portability tiers

`AGENTS.md` defines the three tiers and the invariants the `guardrails` CI job enforces. This file
records what actually sits in each one and why anything in Tier 3 is there.

## Tier 1 — no Qt, builds and tests everywhere

`ui/core`, `ui/theme`, `ui/model`, `ui/shell`, `ui/charts`, `ui/scope`, `ui/scene`, `ui/stage`,
`ui/backend/recording`.

These compile and their tests run on Linux, macOS and Windows with no Qt installed and no display.
CI proves it: the `macos-latest` and `windows-latest` jobs build the `host-single-Debug` preset,
which never configures a Qt target.

`ui/terminal/` is Tier 1 but is deliberately **not** on the allocation-free list. A VT100
scrollback is unbounded by definition, and `Vt100Terminal::TakeOutgoing` returns a string; pinning
either would mean a fixed history nobody asked for. Neither the parser nor the screen sits on a
per-sample path, so the rule that exists to keep `Paint` allocation-free does not apply to them.

`ui/stage/` allocates while a scene is being built: adding a node, part, mesh or trail grows a
vector, and STL import is a load-time operation. That is setup, not painting. Moving joints,
pushing trail points and relabelling do not allocate, and the renderer's scratch buffers grow only
on the first paint after the stage has grown. `ui.stage_allocation_test` pins this. The 3D view
draws through the 2D `Canvas` with no depth buffer, so it runs on every backend; see
`doc/scene3d.md`.

## Tier 2 — abstracted, currently only a Qt implementation

`ui/backend/qt/QtCanvas` implements `ui::Canvas`; `ui/backend/qt/QtPaintedWidget` hosts any
`ui::PaintedView` in a `QWidget` by implementing `ui::PaintedViewHost`. Both are replaceable: a
second toolkit needs one class each, and no widget changes.

The view and its host link in both directions, and whichever is destroyed first clears the other
end. That matters because a window destroys its view members before the toolkit deletes the child
widgets hosting them, so a one-way link leaves the adapter reaching into freed storage.

**Repaint cadence belongs to the host, not to the view.** A view calls `RequestRepaint()` when its
own configuration changes, never when data arrives: `ScopeCore::AddSample` accepts samples at tens
of kilohertz while a display follows at tens of hertz, so a host that wants a live trace drives its
own timer. `RecordingPaintedHost` exists partly to keep that assertable from Tier 1.

`ui/shell/FormView` is implemented by `ui/backend/qt/QtFormView` and by
`ui/backend/recording/RecordingFormView`; `ui/shell/ShellView` by `ui/backend/qt/QtAppShell` and by
`ui/backend/recording/RecordingShell`. Two implementations of each exist today, so these interfaces
are held honest the same way `RecordingCanvas` holds `Canvas` honest — and their Tier 1 tests run on
the macOS and Windows jobs with no Qt installed, which is the part that cannot be faked.

## Tier 3 — Qt-only, with justification

| Component       | Why it is not abstracted                                                                                                                                                       |
|-----------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `QtTheme`       | Maps theme roles onto `QPalette` and Qt stylesheet strings. Both are Qt concepts with no equivalent elsewhere; another backend maps the same roles onto its own styling model. |
| `QtConversions` | By definition: it is the type mapping between `ui::` and `Qt::`.                                                                                                               |

## What degrades rather than breaks

Some input has no equivalent on a touch-first backend. `InputHandler` gives every event an empty
default implementation, so a view that relies on one of these still compiles and still runs — it
simply never receives the event.

| Event                                     | Absent on                    |
|-------------------------------------------|------------------------------|
| `OnMouseMove` with no button held (hover) | touch                        |
| `OnMouseLeave`                            | touch                        |
| `OnMouseDoubleClick`                      | some embedded input stacks   |
| `OnKeyPress`                              | any backend with no keyboard |

The chart's hover crosshair is the concrete case: it is driven by hover and leave, so on a touch
backend it simply never appears. Nothing else in the chart depends on either event.

## Deliberately out of scope

Direct manipulation of native widgets stays in the consumer. `ui/shell` describes *what* an
application is made of — a title, a parameter panel, named pages, a status line, a set of typed
fields — and the backend decides what that means. It never exposes a splitter handle, a layout, a
tab-bar policy or a dialog button box.

The distinction is between composition and manipulation, and it is worth stating plainly because an
earlier version of this section conflated them. "A title, a 350px panel, three named pages and a
status line" is data, and it ports: a toolkit with no desktop windowing system renders it as
something else entirely, but it can render it. `splitter->setStretchFactor(0, 0)` does not port, and
nothing in `ui/shell` returns a splitter to call it on. A consumer that genuinely needs one reaches
past the abstraction and talks to Qt, and that is expected rather than a failure:
`QtAppShell::SetPage` accepts any `QWidget*`, so a hand-written native page sits beside a portable
one with no wrapper type and no ceremony.

Event-loop integration is not abstracted either. `QTimer`, `QSocketNotifier` and
`QMetaObject::invokeMethod` thread marshalling stay in the consumer repositories.

A repeating group is a fixed set of numeric columns with add and remove, and nothing more. Sorting,
selection models, per-cell delegates and mixed column types are a spreadsheet rather than a form,
and are out of scope permanently.

`FormSpec::layout` sits on the composition side of that line, not the manipulation side. It says
whether a form reads as a column of labelled rows (`Stacked`) or as a single strip of controls
(`Inline`) — the shape an instrument toolbar wants — and a backend with no rows and columns at all
is still free to decide what that means. It is deliberately two named intents rather than a
stretch factor, an alignment flag or a margin: those describe a Qt layout, and describing a Qt
layout is what this section forbids. A group keeps its own stacked rows under either setting, so
`Inline` arranges the top level and never reaches inside a group.

## Verifying a change

Build the `host` preset (no Qt) and the `host-qt` preset (Qt backend and its tests) before pushing.
The first is what macOS and Windows CI run; a change that only builds with Qt present breaks two of
the three platforms without ever failing locally on Linux.
