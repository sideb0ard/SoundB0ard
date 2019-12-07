#include <iostream>

#include "gtest/gtest.h"
//
// #include "evaluator_test.cpp"
//#include "lexer_test.cpp"
//#include "parser_test.cpp"

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    std::cout << "YARLY!\n";
    return RUN_ALL_TESTS();
}
