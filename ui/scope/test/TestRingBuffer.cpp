#include "ui/scope/RingBuffer.hpp"
#include <gmock/gmock.h>

namespace
{
    class RingBufferTest
        : public ::testing::Test
    {
    protected:
        RingBufferTest()
        {
            buffer.Resize(4);
        }

        ui::scope::RingBuffer buffer;
    };
}

TEST_F(RingBufferTest, AFreshBufferHoldsNothing)
{
    EXPECT_EQ(buffer.Count(), 0u);
    EXPECT_EQ(buffer.Capacity(), 4u);
    EXPECT_FALSE(buffer.IsFull());
}

TEST_F(RingBufferTest, SamplesReadBackInPushOrderBeforeItWraps)
{
    buffer.Push(1.0f);
    buffer.Push(2.0f);
    buffer.Push(3.0f);

    EXPECT_EQ(buffer.Count(), 3u);
    EXPECT_NEAR(buffer.At(0), 1.0f, 1e-6f);
    EXPECT_NEAR(buffer.At(1), 2.0f, 1e-6f);
    EXPECT_NEAR(buffer.At(2), 3.0f, 1e-6f);
}

TEST_F(RingBufferTest, IndexZeroBecomesTheOldestRetainedSampleAfterWrapping)
{
    for (auto value : { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f })
        buffer.Push(value);

    EXPECT_EQ(buffer.Count(), 4u);
    EXPECT_TRUE(buffer.IsFull());
    EXPECT_NEAR(buffer.At(0), 3.0f, 1e-6f);
    EXPECT_NEAR(buffer.At(3), 6.0f, 1e-6f);
}

TEST_F(RingBufferTest, TheCountStopsAtCapacity)
{
    for (auto i = 0; i < 100; ++i)
        buffer.Push(static_cast<float>(i));

    EXPECT_EQ(buffer.Count(), 4u);
    EXPECT_NEAR(buffer.At(3), 99.0f, 1e-6f);
}

TEST_F(RingBufferTest, ReadingPastTheCountIsZeroRatherThanStaleData)
{
    buffer.Push(7.0f);

    EXPECT_NEAR(buffer.At(1), 0.0f, 1e-6f);
    EXPECT_NEAR(buffer.At(99), 0.0f, 1e-6f);
}

TEST_F(RingBufferTest, ClearingKeepsTheCapacityButDropsTheSamples)
{
    for (auto value : { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f })
        buffer.Push(value);

    buffer.Clear();

    EXPECT_EQ(buffer.Count(), 0u);
    EXPECT_EQ(buffer.Capacity(), 4u);
    EXPECT_NEAR(buffer.At(0), 0.0f, 1e-6f);
}

TEST_F(RingBufferTest, ResizingStartsOver)
{
    buffer.Push(1.0f);
    buffer.Resize(8);

    EXPECT_EQ(buffer.Count(), 0u);
    EXPECT_EQ(buffer.Capacity(), 8u);
}

TEST_F(RingBufferTest, PushingIntoAnUnsizedBufferIsIgnoredRatherThanUndefined)
{
    ui::scope::RingBuffer unsized;
    unsized.Push(1.0f);

    EXPECT_EQ(unsized.Count(), 0u);
    EXPECT_FALSE(unsized.IsFull());
}
