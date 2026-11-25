#include "ui/backend/qt/update/CrashReason.hpp"
#include "gtest/gtest.h"
#include <csignal>
#include <stdexcept>
#include <string>

namespace
{
    using ui::backend::qt::update::detail::CurrentExceptionReason;
    using ui::backend::qt::update::detail::SignalName;

    class TestCrashReason
        : public testing::Test
    {};
}

TEST_F(TestCrashReason, names_a_standard_exception_by_its_message)
{
    std::string reason;
    try
    {
        throw std::runtime_error{ "mesh index out of range" };
    }
    catch (...)
    {
        reason = CurrentExceptionReason().data();
    }

    EXPECT_EQ("Unhandled C++ exception: mesh index out of range", reason);
}

TEST_F(TestCrashReason, names_an_exception_of_unknown_type)
{
    std::string reason;
    try
    {
        throw 42;
    }
    catch (...)
    {
        reason = CurrentExceptionReason().data();
    }

    EXPECT_EQ("Unhandled C++ exception of unknown type", reason);
}

TEST_F(TestCrashReason, names_terminate_without_an_exception)
{
    EXPECT_EQ(std::string{ "std::terminate without an exception" }, CurrentExceptionReason().data());
}

TEST_F(TestCrashReason, truncates_a_long_message)
{
    std::string reason;
    try
    {
        throw std::runtime_error{ std::string(1000, 'x') };
    }
    catch (...)
    {
        reason = CurrentExceptionReason().data();
    }

    EXPECT_EQ(255u, reason.size());
}

TEST_F(TestCrashReason, names_each_crash_signal)
{
    EXPECT_EQ(std::string{ "Segmentation fault" }, SignalName(SIGSEGV));
    EXPECT_EQ(std::string{ "Floating-point exception" }, SignalName(SIGFPE));
    EXPECT_EQ(std::string{ "Illegal instruction" }, SignalName(SIGILL));
    EXPECT_EQ(std::string{ "Aborted" }, SignalName(SIGABRT));
#ifdef SIGBUS
    EXPECT_EQ(std::string{ "Bus error" }, SignalName(SIGBUS));
#endif
    EXPECT_EQ(std::string{ "Unknown signal" }, SignalName(0));
}
