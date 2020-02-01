#include <variant>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "interpreter/evaluator.hpp"
#include "interpreter/lexer.hpp"
#include "interpreter/object.hpp"
#include "interpreter/parser.hpp"
#include "interpreter/token.hpp"

namespace
{

struct EvaluatorTest : public ::testing::Test
{
};

bool TestNumberObject(std::shared_ptr<object::Object> obj, int64_t expected)
{
    std::shared_ptr<object::Number> io =
        std::dynamic_pointer_cast<object::Number>(obj);
    if (!io)
    {
        std::cerr << "NOT AN INTEGER OBJECT\n";
        return false;
    }

    if (io->value_ != expected)
    {
        std::cerr << "TEST INTEGER OBJECT - val not correct - actual:"
                  << io->value_ << " // expected:" << expected << std::endl;
        return false;
    }
    return true;
}

bool TestBooleanObject(std::shared_ptr<object::Object> obj, bool expected)
{
    std::shared_ptr<object::Boolean> bo =
        std::dynamic_pointer_cast<object::Boolean>(obj);
    if (!bo)
        return false;

    std::cout << "Testing Boolean! - object val:"
              << (bo->value_ ? "true" : "false")
              << " // Expected:" << (expected ? "true" : "false") << std::endl;
    if (bo->value_ != expected)
    {
        std::cerr << "TEST BOOLEAN OBJECT - val not correct - actual:"
                  << (bo->value_ ? "true" : "false")
                  << " // expected:" << (expected ? "true" : "false")
                  << std::endl;
        return false;
    }
    return true;
}

std::shared_ptr<object::Object> TestEval(std::string input)
{
    auto lex = std::make_unique<lexer::Lexer>(input);
    auto parsley = std::make_unique<parser::Parser>(std::move(lex));
    EXPECT_FALSE(parsley->CheckErrors());

    auto program = parsley->ParseProgram();
    std::cout << "Program has " << program->statements_.size() << " statements"
              << std::endl;
    auto env = std::make_shared<object::Environment>();
    return evaluator::Eval(program, env);
}

TEST_F(EvaluatorTest, TestNumberExpression)
{
    struct TestCase
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCase> tests{{"5", 5},
                                {"10", 10},
                                {"-5", -5},
                                {"-10", -10},
                                {"5 + 5 + 5 + 5 - 10", 10},
                                {"2 * 2 * 2 * 2 * 2", 32},
                                {"-50 + 100 + -50", 0},
                                {"5 * 2 + 10", 20},
                                {"5 + 2 * 10", 25},
                                {"20 + 2 * -10 ", 0},
                                {"50 / 2 * 2 + 10", 60},
                                {"2 * (5 + 10)", 30},
                                {"3 * 3  * 3 + 10", 37},
                                {"3 * (3 * 3) + 10", 37},
                                {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50}};
    for (auto tt : tests)
    {
        std::cout << "\nTesting! input: " << tt.input << std::endl;
        auto evaluated = TestEval(tt.input);
        EXPECT_TRUE(TestNumberObject(evaluated, tt.expected));
    }
}

TEST_F(EvaluatorTest, TestString)
{
    std::string input = R"("Hello World!")";
    std::shared_ptr<object::Object> evaluated = TestEval(input);
    std::shared_ptr<object::String> sliteral =
        std::dynamic_pointer_cast<object::String>(evaluated);
    if (!sliteral)
    {
        FAIL() << "Not a StringLiteral - got " << typeid(evaluated).name();
    }
    EXPECT_EQ(sliteral->value_, "Hello World!");
}

TEST_F(EvaluatorTest, TestBooleanExpression)
{
    struct TestCase
    {
        std::string input;
        bool expected;
    };
    std::vector<TestCase> tests{{"true", true},
                                {"false", false},
                                {"1 < 2", true},
                                {"1 > 2", false},
                                {"1 < 1", false},
                                {"1 > 1", false},
                                {"1 == 1", true},
                                {"1 != 1", false},
                                {"1 == 2", false},
                                {"1 != 2", true},
                                {"true == true", true},
                                {"false == false", true},
                                {"true == false", false},
                                {"true != false", true},
                                {"false != true", true},
                                {"(1<2) == true", true},
                                {"(1<2) == false", false},
                                {"(1 > 2) == true", false},
                                {"(1 > 2) == false", true}};
    for (auto tt : tests)
    {
        std::cout << "\nTesting! input: " << tt.input << std::endl;
        auto evaluated = TestEval(tt.input);
        EXPECT_TRUE(TestBooleanObject(evaluated, tt.expected));
    }
}

TEST_F(EvaluatorTest, TestBangOperator)
{
    struct TestCase
    {
        std::string input;
        bool expected;
    };
    std::vector<TestCase> tests{{"!true", false},   {"!false", true},
                                {"!5", false},      {"!!true", true},
                                {"!!false", false}, {"!!5", true}};
    for (auto tt : tests)
    {
        std::cout << "\nTesting! input: " << tt.input << std::endl;
        auto evaluated = TestEval(tt.input);
        EXPECT_TRUE(TestBooleanObject(evaluated, tt.expected));
    }
}

TEST_F(EvaluatorTest, TestIfElseExpression)
{
    std::cout << "TESYTES" << std::endl;
    struct TestCaseInt
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCaseInt> tests{{"if (true) { 10 }", 10},
                                   {"if (1) { 10 }", 10},
                                   {"if (1 < 2) { 10 }", 10},
                                   {"if (1 > 2) { 10 } else { 20 }", 20},
                                   {"if (1 < 2) { 10 } else { 20 }", 10}};
    for (auto tt : tests)
    {
        std::cout << "Nout...\n";
        // std::cout << "\nTesting! input: " << tt.input << std::endl;
        // auto evaluated = TestEval(tt.input);

        // EXPECT_TRUE(TestNumberObject(evaluated, tt.expected));
    }

    //////////////////

    std::vector<std::string> nullTests{"if (false) { 10 }",
                                       "if (1 > 2) { 10 }"};
    for (auto tt : nullTests)
    {
        std::cout << "\nTesting! input: " << tt << std::endl;
        std::shared_ptr<object::Object> evaluated = TestEval(tt);
        EXPECT_EQ(evaluated, evaluator::NULLL);
    }
}

TEST_F(EvaluatorTest, TestReturnStatements)
{
    struct TestCase
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCase> tests{
        {"return 10;", 10},
        {"return 10; 9;", 10},
        {"return 2 * 5; 9;", 10},
        {"9; return 2 * 5; 9", 10},
        {"if (10 > 1) { if (10 > 1) { return 10;} } return 1;}", 10}};
    for (auto tt : tests)
    {
        std::cout << "\nTesting! input: " << tt.input << std::endl;
        auto evaluated = TestEval(tt.input);
        EXPECT_TRUE(TestNumberObject(evaluated, tt.expected));
    }
}

TEST_F(EvaluatorTest, TestErrorHandling)
{
    struct TestCase
    {
        std::string input;
        std::string expected;
    };
    std::vector<TestCase> tests{
        {"5 + true;", "type mismatch: INTEGER + BOOLEAN"},
        {"5 + true; 5", "type mismatch: INTEGER + BOOLEAN"},
        {"-true", "unknown operator: -BOOLEAN"},
        {"true + false;", "unknown operator: BOOLEAN + BOOLEAN"},
        {"5; true + false; 5", "unknown operator: BOOLEAN + BOOLEAN"},
        {"if (10 > 1) { true + false; }",
         "unknown operator: BOOLEAN + BOOLEAN"},
        {"if (10 > 1) { if (10 > 1) { true + false; } return 1;}",
         "unknown operator: BOOLEAN + BOOLEAN"},
        {"foobar", "identifier not found: foobar"},
        {R"("hello" - "world")", "unknown operator: STRING - STRING"}};

    for (auto tt : tests)
    {
        std::cout << "Testing input:" << tt.input << std::endl;
        auto evaluated = TestEval(tt.input);
        std::shared_ptr<object::Error> err_obj =
            std::dynamic_pointer_cast<object::Error>(evaluated);
        EXPECT_TRUE(err_obj);
        if (!err_obj)
        {
            std::cerr << "No Error object returned. Got "
                      << typeid(evaluated).name() << "\n\n";
            continue;
        }
        EXPECT_EQ(err_obj->message_, tt.expected);
    }
}

TEST_F(EvaluatorTest, TestLetStatements)
{
    struct TestCase
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCase> tests{
        {"let a = 5; a;", 5},
        {"let a = 5 * 5; a;", 25},
        {"let a = 5; let b = a; b;", 5},
        {"let a = 5; let b = a; let c = a + b + 5; c;", 15}};

    for (auto tt : tests)
    {
        EXPECT_TRUE(TestNumberObject(TestEval(tt.input), tt.expected));
    }
}

TEST_F(EvaluatorTest, TestFunctionObject)
{
    auto input = "fn(x) { x + 2; };";
    auto evaluated = TestEval(input);
    std::shared_ptr<object::Function> fn =
        std::dynamic_pointer_cast<object::Function>(evaluated);
    EXPECT_TRUE(fn);
    if (!fn)
    {
        FAIL() << "Object is not a function. Got " << typeid(evaluated).name()
               << "\n\n";
    }
    EXPECT_EQ(fn->parameters_.size(), 1);
    EXPECT_EQ(fn->parameters_[0]->String(), "x");
    EXPECT_EQ(fn->body_->String(), "(x+2)");
}

TEST_F(EvaluatorTest, TestFunctionApplication)
{
    struct TestCase
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCase> tests{
        {"let identity = fn(x) { x; }; identity(5);", 5},
        {"let identity = fn(x) { return x; }; identity(5);", 5},
        {"let add = fn(x,y) { x + y; }; add(5,5);", 10},
        {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5,5));", 20},
        {"fn(x) {x;}(5)", 5}};

