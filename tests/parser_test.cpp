#include <variant>

#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "interpreter/lexer.hpp"
#include "interpreter/parser.hpp"
#include "interpreter/token.hpp"

namespace
{

struct ParserTest : public ::testing::Test
{
};

bool TestLetStatement(std::shared_ptr<ast::Statement> s, std::string name)
{
    std::cout << "Literal is " << s->TokenLiteral() << std::endl;

    if (s->TokenLiteral().compare("let") != 0)
        return false;

    std::cout << "All good!\n";

    std::cout << "s is of type " << typeid(s).name() << std::endl;

    std::shared_ptr<ast::LetStatement> ls =
        std::dynamic_pointer_cast<ast::LetStatement>(s);
    if (!ls)
        return false;
    std::cout << "CAST WAS All good!\n";

    if (ls->name_->value_.compare(name) != 0)
        return false;
    std::cout << "NAME WAS All good!\n";

    if (ls->name_->TokenLiteral().compare(name) != 0)
        return false;
    std::cout << "NAME LITERAL WAS All good!\n";

    return true;
}

bool TestForStatement(std::shared_ptr<ast::Statement> s)
{
    std::cout << "FOR STATEMENT is " << s->TokenLiteral() << std::endl;

    if (s->TokenLiteral().compare("for") != 0)
        return false;

    std::cout << "All good!\n";

    std::cout << "s is of type " << typeid(s).name() << std::endl;

    std::shared_ptr<ast::ForStatement> for_stmt =
        std::dynamic_pointer_cast<ast::ForStatement>(s);
    if (!for_stmt)
        return false;
    std::cout << "FOR CAST WAS All good!\n";

    return true;
}

bool TestIdentifier(std::shared_ptr<ast::Expression> expr, std::string val)
{
    std::shared_ptr<ast::Identifier> ident =
        std::dynamic_pointer_cast<ast::Identifier>(expr);
    if (!ident)
        return false;
    if (ident->value_ != val)
        return false;
    if (ident->TokenLiteral() != val)
        return false;
    return true;
}

bool TestIntegerLiteral(std::shared_ptr<ast::Expression> expr, int64_t value)
{
    std::shared_ptr<ast::IntegerLiteral> integ =
        std::dynamic_pointer_cast<ast::IntegerLiteral>(expr);
    if (!integ)
        return false;
    if (integ->value_ != value)
        return false;
    if (integ->TokenLiteral() != std::to_string(value))
        return false;
    return true;
}

bool TestBooleanLiteral(std::shared_ptr<ast::Expression> expr, bool val)
{
    std::shared_ptr<ast::BooleanExpression> bool_expr =
        std::dynamic_pointer_cast<ast::BooleanExpression>(expr);
    if (!bool_expr)
    {
        std::cerr << "Not an BooleanExpression - got " << typeid(&expr).name()
                  << std::endl;
        return false;
    }
    std::cout << "TEST BOOOOL! " << bool_expr->String()
              << " actual:" << bool_expr->value_ << " // expected: " << val
              << std::endl;
    if (bool_expr->value_ != val)
        return false;

    return true;
}

bool TestLiteralExpression(std::shared_ptr<ast::Expression> expr,
                           std::variant<int64_t, std::string, bool> val)
{

    if (const auto int_ptr(std::get_if<int64_t>(&val)); int_ptr)
    {
        std::cout << "int!" << *int_ptr << "\n";
        return TestIntegerLiteral(expr, *int_ptr);
    }
    else if (const auto string_ptr(std::get_if<std::string>(&val)); string_ptr)
    {
        std::cout << "string!" << *string_ptr << "\n";
        return TestIdentifier(expr, *string_ptr);
    }
    else if (const auto bool_ptr(std::get_if<bool>(&val)); bool_ptr)
    {
        std::cout << "bool test val! " << *bool_ptr << "\n";
        return TestBooleanLiteral(expr, *bool_ptr);
    }
    return false;
}

bool TestInfixExpression(std::shared_ptr<ast::Expression> expr,
                         std::variant<int64_t, std::string, bool> left,
                         std::string op,
                         std::variant<int64_t, std::string, bool> right)
{
    std::shared_ptr<ast::InfixExpression> op_expr =
        std::dynamic_pointer_cast<ast::InfixExpression>(expr);
    if (!op_expr)
    {
        std::cerr << "Not an InfixExpression - got " << typeid(&expr).name();
        return false;
    }

    if (!TestLiteralExpression(op_expr->left_, left))
    {
        std::cerr << "LEFT IS not a Literal Expression!\n";
        return false;
    }

    if (op_expr->operator_.compare(op) != 0)
    {
        std::cerr << "OP IS not a operator!\n";
        return false;
    }

    if (!TestLiteralExpression(op_expr->right_, right))
    {
        std::cerr << "RIGHT IS not a Literal Expression!\n";
        return false;
    }

    return true;
}

TEST_F(ParserTest, TestLetStatements)
{
    struct TestCase
    {
        std::string input;
        std::string expected_ident;
        std::variant<int64_t, std::string, bool> expected_val;
    };
    using namespace std::string_literals;
    std::vector<TestCase> tests = {
        {"let x = 5;", "x", (int64_t)5},
        {"let y = true;", "y", true},
        {"let foobar = y;", "foobar", "y"s},
    };

    for (auto &tt : tests)
    {

        std::cout << "Parsey Test setup!\n";
        std::unique_ptr<lexer::Lexer> lex =
            std::make_unique<lexer::Lexer>(tt.input);
        std::unique_ptr<parser::Parser> parsley =
            std::make_unique<parser::Parser>(std::move(lex));
        std::shared_ptr<ast::Program> program = parsley->ParseProgram();
        EXPECT_FALSE(parsley->CheckErrors());

        EXPECT_TRUE(
            TestLetStatement(program->statements_[0], tt.expected_ident));

        std::shared_ptr<ast::LetStatement> stmt =
            std::dynamic_pointer_cast<ast::LetStatement>(
                program->statements_[0]);
        if (!stmt)
            FAIL() << "program->statements_[0] is not an LetStatement";

        EXPECT_TRUE(TestLiteralExpression(stmt->value_, tt.expected_val));
    }
}

TEST_F(ParserTest, TestForStatements)
{
    struct TestCase
    {
        std::string input;
        std::string expected_it_ident;
        int64_t expected_it_val;
        std::string term_condition;
        std::string increment;
    };
    using namespace std::string_literals;
    std::vector<TestCase> tests = {
        {"for (i = 0; i < 5; ++i) { puts(i); }", "i", (int64_t)0, "(i<5)",
         "(++i)"},
        {"for (j = 5; j > 0; --j) { puts(j); }", "j", (int64_t)5, "(j>0)",
         "(--j)"},
    };

    for (auto &tt : tests)
    {

        std::cout << "Parsey FOR Test setup!\n";
        std::cout << "Testing: " << tt.input << std::endl;

        std::unique_ptr<lexer::Lexer> lex =
            std::make_unique<lexer::Lexer>(tt.input);
        std::unique_ptr<parser::Parser> parsley =
            std::make_unique<parser::Parser>(std::move(lex));
        std::shared_ptr<ast::Program> program = parsley->ParseProgram();
        ASSERT_FALSE(parsley->CheckErrors());

        EXPECT_TRUE(TestForStatement(program->statements_[0]));

        std::shared_ptr<ast::ForStatement> stmt =
            std::dynamic_pointer_cast<ast::ForStatement>(
                program->statements_[0]);
        if (!stmt)
            FAIL() << "program->statements_[0] is not an ForStatement";

        std::cout << "FOOOOR!:" << stmt->String() << std::endl;

        EXPECT_TRUE(TestIdentifier(stmt->iterator_, tt.expected_it_ident));
        EXPECT_TRUE(
            TestLiteralExpression(stmt->iterator_value_, tt.expected_it_val));

        EXPECT_EQ(stmt->termination_condition_->String(), tt.term_condition);
        EXPECT_EQ(stmt->increment_->String(), tt.increment);
    }
}

TEST_F(ParserTest, TestReturnStatements)
{
    struct TestCase
    {
        std::string input;
        std::variant<int64_t, std::string, bool> expected_val;
    };
    TestCase t1{"return 5;", (int64_t)5};
    TestCase t2{"return true;", true};
    TestCase t3{"return foobar;", "foobar"};
    std::vector<TestCase> tests{t1, t2};
    // std::vector<TestCase> tests{t1, t2, t3};

    for (auto tt : tests)
    {
        std::unique_ptr<lexer::Lexer> lex =
            std::make_unique<lexer::Lexer>(tt.input);
        std::unique_ptr<parser::Parser> parsley =
            std::make_unique<parser::Parser>(std::move(lex));
        std::shared_ptr<ast::Program> program = parsley->ParseProgram();
        EXPECT_FALSE(parsley->CheckErrors());

        ASSERT_EQ(1, program->statements_.size());
        std::shared_ptr<ast::ReturnStatement> stmt =
            std::dynamic_pointer_cast<ast::ReturnStatement>(
                program->statements_[0]);
        if (!stmt)
            FAIL() << "program->statements_[0] is not an ReturnStatement";

        auto print_visitor = [](auto &&arg) { std::cout << arg; };
        ASSERT_EQ(stmt->TokenLiteral(), "return");
        std::cout << "RET VAL is" << stmt->return_value_ << " EXpected:";
        std::visit(print_visitor, tt.expected_val);
        std::cout << std::endl;
        ASSERT_TRUE(
            TestLiteralExpression(stmt->return_value_, tt.expected_val));
    }
}

TEST_F(ParserTest, TestStringLiteralExpression)
{
    std::string input = R"("hello world";)";
    std::shared_ptr<lexer::Lexer> lex = std::make_shared<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(lex);
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());
    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::StringLiteral> literal =
        std::dynamic_pointer_cast<ast::StringLiteral>(stmt->expression_);
    if (!literal)
        FAIL() << "program->statements_[0] is not a StringLiteral. Got "
               << typeid(&stmt->expression_).name();

