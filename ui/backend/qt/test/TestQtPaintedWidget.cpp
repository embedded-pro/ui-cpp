#include "ui/backend/qt/QtPaintedWidget.hpp"
#include "ui/backend/qt/test/QtTestSupport.hpp"
#include <QCoreApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <gmock/gmock.h>
#include <memory>

namespace
{
    constexpr QRgb background{ 0xFFFFFFFFu };

    class PaintedViewMock
        : public ui::PaintedView
    {
    public:
        MOCK_METHOD(void, Paint, (ui::Canvas & canvas, const ui::Rect& bounds), (override));
        MOCK_METHOD(ui::Size, MinimumSize, (), (const, override));
        MOCK_METHOD(void, OnMousePress, (const ui::MouseEvent& event), (override));
        MOCK_METHOD(void, OnMouseMove, (const ui::MouseEvent& event), (override));
        MOCK_METHOD(void, OnMouseRelease, (const ui::MouseEvent& event), (override));
        MOCK_METHOD(void, OnMouseDoubleClick, (const ui::MouseEvent& event), (override));
        MOCK_METHOD(void, OnMouseLeave, (), (override));
        MOCK_METHOD(void, OnWheel, (const ui::WheelEvent& event), (override));
        MOCK_METHOD(void, OnKeyPress, (const ui::KeyEvent& event), (override));

        void Repaint()
        {
            RequestRepaint();
        }
    };

    class QtPaintedWidgetTest
        : public ::testing::Test
    {
    protected:
        QtPaintedWidgetTest()
        {
            ui::theme::SetCurrent(ui::theme::Light());
            widget.resize(200, 160);
        }

        void Send(QEvent& event)
        {
            QCoreApplication::sendEvent(&widget, &event);
        }

        static QMouseEvent Mouse(QEvent::Type type, QPointF position, ::Qt::MouseButton button)
        {
            return QMouseEvent{ type, position, position, button, button, ::Qt::NoModifier };
        }

        ::testing::StrictMock<PaintedViewMock> view;
        ui::backend::qt::QtPaintedWidget widget{ view };
    };
}

TEST_F(QtPaintedWidgetTest, WheelDeltaKeepsQtSignConvention)
{
    ui::WheelEvent forwarded;
    EXPECT_CALL(view, OnWheel(::testing::_)).WillOnce(::testing::SaveArg<0>(&forwarded));

    QWheelEvent event{ QPointF{ 80.0f, 40.0f }, QPointF{ 80.0f, 40.0f }, QPoint{ 0, 0 }, QPoint{ 0, 120 },
        ::Qt::NoButton, ::Qt::NoModifier, ::Qt::NoScrollPhase, false };
    Send(event);

    EXPECT_NEAR(forwarded.delta, 120.0f, 1e-3f);
    EXPECT_NEAR(forwarded.position.x, 80.0f, 1e-3f);
    EXPECT_NEAR(forwarded.position.y, 40.0f, 1e-3f);
}

TEST_F(QtPaintedWidgetTest, WheelDeltaKeepsItsNegativeSign)
{
    ui::WheelEvent forwarded;
    EXPECT_CALL(view, OnWheel(::testing::_)).WillOnce(::testing::SaveArg<0>(&forwarded));

    QWheelEvent event{ QPointF{ 10.0f, 10.0f }, QPointF{ 10.0f, 10.0f }, QPoint{ 0, 0 }, QPoint{ 0, -120 },
        ::Qt::NoButton, ::Qt::NoModifier, ::Qt::NoScrollPhase, false };
    Send(event);

    EXPECT_NEAR(forwarded.delta, -120.0f, 1e-3f);
}

TEST_F(QtPaintedWidgetTest, MousePressCarriesPositionAndButton)
{
    ui::MouseEvent forwarded;
    EXPECT_CALL(view, OnMousePress(::testing::_)).WillOnce(::testing::SaveArg<0>(&forwarded));

    auto event = Mouse(QEvent::MouseButtonPress, QPointF{ 30.0f, 50.0f }, ::Qt::LeftButton);
    Send(event);

    EXPECT_EQ(forwarded.button, ui::MouseButton::Left);
    EXPECT_NEAR(forwarded.position.x, 30.0f, 1e-3f);
    EXPECT_NEAR(forwarded.position.y, 50.0f, 1e-3f);
}

