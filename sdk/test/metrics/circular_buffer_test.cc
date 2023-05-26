// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#include "opentelemetry/sdk/metrics/data/circular_buffer.h"
#include <gtest/gtest.h>
#include <limits>

using namespace opentelemetry::sdk::metrics;
namespace nostd = opentelemetry::nostd;

class AdaptingIntegerArrayTest : public testing::TestWithParam<uint64_t>
{};

INSTANTIATE_TEST_SUITE_P(InterestingValues,
                         AdaptingIntegerArrayTest,
                         testing::Values<uint64_t>(1,
                                                   std::numeric_limits<uint8_t>::max() + 1ull,
                                                   std::numeric_limits<uint16_t>::max() + 1ull,
                                                   std::numeric_limits<uint32_t>::max() + 1ull));

TEST_P(AdaptingIntegerArrayTest, PreservesSizeOnEnlargement)
{
  AdaptingIntegerArray counter(10);
  EXPECT_EQ(counter.Size(), 10);
  counter.Increment(0, GetParam());
  EXPECT_EQ(counter.Size(), 10);
}

TEST_P(AdaptingIntegerArrayTest, IncrementAndGet)
{
  AdaptingIntegerArray counter(10);
  for (int idx = 0; idx < 10; idx += 1)
  {
    EXPECT_EQ(counter.Get(idx), 0);
    counter.Increment(idx, 1);
    EXPECT_EQ(counter.Get(idx), 1);
    counter.Increment(idx, GetParam());
    EXPECT_EQ(counter.Get(idx), GetParam() + 1);
  }
}

TEST_P(AdaptingIntegerArrayTest, Copy)
{
  AdaptingIntegerArray counter(10);
  counter.Increment(0, GetParam());
  EXPECT_EQ(counter.Get(0), GetParam());

  AdaptingIntegerArray copy = counter;
  EXPECT_EQ(copy.Get(0), GetParam());

  counter.Increment(0, 1);
  EXPECT_EQ(counter.Get(0), GetParam() + 1);
  EXPECT_EQ(copy.Get(0), GetParam());
}

TEST_P(AdaptingIntegerArrayTest, Clear)
{
  AdaptingIntegerArray counter(10);
  counter.Increment(0, GetParam());
  EXPECT_EQ(counter.Get(0), GetParam());

  counter.Clear();
  counter.Increment(0, 1);
  EXPECT_EQ(counter.Get(0), 1);
}

class AdaptingCircularBufferCounterTest : public ::testing::Test
{
protected:
  AdaptingCircularBufferCounter newCounter(int maxBuckets)
  {
    return AdaptingCircularBufferCounter(maxBuckets);
  }
};

TEST_F(AdaptingCircularBufferCounterTest, ReturnsZeroOutsidePopulatedRange)
{
  AdaptingCircularBufferCounter counter{10};
  EXPECT_EQ(counter.Get(0), 0);
  EXPECT_EQ(counter.Get(100), 0);
  counter.Increment(2, 1);
  counter.Increment(99, 1);
  EXPECT_EQ(counter.Get(0), 0);
  EXPECT_EQ(counter.Get(100), 0);
}

TEST_F(AdaptingCircularBufferCounterTest, ExpandLower)
{
  AdaptingCircularBufferCounter counter{160};
  EXPECT_TRUE(counter.Increment(10, 1));
  // Add BEFORE the initial see (array index 0) and make sure we wrap around the datastructure.
  EXPECT_TRUE(counter.Increment(0, 1));
  EXPECT_EQ(counter.Get(10), 1);
  EXPECT_EQ(counter.Get(0), 1);
  EXPECT_EQ(counter.StartIndex(), 0);
  EXPECT_EQ(counter.EndIndex(), 10);
  // Add AFTER initial entry and just push back end.
  EXPECT_TRUE(counter.Increment(20, 1));
  EXPECT_EQ(counter.Get(20), 1);
  EXPECT_EQ(counter.Get(10), 1);
  EXPECT_EQ(counter.Get(0), 1);
  EXPECT_EQ(counter.StartIndex(), 0);
  EXPECT_EQ(counter.EndIndex(), 20);
}

TEST_F(AdaptingCircularBufferCounterTest, ShouldFailAtLimit)
{
  AdaptingCircularBufferCounter counter{160};
  EXPECT_TRUE(counter.Increment(0, 1));
  EXPECT_TRUE(counter.Increment(120, 1));
  // Check state
  EXPECT_EQ(counter.StartIndex(), 0);
  EXPECT_EQ(counter.EndIndex(), 120);
  EXPECT_EQ(counter.Get(0), 1);
  EXPECT_EQ(counter.Get(120), 1);
  // Adding over the maximum # of buckets
  EXPECT_FALSE(counter.Increment(3000, 1));
}

TEST_F(AdaptingCircularBufferCounterTest, ShouldCopyCounters)
{
  AdaptingCircularBufferCounter counter{2};
  EXPECT_TRUE(counter.Increment(2, 1));
  EXPECT_TRUE(counter.Increment(1, 1));
  EXPECT_FALSE(counter.Increment(3, 1));

  AdaptingCircularBufferCounter copy{counter};
  EXPECT_EQ(counter.Get(2), 1);
  EXPECT_EQ(copy.Get(2), 1);
  EXPECT_EQ(copy.MaxSize(), counter.MaxSize());
  EXPECT_EQ(copy.StartIndex(), counter.StartIndex());
  EXPECT_EQ(copy.EndIndex(), counter.EndIndex());
  // Mutate copy and make sure original is unchanged.
  EXPECT_TRUE(copy.Increment(2, 1));
  EXPECT_EQ(copy.Get(2), 2);
  EXPECT_EQ(counter.Get(2), 1);
}