    if (literal->value_.compare("hello world") != 0)
        FAIL() << "literal.value_ is not " << input << ". Got "
               << literal->value_ << std::endl;
}

TEST_F(ParserTest, TestString)
{
    std::string input = R"(let myVar = anotherVar;)";

    token::Token toke{token::SLANG_LET, "let"};
    auto stmt = std::make_shared<ast::LetStatement>(toke);
    token::Token name{token::SLANG_IDENT, "myVar"};
    stmt->name_ = std::make_shared<ast::Identifier>(name, "myVar");
    token::Token val{token::SLANG_IDENT, "anotherVar"};
    stmt->value_ = std::make_shared<ast::Identifier>(val, "anotherVar");

    std::unique_ptr<ast::Program> program = std::make_unique<ast::Program>();
    program->statements_.push_back(stmt);

    ASSERT_EQ(input, program->String());
}

TEST_F(ParserTest, TestIdentifierExpression)
{
    std::string input = "foobar";
    std::shared_ptr<lexer::Lexer> lex = std::make_shared<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(lex);
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());

    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::Identifier> ident =
        std::dynamic_pointer_cast<ast::Identifier>(stmt->expression_);
    if (!ident)
        FAIL() << "Not an Identifier - got "
               << typeid(&stmt->expression_).name();
    ASSERT_EQ(ident->value_, "foobar");
    ASSERT_EQ(ident->TokenLiteral(), "foobar");
}