    for (auto tt : tests)
    {
        EXPECT_TRUE(TestNumberObject(TestEval(tt.input), tt.expected));
    }
}

TEST_F(EvaluatorTest, TestClosures)
{
    auto input = R"(let newAdder = fn(x) {
  fn(y) { x + y; };
};

 let addTwo = newAdder(2);
 addTwo(2);)";

    TestNumberObject(TestEval(input), 4);
}

TEST_F(EvaluatorTest, TestStringConcatentation)
{
    auto input = R"("Hello" + " " + "World!")";
    std::shared_ptr<object::Object> evaluated = TestEval(input);
    std::shared_ptr<object::String> str_obj =
        std::dynamic_pointer_cast<object::String>(evaluated);
    if (!str_obj)
    {
        FAIL() << "Object is not a String. Got " << typeid(evaluated).name()
               << "\n\n";
    }

    EXPECT_EQ(str_obj->value_, "Hello World!");
}

TEST_F(EvaluatorTest, TestInBuiltFunctions)
{
    struct TestCaseInt
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCaseInt> tests{{R"(len(""))", 0},
                                   {R"(len("four"))", 4},
                                   {R"(len("hello wurld"))", 11}};
    for (auto &tt : tests)
    {
        EXPECT_TRUE(TestNumberObject(TestEval(tt.input), tt.expected));
    }
}

