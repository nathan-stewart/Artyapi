#include <gtest/gtest.h>
#include <vector>
#include <boost/circular_buffer.hpp>
#include <algorithm>
#include <random>
#include "../src/AudioProcessor.h"

TEST(CircularBufferTest, Roll)
{
    boost::circular_buffer<float> buffer(128);
    std::vector<float> zeros = std::vector<float>(64, 0.0f);
    std::vector<float> ones =  std::vector<float>(184, 1.0f);

    buffer.insert(buffer.end(), zeros.begin(), zeros.end());
    ASSERT_EQ(buffer.size(), 64);
    ASSERT_EQ(std::accumulate(buffer.begin(), buffer.end(), 0.0f), 0.0f);

    buffer.insert(buffer.end(), ones.begin(), ones.end());
    ASSERT_EQ(buffer.size(), 128);
    ASSERT_EQ(std::accumulate(buffer.begin(), buffer.end(), 0.0f), 128.0f);

    buffer.insert(buffer.end(), zeros.begin(), zeros.end());
    ASSERT_EQ(buffer.size(), 128);
    ASSERT_EQ(std::accumulate(buffer.begin(), buffer.end(), 0.0f), 64.0f);
}
