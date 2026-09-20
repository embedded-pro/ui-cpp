# Portability tiers

`AGENTS.md` defines the three tiers and the invariants the `guardrails` CI job enforces. This file
records what actually sits in each one and why anything in Tier 3 is there.

## Tier 1 — no Qt, builds and tests everywhere

`ui/core`, `ui/theme`, `ui/charts`, `ui/scope`, `ui/scene`, `ui/backend/recording`.

These compile and their tests run on Linux, macOS and Windows with no Qt installed and no display.
CI proves it: the `macos-latest` and `windows-latest` jobs build the `host-single-Debug` preset,
which never configures a Qt target.

## Tier 2 — abstracted, currently only a Qt implementation

`ui/backend/qt/QtCanvas` implements `ui::Canvas`; `ui/backend/qt/QtPaintedWidget` hosts any
`ui::PaintedView` in a `QWidget`. Both are replaceable: a second toolkit needs one class each, and
no widget changes.

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

Desktop window chrome — splitters, tab bars, menu bars, status bars, modal dialogs — is not
abstracted and will not be. There is no portable model for it: a toolkit without a desktop windowing
system does not host these applications differently, it hosts a different application. Consumers use
Qt directly for chrome and reach the portable layer through `QtPaintedWidget`.

Event-loop integration is the same story. `QTimer`, `QSocketNotifier` and
`QMetaObject::invokeMethod` thread marshalling stay in the consumer repositories.

## Verifying a change

Build the `host` preset (no Qt) and the `host-qt` preset (Qt backend and its tests) before pushing.
The first is what macOS and Windows CI run; a change that only builds with Qt present breaks two of
the three platforms without ever failing locally on Linux.
