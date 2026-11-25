find_package(Qt6 6.2 REQUIRED COMPONENTS Core Network)

ui_add_library(ui.backend.qt.update QT
    SOURCES
        CrashReporter.cpp
        CrashReporter.hpp
        IssueLog.cpp
        IssueLog.hpp
        IssueReport.cpp
        IssueReport.hpp
        ReleaseFeed.cpp
        ReleaseFeed.hpp
        ReleaseVersion.cpp
        ReleaseVersion.hpp
        UpdateChecker.cpp
        UpdateChecker.hpp
        UpdateDownloader.cpp
        UpdateDownloader.hpp
        UpdateInstallation.cpp
        UpdateInstallation.hpp
        UpdateInstaller.cpp
        UpdateInstaller.hpp
        UpdateSession.cpp
        UpdateSession.hpp
    LINK
        Qt6::Core
        Qt6::Network
)

if (WIN32)
    target_link_libraries(ui.backend.qt.update PRIVATE dbghelp)
endif()

add_subdirectory(test)
add_subdirectory(widgets)
