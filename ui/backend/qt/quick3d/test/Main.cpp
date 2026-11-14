#include <QByteArray>
#include <QGuiApplication>
#include <gmock/gmock.h>

int main(int argc, char** argv)
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication application{ argc, argv };
    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