TEST_F(QtPaintedWidgetTest, MouseMoveArrivesWithNoButtonHeld)
{
    ui::MouseEvent forwarded;
    EXPECT_CALL(view, OnMouseMove(::testing::_)).WillOnce(::testing::SaveArg<0>(&forwarded));

    auto event = Mouse(QEvent::MouseMove, QPointF{ 12.0f, 90.0f }, ::Qt::NoButton);
    Send(event);

    EXPECT_EQ(forwarded.button, ui::MouseButton::None);
    EXPECT_NEAR(forwarded.position.x, 12.0f, 1e-3f);
}

TEST_F(QtPaintedWidgetTest, MouseReleaseAndDoubleClickAreForwarded)
{
    ui::MouseEvent released;
    ui::MouseEvent doubleClicked;
    EXPECT_CALL(view, OnMouseRelease(::testing::_)).WillOnce(::testing::SaveArg<0>(&released));
    EXPECT_CALL(view, OnMouseDoubleClick(::testing::_)).WillOnce(::testing::SaveArg<0>(&doubleClicked));

    auto release = Mouse(QEvent::MouseButtonRelease, QPointF{ 5.0f, 5.0f }, ::Qt::LeftButton);
    Send(release);

    auto doubleClick = Mouse(QEvent::MouseButtonDblClick, QPointF{ 7.0f, 8.0f }, ::Qt::LeftButton);
    Send(doubleClick);

    EXPECT_EQ(released.button, ui::MouseButton::Left);
    EXPECT_NEAR(doubleClicked.position.y, 8.0f, 1e-3f);
}

TEST_F(QtPaintedWidgetTest, RightAndMiddleButtonsAreDistinguished)
{
    ui::MouseEvent right;
    ui::MouseEvent middle;
    EXPECT_CALL(view, OnMousePress(::testing::_))
        .WillOnce(::testing::SaveArg<0>(&right))
        .WillOnce(::testing::SaveArg<0>(&middle));

    auto rightPress = Mouse(QEvent::MouseButtonPress, QPointF{ 1.0f, 1.0f }, ::Qt::RightButton);
    Send(rightPress);

    auto middlePress = Mouse(QEvent::MouseButtonPress, QPointF{ 1.0f, 1.0f }, ::Qt::MiddleButton);
    Send(middlePress);

    EXPECT_EQ(right.button, ui::MouseButton::Right);
    EXPECT_EQ(middle.button, ui::MouseButton::Middle);
}

TEST_F(QtPaintedWidgetTest, ModifiersAreTranslated)
{
    ui::MouseEvent forwarded;
    EXPECT_CALL(view, OnMousePress(::testing::_)).WillOnce(::testing::SaveArg<0>(&forwarded));

    QMouseEvent event{ QEvent::MouseButtonPress, QPointF{ 2.0f, 3.0f }, QPointF{ 2.0f, 3.0f },
        ::Qt::LeftButton, ::Qt::LeftButton, ::Qt::ShiftModifier | ::Qt::ControlModifier };
    Send(event);

    EXPECT_TRUE(forwarded.modifiers.shift);
    EXPECT_TRUE(forwarded.modifiers.control);
    EXPECT_FALSE(forwarded.modifiers.alt);
}

TEST_F(QtPaintedWidgetTest, LeaveIsForwarded)
{
    EXPECT_CALL(view, OnMouseLeave());

    QEvent event{ QEvent::Leave };
    Send(event);
}

TEST_F(QtPaintedWidgetTest, KeyPressCarriesTheMappedKeyAndCodepoint)
{
    ui::KeyEvent forwarded;
    EXPECT_CALL(view, OnKeyPress(::testing::_)).WillOnce(::testing::SaveArg<0>(&forwarded));

    QKeyEvent event{ QEvent::KeyPress, ::Qt::Key_Left, ::Qt::NoModifier };
    Send(event);

    EXPECT_EQ(forwarded.key, ui::Key::Left);
    EXPECT_EQ(forwarded.codepoint, char32_t{ 0 });
}

TEST_F(QtPaintedWidgetTest, UnmappedKeysStillCarryTheirCodepoint)
{
    ui::KeyEvent forwarded;
    EXPECT_CALL(view, OnKeyPress(::testing::_)).WillOnce(::testing::SaveArg<0>(&forwarded));

    QKeyEvent event{ QEvent::KeyPress, ::Qt::Key_A, ::Qt::NoModifier, QStringLiteral("a") };
    Send(event);

    EXPECT_EQ(forwarded.key, ui::Key::Unknown);
    EXPECT_EQ(forwarded.codepoint, U'a');
}

