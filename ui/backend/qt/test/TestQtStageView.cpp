#include "ui/backend/qt/QtPaintedWidget.hpp"
#include "ui/backend/qt/test/QtTestSupport.hpp"
#include "ui/stage/StageBuilder.hpp"
#include "ui/stage/StageView.hpp"
#include <QCoreApplication>
#include <QMouseEvent>
#include <gmock/gmock.h>

namespace
{
    using ui::stage::NodeId;
    using ui::stage::Vector3;

    class QtStageViewTest
        : public ::testing::Test
    {
    protected:
        QtStageViewTest()
        {
            ui::theme::SetCurrent(ui::theme::Light());
            widget.resize(320, 240);

            ui::stage::AddBox(view.Scene(), node, Vector3{ 0.6f, 0.6f, 0.6f }, view.Scene().AddMaterial(ui::stage::materials::Plastic(ui::Color::Rgb(0xC0392B))));
        }

        [[nodiscard]] QImage Render()
        {
            QImage image{ 320, 240, QImage::Format_ARGB32 };
            image.fill(qRgb(0, 0, 0));
            widget.render(&image);
            return image;
        }

        void Click(QPointF position)
        {
            QMouseEvent press{ QEvent::MouseButtonPress, position, position, ::Qt::LeftButton, ::Qt::LeftButton, ::Qt::NoModifier };
            QMouseEvent release{ QEvent::MouseButtonRelease, position, position, ::Qt::LeftButton, ::Qt::NoButton, ::Qt::NoModifier };
            QCoreApplication::sendEvent(&widget, &press);
            QCoreApplication::sendEvent(&widget, &release);
        }

        ui::stage::StageView view;
        NodeId node{ view.Scene().Graph().AddFrame(NodeId{}, ui::stage::Transform3::Translation(Vector3{ 0.0f, 0.0f, 0.3f })) };
        ui::backend::qt::QtPaintedWidget widget{ view };
    };
}

TEST_F(QtStageViewTest, TheStagePaintsItsBackgroundAndItsParts)
{
    const auto image = Render();
    const auto sceneBackground = ui::theme::Light().Get(ui::theme::ColorRole::SceneBackground);
    const auto backgroundPixel = qRgb(sceneBackground.red, sceneBackground.green, sceneBackground.blue);

    EXPECT_EQ(image.pixel(2, 2), backgroundPixel);
    EXPECT_NE(image.pixel(160, 120), backgroundPixel);
    EXPECT_GT(qRed(image.pixel(160, 120)), qBlue(image.pixel(160, 120)));
}

TEST_F(QtStageViewTest, AClickThroughTheQtAdapterSelectsThePart)
{
    (void)Render();

    Click(QPointF{ 160.0, 120.0 });

    EXPECT_EQ(view.Selection(), node);
}