TEST_F(ParserTest, TestIfExpression)
{
    std::string input = "if (x < y) { x }";
    std::cout << "IFFF! -- " << input << std::endl;
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());

    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::IfExpression> expr =
        std::dynamic_pointer_cast<ast::IfExpression>(stmt->expression_);
    if (!expr)
        FAIL() << "Not an IfExpression - got "
               << typeid(&stmt->expression_).name();
    std::cout << "\nTESTING FOR INFIX..\n";
    using namespace std::string_literals;
    std::variant<int64_t, std::string, bool> left = "x"s;
    std::variant<int64_t, std::string, bool> right = "y"s;
    ASSERT_TRUE(TestInfixExpression(expr->condition_, left, "<", right));

    ASSERT_EQ(1, expr->consequence_->statements_.size());
    std::shared_ptr<ast::ExpressionStatement> consequence =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            expr->consequence_->statements_[0]);
    if (!consequence)
        FAIL() << "Not an Expression Statement! - got "
               << typeid(expr->consequence_->statements_[0]).name();

    if (!TestIdentifier(consequence->expression_, "x"))
        FAIL() << "Not an IdentifierExpression!\n";

    if (expr->alternative_ != nullptr)
        FAIL() << "Expression Alternative wasn't null!\n";
}

TEST_F(ParserTest, TestIfElseExpression)
{
    std::string input = "if (x < y) { x } else { y }";
    std::cout << "IFFF ELSE! -- " << input << std::endl;
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());

    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::IfExpression> expr =
        std::dynamic_pointer_cast<ast::IfExpression>(stmt->expression_);
    if (!expr)
        FAIL() << "Not an IfExpression - got "
               << typeid(&stmt->expression_).name();
    std::cout << "\nTESTING FOR INFIX..\n";
    using namespace std::string_literals;
    std::variant<int64_t, std::string, bool> left = "x"s;
    std::variant<int64_t, std::string, bool> right = "y"s;
    ASSERT_TRUE(TestInfixExpression(expr->condition_, left, "<", right));

    ASSERT_EQ(1, expr->consequence_->statements_.size());

    std::shared_ptr<ast::ExpressionStatement> consequence =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            expr->consequence_->statements_[0]);
    if (!consequence)
        FAIL() << "Not an Expression Statement! - got "
               << typeid(expr->consequence_->statements_[0]).name();

    if (!TestIdentifier(consequence->expression_, "x"))
        FAIL() << "Not an IdentifierExpression!\n";

    ASSERT_EQ(1, expr->alternative_->statements_.size());
    std::shared_ptr<ast::ExpressionStatement> alternative =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            expr->alternative_->statements_[0]);
    if (!alternative)
        FAIL() << "Not an Expression Statement! - got "
               << typeid(expr->alternative_->statements_[0]).name();

    if (!TestIdentifier(alternative->expression_, "y"))
        FAIL() << "Not an IdentifierExpression!\n";
}
TEST_F(ParserTest, TestIntegerLiteralExpression)
{
    std::string input = "5";
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());

    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program_->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::IntegerLiteral> literal =
        std::dynamic_pointer_cast<ast::IntegerLiteral>(stmt->expression_);
    if (!literal)
        FAIL() << "Not an IntegerLiteral - got "
               << typeid(&stmt->expression_).name();
    ASSERT_EQ(literal->value_, 5);
    ASSERT_EQ(literal->TokenLiteral(), "5");
}

