#include <QApplication>
#include <QByteArray>
#include <gmock/gmock.h>

// QApplication has to be destroyed before the C runtime's exit handlers run: Qt tears down its
// own input-device registry there, so an application object owned by a static outlives the
// statics it depends on and segfaults after the last test has already passed.
int main(int argc, char** argv)
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");

    QApplication application{ argc, argv };
    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}
