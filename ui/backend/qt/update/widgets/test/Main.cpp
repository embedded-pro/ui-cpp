#include <QApplication>
#include <QStandardPaths>
#include <gmock/gmock.h>

int main(int argc, char** argv)
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");

    QStandardPaths::setTestModeEnabled(true);
    QApplication application{ argc, argv };
    QApplication::setOrganizationName("ui-cpp-test");
    QApplication::setApplicationName("update-widgets-test");
    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