TEST_F(ParserTest, TestPrefixExpressions)
{

    struct TestCase
    {
        std::string input;
        std::string op;
        std::variant<int64_t, std::string, bool> value;
    };
    std::vector<TestCase> prefix_tests{
        {"!5", "!", (int64_t)5},     {"-15", "-", (int64_t)15},
        {"++15", "++", (int64_t)15}, {"--15", "--", (int64_t)15},
        {"!true;", "!", true},       {"!false;", "!", false},
    };
    for (auto tt : prefix_tests)
    {
        std::unique_ptr<lexer::Lexer> lex =
            std::make_unique<lexer::Lexer>(tt.input);
        std::unique_ptr<parser::Parser> parsley =
            std::make_unique<parser::Parser>(std::move(lex));
        std::shared_ptr<ast::Program> program = parsley->ParseProgram();
        EXPECT_FALSE(parsley->CheckErrors());
        ASSERT_EQ(1, program->statements_.size());

        std::shared_ptr<ast::ExpressionStatement> stmt =
            std::dynamic_pointer_cast<ast::ExpressionStatement>(
                program->statements_[0]);
        if (!stmt)
            FAIL() << "program_->statements_[0] is not an ExpressionStatement ";

        std::shared_ptr<ast::PrefixExpression> expr =
            std::dynamic_pointer_cast<ast::PrefixExpression>(stmt->expression_);
        if (!expr)
            FAIL() << "Not a Prefix Expression - got "
                   << typeid(&stmt->expression_).name();
        ASSERT_EQ(expr->operator_, tt.op);
        ASSERT_TRUE(TestLiteralExpression(expr->right_, tt.value));

        std::cout << program->String() << std::endl;
    }
}

