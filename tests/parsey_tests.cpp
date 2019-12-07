#include <iostream>

#include "gtest/gtest.h"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "YARLY!\n";
    return RUN_ALL_TESTS();
}