TEST_F(EvaluatorTest, TestArrayLiterals)
{
    auto input = "[1, 2 * 2, 3 + 3]";
    std::shared_ptr<object::Object> evaluated = TestEval(input);
    std::shared_ptr<object::Array> array_obj =
        std::dynamic_pointer_cast<object::Array>(evaluated);
    if (!array_obj)
    {
        FAIL() << "Object is not an Array. Got " << typeid(evaluated).name()
               << "\n\n";
    }

    ASSERT_EQ(array_obj->elements_.size(), 3);

    EXPECT_TRUE(TestNumberObject(array_obj->elements_[0], 1));
    EXPECT_TRUE(TestNumberObject(array_obj->elements_[1], 4));
    EXPECT_TRUE(TestNumberObject(array_obj->elements_[2], 6));
}

TEST_F(EvaluatorTest, TestArrayIndexExpressions)
{
    struct TestCase
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCase> tests{
        {"[1, 2, 3][0]", 1},
        {"[1, 2, 3][1]", 2},
        {"[1, 2, 3][2]", 3},
        {"let i = 0; [1][i];", 1},
        {"[1, 2, 3][1 + 1]", 3},
        {"let myArray = [1, 2, 3]; myArray[2];", 3},
        {"let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];", 6},
        {"let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i]", 2}};

    for (auto &tt : tests)
    {
        auto evaluated = TestEval(tt.input);
        EXPECT_TRUE(TestNumberObject(evaluated, tt.expected));
    }

    std::vector<std::string> nullTests{"[1, 2, 3][3]", "[1, 2, 3][-1]"};
    for (auto tt : nullTests)
    {
        std::cout << "\nTesting! input: " << tt << std::endl;
        auto evaluated = TestEval(tt);
        EXPECT_EQ(evaluated, evaluator::NULLL);
    }
}