TEST_F(ParserTest, TestInfixExpressions)
{
    struct TestCase
    {
        TestCase(std::string inp, std::variant<int64_t, std::string, bool> lft,
                 std::string opr, std::variant<int64_t, std::string, bool> rght)

            : input{inp}, left_value{lft}, op{opr}, right_value{rght}
        {
        }
        std::string input;
        std::variant<int64_t, std::string, bool> left_value;
        std::string op;
        std::variant<int64_t, std::string, bool> right_value;
    };
    std::vector<TestCase> infix_tests{{"5+5", (int64_t)5, "+", (int64_t)5},
                                      {"5 - 5", (int64_t)5, "-", (int64_t)5},
                                      {"5 * 5", (int64_t)5, "*", (int64_t)5},
                                      {"5 / 5", (int64_t)5, "/", (int64_t)5},
                                      {"5 > 5", (int64_t)5, ">", (int64_t)5},
                                      {"5 < 5", (int64_t)5, "<", (int64_t)5},
                                      {"5 == 5", (int64_t)5, "==", (int64_t)5},
                                      {"5 != 5", (int64_t)5, "!=", (int64_t)5},
                                      {"true == true", true, "==", true},
                                      {"true != false", true, "!=", false},
                                      {"false == false", false, "==", false}};

    for (auto tt : infix_tests)
    {
        std::cout << "\n\nTesting with input = " << tt.input << std::endl;
        std::unique_ptr<lexer::Lexer> lex =
            std::make_unique<lexer::Lexer>(tt.input);
        std::unique_ptr<parser::Parser> parsley =
            std::make_unique<parser::Parser>(std::move(lex));
        std::shared_ptr<ast::Program> program = parsley->ParseProgram();
        EXPECT_FALSE(parsley->CheckErrors());

        std::shared_ptr<ast::ExpressionStatement> stmt =
            std::dynamic_pointer_cast<ast::ExpressionStatement>(
                program->statements_[0]);
        if (!stmt)
            FAIL() << "program_->statements_[0] is not an ExpressionStatement ";

        std::variant<int64_t, std::string, bool> left = tt.left_value;
        std::variant<int64_t, std::string, bool> right = tt.right_value;
        ASSERT_TRUE(TestInfixExpression(stmt->expression_, left, tt.op, right));

        // std::cout << program->String() << std::endl;
    }
} // namespace
TEST_F(ParserTest, TestOperatorPrecedence)
{
    struct TestCase
    {
        std::string input;
        std::string expected;
    };
    std::vector<TestCase> tests{
        {"-a * b", "((-a)*b)"},
        {"!-a", "(!(-a))"},
        {"a + b + c", "((a+b)+c)"},
        {"a + b - c", "((a+b)-c)"},
        {"a * b * c", "((a*b)*c)"},
        {"a * b / c", "((a*b)/c)"},
        {"a + b / c", "(a+(b/c))"},
        {"a + b * c + d / e - f", "(((a+(b*c))+(d/e))-f)"},
        {"3 + 4; -5 * 5", "(3+4)((-5)*5)"},
        {"5 > 5 == 3 < 4", "((5>5)==(3<4))"},
        {"5 < 4 != 3 > 4", "((5<4)!=(3>4))"},
        {"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3+(4*5))==((3*1)+(4*5)))"},
        {"true", "true"},
        {"false", "false"},
        {"3 > 5 == false", "((3>5)==false)"},
        {"3 < 5 == true", "((3<5)==true)"},
        {"1 + (2 + 3) + 4", "((1+(2+3))+4)"},
        {"(5 + 5) * 2", "((5+5)*2)"},
        {"2/(5+5)", "(2/(5+5))"},
        {"-(5 + 5)", "(-(5+5))"},
        {"!(true == true)", "(!(true==true))"},
        {"a * [1, 2, 3, 4][b * c] * d", "((a*([1, 2, 3, 4][(b*c)]))*d)"},
        {"add(a * b[2], b[1], 2 * [1, 2][1])",
         "add((a*(b[2])), (b[1]), (2*([1, 2][1])))"}};

    for (auto tt : tests)
    {
        std::cout << "\n\nTesting with input = " << tt.input << std::endl;
        std::unique_ptr<lexer::Lexer> lex =
            std::make_unique<lexer::Lexer>(tt.input);
        std::unique_ptr<parser::Parser> parsley =
            std::make_unique<parser::Parser>(std::move(lex));
        std::shared_ptr<ast::Program> program = parsley->ParseProgram();
        EXPECT_FALSE(parsley->CheckErrors());

        auto actual = program->String();
        std::cout << program->String() << std::endl;
        ASSERT_EQ(actual, tt.expected);
    }
}

TEST_F(ParserTest, TestFunctionLiteral)
{
    std::string input = "fn(x, y) { x + y; }";
    std::cout << "FN LITERAL! -- " << input << std::endl;
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());

    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::FunctionLiteral> fnlit =
        std::dynamic_pointer_cast<ast::FunctionLiteral>(stmt->expression_);
    if (!fnlit)
        FAIL() << "Not an FunctionLiteral - got "
               << typeid(&stmt->expression_).name();

    ASSERT_EQ(2, fnlit->parameters_.size());

    using namespace std::string_literals;
    ASSERT_TRUE(TestLiteralExpression(fnlit->parameters_[0], "x"s));
    ASSERT_TRUE(TestLiteralExpression(fnlit->parameters_[1], "y"s));

    ASSERT_EQ(1, fnlit->body_->statements_.size());

    std::shared_ptr<ast::ExpressionStatement> body_stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            fnlit->body_->statements_[0]);

    if (!body_stmt)
        FAIL() << "Not an ExpressionStatement - got "
               << typeid(&fnlit->body_->statements_[0]).name();
    std::cout << "\nTESTING FOR INFIX..\n";
    ASSERT_TRUE(TestInfixExpression(body_stmt->expression_, "x"s, "+", "y"s));
}