TEST_F(QtPaintedWidgetTest, MinimumSizeHintComesFromTheView)
{
    EXPECT_CALL(view, MinimumSize()).WillOnce(::testing::Return(ui::Size{ 320.0f, 240.0f }));

    EXPECT_EQ(widget.minimumSizeHint(), QSize(320, 240));
}

TEST_F(QtPaintedWidgetTest, PaintFillsTheThemeBackgroundAndDelegatesToTheView)
{
    ui::Rect painted;
    EXPECT_CALL(view, Paint(::testing::_, ::testing::_)).WillOnce(::testing::SaveArg<1>(&painted));

    QImage image{ 200, 160, QImage::Format_ARGB32 };
    image.fill(qRgb(0, 0, 0));
    widget.render(&image);

    EXPECT_EQ(image.pixel(100, 80), background);
    EXPECT_NEAR(painted.width, 200.0f, 1e-3f);
    EXPECT_NEAR(painted.height, 160.0f, 1e-3f);
}

TEST_F(QtPaintedWidgetTest, DestroyingTheWidgetDetachesItFromTheView)
{
    {
        const ui::backend::qt::QtPaintedWidget scoped{ view };
        EXPECT_TRUE(view.HasHost());
    }

    // The fixture's own widget re-attached itself first, so the view stays hosted; what matters is
    // that the destroyed widget is no longer the one it points at.
    view.Repaint();
}

// The ordering this link exists for. A window destroys its chart and scene members before
// ~QMainWindow deletes the child widgets hosting them, so the widget must tolerate its view
// vanishing underneath it.
TEST_F(QtPaintedWidgetTest, AViewDestroyedBeforeItsWidgetLeavesTheWidgetInert)
{
    auto owned = std::make_unique<::testing::StrictMock<PaintedViewMock>>();
    ui::backend::qt::QtPaintedWidget orphaned{ *owned };
    orphaned.resize(200, 160);

    owned.reset();

    QImage image{ 200, 160, QImage::Format_ARGB32 };
    image.fill(qRgb(0, 0, 0));
    orphaned.render(&image);

    auto press = Mouse(QEvent::MouseButtonPress, QPointF{ 10.0f, 10.0f }, ::Qt::LeftButton);
    QCoreApplication::sendEvent(&orphaned, &press);

    EXPECT_EQ(image.pixel(100, 80), background);
}

TEST_F(QtPaintedWidgetTest, TheBackgroundRoleSelectsTheFillColour)
{
    EXPECT_CALL(view, Paint(::testing::_, ::testing::_));

    widget.SetBackgroundRole(ui::theme::ColorRole::ScopeBackground);

    QImage image{ 200, 160, QImage::Format_ARGB32 };
    image.fill(qRgb(255, 0, 0));
    widget.render(&image);

    const auto expected = ui::theme::Light().Get(ui::theme::ColorRole::ScopeBackground);
    EXPECT_EQ(image.pixel(100, 80), qRgb(expected.red, expected.green, expected.blue));
}

// The grab cursor is opt-in: a chart wants it while panning, a 3D scene that orbits does not.
TEST_F(QtPaintedWidgetTest, ThePanCursorIsOnlyShownWhenEnabled)
{
    EXPECT_CALL(view, OnMousePress(::testing::_)).Times(2);
    EXPECT_CALL(view, OnMouseRelease(::testing::_));

    auto press = Mouse(QEvent::MouseButtonPress, QPointF{ 10.0f, 10.0f }, ::Qt::LeftButton);
    Send(press);
    EXPECT_NE(widget.cursor().shape(), ::Qt::ClosedHandCursor);

    widget.SetPanCursorEnabled(true);
    Send(press);
    EXPECT_EQ(widget.cursor().shape(), ::Qt::ClosedHandCursor);

    auto release = Mouse(QEvent::MouseButtonRelease, QPointF{ 10.0f, 10.0f }, ::Qt::LeftButton);
    Send(release);
    EXPECT_NE(widget.cursor().shape(), ::Qt::ClosedHandCursor);
}

TEST_F(QtPaintedWidgetTest, AViewRepaintRequestReachesTheWidget)
{
    EXPECT_CALL(view, Paint(::testing::_, ::testing::_)).Times(::testing::AtLeast(1));

    widget.show();
    QCoreApplication::processEvents();

    view.Repaint();
    QCoreApplication::processEvents();
}
