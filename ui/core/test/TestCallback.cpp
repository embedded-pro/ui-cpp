#include "ui/core/Callback.hpp"
#include <gmock/gmock.h>

namespace
{
    class CallbackTest
        : public ::testing::Test
    {
    protected:
        ui::Callback<void()> notification;
        ui::Callback<int(int, int)> combination;
    };
}

TEST_F(CallbackTest, DefaultConstructedIsEmpty)
{
    EXPECT_FALSE(static_cast<bool>(notification));
}

TEST_F(CallbackTest, AssignedCallbackIsInvocable)
{
    auto invoked = false;
    notification = [&invoked]
    {
        invoked = true;
    };

    ASSERT_TRUE(static_cast<bool>(notification));
    notification();

    EXPECT_TRUE(invoked);
}

TEST_F(CallbackTest, ForwardsArgumentsAndReturnsResult)
{
    combination = [](int left, int right)
    {
        return left + right;
    };

    EXPECT_EQ(combination(2, 3), 5);
}

TEST_F(CallbackTest, ResetMakesCallbackEmpty)
{
    notification = [] {};
    notification.Reset();

    EXPECT_FALSE(static_cast<bool>(notification));
}

TEST_F(CallbackTest, CopyPreservesTarget)
{
    auto counter = 0;
    notification = [&counter]
    {
        ++counter;
    };

    auto copy = notification;
    copy();
    notification();

    EXPECT_EQ(counter, 2);
}

TEST_F(CallbackTest, ReassignmentReplacesTarget)
{
    auto first = 0;
    auto second = 0;

    notification = [&first]
    {
        ++first;
    };
    notification = [&second]
    {
        ++second;
    };
    notification();

    EXPECT_EQ(first, 0);
    EXPECT_EQ(second, 1);
}

// The stored target is copied through placement new rather than assignment, so a copied callback
// must carry its own captured state and not alias the original's.
TEST_F(CallbackTest, CopyingACallbackCopiesItsCapturedState)
{
    auto observed = 0;
    auto captured = 7;

    ui::Callback<void()> original{ [&observed, captured]
        {
            observed = captured;
        } };

    captured = 9;

    auto copy = original;
    original.Reset();

    ASSERT_TRUE(static_cast<bool>(copy));
    copy();

    EXPECT_EQ(observed, 7);
}