TEST_F(ParserTest, TestFunctionParameterParsing)
{
    struct TestCase
    {
        std::string input;
        std::vector<std::string> expected_params;
    };
    std::vector<TestCase> tests{
        {"fn() {};", std::vector<std::string>()},
        {"fn(x) {};", std::vector<std::string>{"x"}},
        {"fn(x, y, z) {};", std::vector<std::string>{"x", "y", "z"}}};
    for (auto tt : tests)
    {
        std::unique_ptr<lexer::Lexer> lex =
            std::make_unique<lexer::Lexer>(tt.input);
        std::unique_ptr<parser::Parser> parsley =
            std::make_unique<parser::Parser>(std::move(lex));
        std::shared_ptr<ast::Program> program = parsley->ParseProgram();
        EXPECT_FALSE(parsley->CheckErrors());
        ASSERT_EQ(1, program->statements_.size());

        std::shared_ptr<ast::ExpressionStatement> stmt =
            std::dynamic_pointer_cast<ast::ExpressionStatement>(
                program->statements_[0]);

        if (!stmt)
            FAIL() << "Not an ExpressionStatement - got "
                   << typeid(program->statements_[0]).name();
        std::shared_ptr<ast::FunctionLiteral> fnlit =
            std::dynamic_pointer_cast<ast::FunctionLiteral>(stmt->expression_);

        if (!fnlit)
            FAIL() << "Not an FunctionLiteral - got "
                   << typeid(&stmt->expression_).name();

        ASSERT_EQ(fnlit->parameters_.size(), tt.expected_params.size());

        int params_len = tt.expected_params.size();
        for (int i = 0; i < params_len; i++)
        {
            std::cout << "      testing param " << i << " :"
                      << tt.expected_params[i] << std::endl;
            TestLiteralExpression(fnlit->parameters_[i], tt.expected_params[i]);
        }
    }
}

TEST_F(ParserTest, TestParsingArray)
{
    std::string input = "[1, 2 * 2, 3 + 3]";
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    EXPECT_EQ(1, program->statements_.size());
    if (program->statements_.size() != 1)
    {
        for (auto s : program->statements_)
            std::cout << s->String() << std::endl;
    }
    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::ArrayLiteral> array_lit =
        std::dynamic_pointer_cast<ast::ArrayLiteral>(stmt->expression_);
    if (!array_lit)
        FAIL() << "Not an ArrayLiteral - got "
               << typeid(&stmt->expression_).name();

    if (array_lit->elements_.size() != 3)
        FAIL() << "len(array.elements_) not 3. got="
               << array_lit->elements_.size();

    TestIntegerLiteral(array_lit->elements_[0], 1);
    TestInfixExpression(array_lit->elements_[1], (int64_t)2, "*", (int64_t)2);
    TestInfixExpression(array_lit->elements_[2], (int64_t)3, "+", (int64_t)3);
}

TEST_F(ParserTest, TestParsingHashLiteral)
{
    std::string input = R"({"one": 1, "two": 2, "three": 3})";
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    EXPECT_EQ(1, program->statements_.size());
    if (program->statements_.size() != 1)
    {
        for (auto s : program->statements_)
            std::cout << s->String() << std::endl;
    }
    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::HashLiteral> hash_lit =
        std::dynamic_pointer_cast<ast::HashLiteral>(stmt->expression_);
    if (!hash_lit)
        FAIL() << "Not a HashLiteral - got "
               << typeid(&stmt->expression_).name();

    if (hash_lit->pairs_.size() != 3)
        FAIL() << "len(hash.pairs_) not 3. got=" << hash_lit->pairs_.size();

    std::unordered_map<std::string, int> expected = {
        {"one", 1}, {"two", 2}, {"three", 3}};

    for (auto &it : hash_lit->pairs_)
    {
        std::shared_ptr<ast::StringLiteral> string_lit =
            std::dynamic_pointer_cast<ast::StringLiteral>(it.first);
        if (!string_lit)
            FAIL() << "Not a StringLiteral - got " << typeid(it.first).name();

        int expected_value = expected.at(string_lit->String());

        TestIntegerLiteral(it.second, expected_value);
    }
}