TEST_F(EvaluatorTest, TestBuiltInFunctions)
{
    struct TestCase
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCase> tests{
        {R"(len(""))", 0},
        {R"(len("four"))", 4},
        {R"(len("hello world"))", 11},
        {R"(len([1, 2, 3]))", 3},
        {R"(len([]))", 0},
        {R"(head([1, 2, 3]))", 1},
        {R"(last([1, 2, 3]))", 3},
    };

    for (auto &tt : tests)
    {
        auto evaluated = TestEval(tt.input);
        EXPECT_TRUE(TestNumberObject(evaluated, tt.expected));
    }

    /////
    /////////////////////////////

    struct TestCaseString
    {
        std::string input;
        std::string expected;
    };

    std::vector<TestCaseString> teststrings{
        {R"(len(1))", "argument to `len` not supported, got INTEGER"},
        {R"(len("one", "two"))",
         "Too many arguments for len - can only accept one"},
        {R"(head(1))", "argument to `head` must be an array - got INTEGER"},
        {R"(last(1))", "argument to `last` must be an array - got INTEGER"},
        {R"(push(1, 1))", "argument to `push` must be an array - got INTEGER"},
    };

    for (auto &tt : teststrings)
    {
        auto evaluated = TestEval(tt.input);
        std::shared_ptr<object::Error> err_obj =
            std::dynamic_pointer_cast<object::Error>(evaluated);
        if (!err_obj)
            FAIL() << "Object is not Error - got " << typeid(evaluated).name();

        EXPECT_EQ(tt.expected, err_obj->message_);
    }

    ////////////////////////////

    struct TestCaseArray
    {
        std::string input;
        std::vector<int64_t> expected;
    };

    std::vector<TestCaseArray> test_arrays{
        {R"(tail([1, 2, 3]))", {2, 3}},
        {R"(push([], 1))", {1}},
    };

    for (auto &tt : test_arrays)
    {
        auto evaluated = TestEval(tt.input);
        std::shared_ptr<object::Array> arr_obj =
            std::dynamic_pointer_cast<object::Array>(evaluated);
        if (!arr_obj)
            FAIL() << "Object is not Array - got " << typeid(evaluated).name();

        ASSERT_EQ(tt.expected.size(), arr_obj->elements_.size());
        int elems_size = arr_obj->elements_.size();
        for (int i = 0; i < elems_size; i++)
            TestNumberObject(arr_obj->elements_[i], tt.expected[i]);
    }

    /////////////////////////////

    std::vector<std::string> nullTests{
        R"(head([]))",
        R"(tail([]))",
        R"(last([]))",
        // R"(puts("hello", "world!"))",
    };

    for (auto tt : nullTests)
    {
        std::cout << "\nTesting! input: " << tt << std::endl;
        auto evaluated = TestEval(tt);
        EXPECT_EQ(evaluated, evaluator::NULLL);
    }
}

