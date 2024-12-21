#include "../src/AudioProcessor.h"
#include <gtest/gtest.h>
#include <vector>
#include <boost/circular_buffer.hpp>

TEST(AudioProcessorTest, ComputeRMSAndPeak)
{
    boost::circular_buffer<float> data(4);
    for (float v : {1.0, 2.0, 3.0, 4.0})
        data.push_back(v);

    ASSERT_EQ(get_slice(data), (std::vector<float>{1.0, 2.0, 3.0, 4.0}));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}