TEST_F(ParserTest, TestParsingEmptyHash)
{
    std::string input = R"({})";
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    EXPECT_EQ(1, program->statements_.size());
    if (program->statements_.size() != 1)
    {
        for (auto s : program->statements_)
            std::cout << s->String() << std::endl;
    }
    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::HashLiteral> hash_lit =
        std::dynamic_pointer_cast<ast::HashLiteral>(stmt->expression_);
    if (!hash_lit)
        FAIL() << "Not a HashLiteral - got "
               << typeid(&stmt->expression_).name();

    if (hash_lit->pairs_.size() != 0)
        FAIL() << "hash.pairs has wrong length. got="
               << hash_lit->pairs_.size();
}

TEST_F(ParserTest, TestParsingHashLiteralsWithExpressions)
{
    std::string input = R"({"one": 0 + 1, "two": 10 - 8, "three": 15 / 5})";
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    EXPECT_EQ(1, program->statements_.size());
    if (program->statements_.size() != 1)
    {
        for (auto s : program->statements_)
            std::cout << s->String() << std::endl;
    }
    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::HashLiteral> hash_lit =
        std::dynamic_pointer_cast<ast::HashLiteral>(stmt->expression_);
    if (!hash_lit)
        FAIL() << "Not a HashLiteral - got "
               << typeid(&stmt->expression_).name();

    if (hash_lit->pairs_.size() != 3)
        FAIL() << "hash.pairs has wrong length. got="
               << hash_lit->pairs_.size();

    std::unordered_map<std::string,
                       std::function<void(std::shared_ptr<ast::Expression>)>>
        tests{
            {"one",
             [](std::shared_ptr<ast::Expression> e) {
                 TestInfixExpression(e, (int64_t)0, "+", (int64_t)1);
             }},

            {"two",
             [](std::shared_ptr<ast::Expression> e) {
                 TestInfixExpression(e, (int64_t)10, "-", (int64_t)8);
             }},

            {"three",
             [](std::shared_ptr<ast::Expression> e) {
                 TestInfixExpression(e, (int64_t)15, "/", (int64_t)5);
             }},
        };

    for (auto &it : hash_lit->pairs_)
    {
        auto lit = std::dynamic_pointer_cast<ast::StringLiteral>(it.first);
        if (!lit)
            FAIL() << "Key not  a StringLiteral - got "
                   << typeid(it.first).name();

        auto test_func = tests[lit->String()];

        test_func(it.second);
    }
}

TEST_F(ParserTest, TestParsingIndexExpressions)
{
    std::string input = "myArray[1 + 1]";
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    EXPECT_EQ(1, program->statements_.size());
    if (program->statements_.size() != 1)
    {
        for (auto s : program->statements_)
            std::cout << s->String() << std::endl;
    }
    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::IndexExpression> index_x =
        std::dynamic_pointer_cast<ast::IndexExpression>(stmt->expression_);
    if (!index_x)
        FAIL() << "Not an IndexExpression - got "
               << typeid(&stmt->expression_).name();

    EXPECT_TRUE(TestIdentifier(index_x->left_, "myArray"));
    EXPECT_TRUE(
        TestInfixExpression(index_x->index_, (int64_t)1, "+", (int64_t)1));
}

