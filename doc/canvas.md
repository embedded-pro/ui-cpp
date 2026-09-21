# The Canvas contract

`ui/core/Canvas.hpp` is the whole rendering surface. Everything a widget can draw goes through its
21 methods, and every backend implements all of them — there is no escape hatch and no optional
subset. Two implementations exist today: `QtCanvas` over `QPainter`, and `RecordingCanvas`, which
captures the call stream for tests.

## Geometry conventions

Coordinates are `float`, not integers. `Rect` is `{ x, y, width, height }` and **`Bottom()` is
`y + height`**, deliberately unlike `QRect::bottom()`, which is `y + height - 1`. A widget ported
from Qt therefore sits one pixel differently from its original at the bottom and right edges. That
was a considered trade: `QRect`'s convention only makes sense for an integer raster, and it breaks
down the moment a backend draws with sub-pixel precision.

`QRectF` already uses the `y + height` convention, so `ToQt(Rect)` is a straight field copy.

## State

`SetPen`, `SetBrush`, `SetFont`, `SetAntialiasing` and the clip set canvas state that persists until
changed. `Save`/`Restore` bracket a change; `CanvasStateGuard` does it with RAII. `Restore` restores
everything `Save` captured, the clip and the transform included — `QPainter::save` and `restore`
define the behaviour, and any other backend has to match it.

`ClearClip` removes clipping entirely rather than popping one level. Nested clips are not part of
the contract.

`LineStyle::None` draws no outline at all, leaving the brush to fill the shape. A transparent pen
colour renders identically, but the two are not interchangeable: only `LineStyle::None` is
distinguishable in a recorded command stream from a stroke that was meant to be visible and was
given the wrong colour.

## Text

`DrawText` has two overloads. The `Point` overload places the text **baseline** at that point, which
is what chart labels need. The `Rect` overload aligns text inside a box. Qt aligns by box rather
than by baseline, so `TextVerticalAlign::Baseline` maps to vertical centring in the rectangle form;
use the `Point` overload when the baseline actually matters.

Text is UTF-8, and a `std::string_view` handed to `DrawText` is **not** guaranteed to be
null-terminated — `ui::FormatBuffer` hands out views into a fixed array. A backend must convert with
an explicit length. `QtCanvas` uses `QString::fromUtf8(text.data(), text.size())` for exactly this
reason; the `const char*` overload would read past the end.

`MeasureText` is the one question the portable layer asks the backend, because chart margins are
sized from label widths. It reports the advance width and the line height of the *currently set*
font, so `SetFont` precedes it. `LineHeight` reports that same line height without a string, for
laying out stacked rows where the advance is irrelevant; `MeasureText(t).height == LineHeight()`
holds for every `t`, and both backends are held to it by a conformance test. Measuring the empty
string is not a substitute — `RecordingCanvas` reports zero width for it, and a backend is free to
return an ink box rather than a line box.

## Batching

`DrawPolyline` takes the whole trace in one call. This is not a convenience: the widgets this
library replaces called `drawLine` once per sample, over ring buffers holding up to 256 K samples,
and a virtual call per segment would have put the indirection on the hot path. The abstracted code
therefore issues **fewer** draw calls than the Qt original it came from.

The corollary is an allocation rule. Nothing allocates inside `Paint`, on either side of the
interface: `ChartCore` accumulates into a reused `std::vector<Point>`, and `QtCanvas` copies it into
a reused `QPolygonF` member. That is why `QtPaintedWidget` keeps its `QtCanvas` as a member and
rebinds the painter each event rather than constructing a canvas per frame.

## Adding a backend

Implement all 21 methods. If one cannot be expressed on the target toolkit, that is a finding about
the interface, not a reason to stub the method out — raise it rather than narrowing the contract
silently. `RecordingCanvas` exists to catch the opposite failure: it is a second, non-Qt
implementation, so an interface that has quietly grown Qt-shaped stops compiling or stops being
implementable there first.