TEST_F(EvaluatorTest, TestForLoop)
{
    struct TestCase
    {
        std::string input;
        std::string expected;
    };

    std::vector<TestCase> tests{
        {R"(for (i = 0; i < 5; ++i) { puts(i); i; })", "5"},
        {R"(for (i = 5; i > 0; --i) { puts(i); i; })", "0"},
        {R"(let i = 7; let x = 10; for (i = 5; i > 0; --i) { let x = x +
        i; puts(x); }; i)",
         "7"},
        {R"(let x = 10; for (i = 5; i > 0; --i) { let x = x + i; puts(x);
        i;
        }; i;)",
         "ERROR: identifier not found: i"},
        {R"(let x = 10; for (i = 5; i > 0; --i) { let x = x + x; puts(x);
        x;
        })",
         "320"},
    };

    for (auto &tt : tests)
    {
        std::shared_ptr<object::Object> evaluated = TestEval(tt.input);
        EXPECT_EQ(evaluated->Inspect(), tt.expected);
    }
}

TEST_F(EvaluatorTest, TestHashLiterals)
{
    std::string input = R"(let two = "two";
{
    "one": 10 - 9,
    two: 1 + 1,
    "thr" + "ee": 6 / 2,
    4: 4,
    true: 5,
    false: 6
})";

    std::shared_ptr<object::Object> evaluated = TestEval(input);
    std::shared_ptr<object::Hash> hsh =
        std::dynamic_pointer_cast<object::Hash>(evaluated);
    if (!hsh)
        FAIL() << "Object is not a Hash. Got " << typeid(evaluated).name()
               << "\n\n";

    std::map<object::HashKey, int64_t> expected{
        {object::String("one").HashKey(), 1},
        {object::String("two").HashKey(), 2},
        {object::String("three").HashKey(), 3},
        {object::Number(4).HashKey(), 4},
        {evaluator::TTRUE->HashKey(), 5},
        {evaluator::FFALSE->HashKey(), 6},
    };

    if (hsh->pairs_.size() != expected.size())
    {
        hsh->Inspect();
        FAIL() << "Hash has wrong num of pairs. Got " << hsh->pairs_.size();
    }

    for (auto &e : expected)
    {
        auto expected_key = e.first;
        auto expected_value = e.second;

        auto p = hsh->pairs_.find(expected_key);
        if (p == hsh->pairs_.end())
            FAIL() << "No pair found for key in pairs...";

        TestNumberObject(p->second.value_, expected_value);
    }
}

TEST_F(EvaluatorTest, TestHashIndexExpression)
{
    struct TestCase
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCase> tests{
        {R"({"foo": 5}["foo"])", 5},
        {R"(let key = "foo"; {"foo":
        5}[key])",
         5},
        {R"({5: 5}[5])", 5},
        {R"({true: 5}[true])", 5},
        {R"({false: 5}[false])", 5},
    };

    for (auto &tt : tests)
    {
        auto evaluated = TestEval(tt.input);
        EXPECT_TRUE(TestNumberObject(evaluated, tt.expected));
    }

    std::vector<std::string> null_tests{
        {R"({}["foo"])"},
        {R"({"foo": 5}["bar"])"},
    };

    for (auto &tt : null_tests)
    {
        auto evaluated = TestEval(tt);
        EXPECT_EQ(evaluated, evaluator::NULLL);
    }
}

TEST_F(EvaluatorTest, TestIncrementOperators)
{
    struct TestCase
    {
        std::string input;
        int64_t expected;
    };
    std::vector<TestCase> tests{
        {"++3", 4},
        {"--3", 2},
        {"++0", 1},
        {"--0", -1},
        {"let i = 3; --i", 2},
        {"let i = 3; ++i", 4},
    };

    for (auto tt : tests)
    {
        EXPECT_TRUE(TestNumberObject(TestEval(tt.input), tt.expected));
    }
}

} // namespace