TEST_F(ParserTest, TestCallExpression)
{
    std::string input = "add(1, 2 * 3, 4 + 5);";
    std::cout << "CALL EXPRESSION! -- " << input << std::endl;
    std::unique_ptr<lexer::Lexer> lex = std::make_unique<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(std::move(lex));
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    EXPECT_EQ(1, program->statements_.size());
    if (program->statements_.size() != 1)
    {
        for (auto s : program->statements_)
            std::cout << s->String() << std::endl;
    }

    std::shared_ptr<ast::ExpressionStatement> stmt =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(
            program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an ExpressionStatement";

    std::shared_ptr<ast::CallExpression> expr =
        std::dynamic_pointer_cast<ast::CallExpression>(stmt->expression_);
    if (!expr)
        FAIL() << "Not an FunctionLiteral - got "
               << typeid(&stmt->expression_).name();

    EXPECT_TRUE(TestIdentifier(expr->function_, "add"));

    EXPECT_EQ(3, expr->arguments_.size());
    EXPECT_TRUE(TestLiteralExpression(expr->arguments_[0], (int64_t)1));
    EXPECT_TRUE(
        TestInfixExpression(expr->arguments_[1], (int64_t)2, "*", (int64_t)3));
    EXPECT_TRUE(
        TestInfixExpression(expr->arguments_[2], (int64_t)4, "+", (int64_t)5));
}

TEST_F(ParserTest, TestCallExpressionParsing)
{
    struct TestCase
    {
        std::string input;
        std::string expected_ident;
        std::vector<std::string> expected_args;
    };
    std::vector<TestCase> tests{
        {"add();", "add", std::vector<std::string>()},
        {"add(1);", "add", std::vector<std::string>{"1"}},
        {"add(1, 2 * 3, 4 + 5);", "add",
         std::vector<std::string>{"1", "(2*3)", "(4+5)"}}};
    for (auto tt : tests)
    {
        std::cout << "\nTesting! input: " << tt.input << std::endl;
        std::unique_ptr<lexer::Lexer> lex =
            std::make_unique<lexer::Lexer>(tt.input);
        std::unique_ptr<parser::Parser> parsley =
            std::make_unique<parser::Parser>(std::move(lex));
        std::shared_ptr<ast::Program> program = parsley->ParseProgram();
        EXPECT_FALSE(parsley->CheckErrors());
        ASSERT_EQ(1, program->statements_.size());

        std::shared_ptr<ast::ExpressionStatement> stmt =
            std::dynamic_pointer_cast<ast::ExpressionStatement>(
                program->statements_[0]);
        if (!stmt)
            FAIL() << "Not an ExpressionStatement - got "
                   << typeid(program->statements_[0]).name();

        std::shared_ptr<ast::CallExpression> expr =
            std::dynamic_pointer_cast<ast::CallExpression>(stmt->expression_);
        if (!expr)
            FAIL() << "Not an CallExpression - got "
                   << typeid(&stmt->expression_).name();

        EXPECT_TRUE(TestIdentifier(expr->function_, tt.expected_ident));
        ASSERT_EQ(expr->arguments_.size(), tt.expected_args.size());

        int args_len = tt.expected_args.size();
        for (int i = 0; i < args_len; i++)
        {
            std::cout << "\nComparing " << expr->arguments_[i]->String()
                      << " with expected: " << tt.expected_args[i] << std::endl;
            EXPECT_TRUE(expr->arguments_[i]->String().compare(
                            tt.expected_args[i]) == 0);
        }
    }
}

TEST_F(ParserTest, TestParsingLsStatement)
{
    std::cout << "Testing `ls` statement" << std::endl;
    std::string input = R"(ls;)";
    std::shared_ptr<lexer::Lexer> lex = std::make_shared<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(lex);
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());
    std::shared_ptr<ast::LsStatement> stmt =
        std::dynamic_pointer_cast<ast::LsStatement>(program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an LsStatement";
}

TEST_F(ParserTest, TestParsingLsPathStatement)
{
    std::cout << "Testing `ls kicks` statement" << std::endl;
    std::string input = R"(ls kicks;)";
    std::shared_ptr<lexer::Lexer> lex = std::make_shared<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(lex);
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());
    std::shared_ptr<ast::LsStatement> stmt =
        std::dynamic_pointer_cast<ast::LsStatement>(program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an LsStatement";

    std::shared_ptr<ast::StringLiteral> literal =
        std::dynamic_pointer_cast<ast::StringLiteral>(stmt->path_);
    if (!literal)
        FAIL() << "ls statment path is not a StringLiteral. Got "
               << typeid(&stmt->path_).name();
}

TEST_F(ParserTest, TestParsingSampleStatement)
{
    std::cout << "Testing sample statement" << std::endl;
    std::string input = R"(ls kicks;)";
    std::shared_ptr<lexer::Lexer> lex = std::make_shared<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(lex);
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());
    std::shared_ptr<ast::LsStatement> stmt =
        std::dynamic_pointer_cast<ast::LsStatement>(program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an LsStatement";

    std::shared_ptr<ast::StringLiteral> literal =
        std::dynamic_pointer_cast<ast::StringLiteral>(stmt->path_);
    if (!literal)
        FAIL() << "ls statment path is not a StringLiteral. Got "
               << typeid(&stmt->path_).name();
}

TEST_F(ParserTest, TestParsingPsStatement)
{
    std::cout << "Testing `ps` statement" << std::endl;
    std::string input = R"(ps;)";
    std::shared_ptr<lexer::Lexer> lex = std::make_shared<lexer::Lexer>(input);
    std::unique_ptr<parser::Parser> parsley =
        std::make_unique<parser::Parser>(lex);
    std::shared_ptr<ast::Program> program = parsley->ParseProgram();
    EXPECT_FALSE(parsley->CheckErrors());
    ASSERT_EQ(1, program->statements_.size());
    std::shared_ptr<ast::PsStatement> stmt =
        std::dynamic_pointer_cast<ast::PsStatement>(program->statements_[0]);
    if (!stmt)
        FAIL() << "program->statements_[0] is not an PsStatement";
}

} // namespace
