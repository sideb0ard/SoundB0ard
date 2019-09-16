#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "token.hpp"

using ::token::Token;

namespace ast
{

class BlockStatement;

/////////////////// NODE /////////////////

class Node
{
  public:
    Node() = default;
    explicit Node(Token toke) : token_{toke} {}
    virtual ~Node() = default;
    virtual std::string TokenLiteral() const { return token_.literal_; }
    virtual std::string String() const = 0;

  public:
    Token token_;
};

/////////////////// EXPRESSIONS

class Expression : public Node
{
  public:
    Expression() {}
    explicit Expression(Token token) : Node{token} {}
};

class Identifier : public Expression
{
  public:
    Identifier() {}
    Identifier(Token token, std::string val) : Expression{token}, value_{val} {}
    std::string String() const override;
    std::string value_;
};

class IntegerLiteral : public Expression
{
  public:
    IntegerLiteral() {}
    explicit IntegerLiteral(Token token) : Expression{token} {}
    IntegerLiteral(Token token, int64_t val) : Expression{token}, value_{val} {}
    std::string String() const override;

  public:
    int64_t value_;
};

class StringLiteral : public Expression
{
  public:
    StringLiteral(Token token, std::string val) : Expression{token}, value_{val}
    {
    }
    std::string String() const override { return value_; }

  public:
    std::string value_;
};

class BooleanExpression : public Expression
{
  public:
    BooleanExpression() {}
    explicit BooleanExpression(Token token) : Expression{token} {}
    BooleanExpression(Token token, bool val) : Expression{token}, value_{val} {}
    std::string String() const override;

  public:
    bool value_;
};

class PrefixExpression : public Expression
{
  public:
    PrefixExpression() {}
    explicit PrefixExpression(Token token) : Expression{token} {}
    PrefixExpression(Token token, std::string op)
        : Expression{token}, operator_{op}
    {
    }
    std::string String() const override;

  public:
    std::string operator_;
    std::shared_ptr<Expression> right_;
};

class InfixExpression : public Expression
{
  public:
    InfixExpression() {}
    explicit InfixExpression(Token token) : Expression{token} {}
    InfixExpression(Token token, std::string op,
                    std::shared_ptr<Expression> left)
        : Expression{token}, operator_{op}, left_{left}
    {
    }
    std::string String() const override;

  public:
    std::string operator_;
    std::shared_ptr<Expression> left_;
    std::shared_ptr<Expression> right_;
};

class IfExpression : public Expression
{
  public:
    IfExpression() {}
    explicit IfExpression(Token token) : Expression{token} {}

    std::string String() const override;

  public:
    std::shared_ptr<Expression> condition_;
    std::shared_ptr<BlockStatement> consequence_;
    std::shared_ptr<BlockStatement> alternative_;
};

class FunctionLiteral : public Expression
{
  public:
    FunctionLiteral() {}
    explicit FunctionLiteral(Token token) : Expression{token} {}

    std::string String() const override;

  public:
    std::vector<std::shared_ptr<Identifier>> parameters_;
    std::shared_ptr<BlockStatement> body_{nullptr};
};

class CallExpression : public Expression
{
  public:
    CallExpression() {}
    CallExpression(Token token, std::shared_ptr<Expression> func)
        : Expression{token}, function_{func}
    {
    }

    std::string String() const override;

  public:
    std::shared_ptr<Expression> function_{nullptr};
    std::vector<std::shared_ptr<Expression>> arguments_;
};

class ArrayLiteral : public Expression
{
  public:
    ArrayLiteral() {}
    explicit ArrayLiteral(Token token) : Expression{token} {}
    ArrayLiteral(Token token, std::vector<std::shared_ptr<Expression>> elements)
        : Expression{token}, elements_{elements}
    {
    }

    std::string String() const override;

  public:
    std::vector<std::shared_ptr<Expression>> elements_;
};

class HashLiteral : public Expression
{
  public:
    HashLiteral() {}
    explicit HashLiteral(Token token) : Expression{token} {}

    std::string String() const override;

  public:
    std::unordered_map<std::shared_ptr<Expression>, std::shared_ptr<Expression>>
        pairs_;
};

class IndexExpression : public Expression
{
  public:
    IndexExpression() {}
    IndexExpression(Token token, std::shared_ptr<Expression> left)
        : Expression{token}, left_{left}
    {
    }
    IndexExpression(Token token, std::shared_ptr<Expression> left,
                    std::shared_ptr<Expression> index)
        : Expression{token}, left_{left}, index_{index}
    {
    }

    std::string String() const override;

  public:
    std::shared_ptr<Expression> left_{nullptr};
    std::shared_ptr<Expression> index_{nullptr};
};

///////////////////////////////////////////////////////////////
//////////////////////// STATEMENTS............. //////////////

class Statement : public Node
{
  public:
    explicit Statement(Token toke) : Node{toke} {};
};

class LetStatement : public Statement
{
  public:
    explicit LetStatement(Token toke) : Statement(toke) {}
    std::string String() const override;

  public:
    std::shared_ptr<Identifier> name_{nullptr};
    std::shared_ptr<Expression> value_{nullptr};
};

class ReturnStatement : public Statement
{
  public:
    explicit ReturnStatement(Token toke) : Statement(toke) {}
    std::string String() const override;

  public:
    std::shared_ptr<Expression> return_value_{nullptr};
};

class ExpressionStatement : public Statement
{
  public:
    explicit ExpressionStatement(Token toke) : Statement(toke) {}
    std::string String() const override;

  public:
    std::shared_ptr<Expression> expression_{nullptr};
};

class BlockStatement : public Statement
{
  public:
    explicit BlockStatement(Token toke) : Statement(toke) {}
    std::string String() const override;

  public:
    std::vector<std::shared_ptr<Statement>> statements_;
};

class ForStatement : public Statement
{
  public:
    explicit ForStatement(Token toke) : Statement(toke) {}
    std::string String() const override;

  public:
    std::shared_ptr<Identifier> iterator_{nullptr};
    std::shared_ptr<Expression> iterator_value_{nullptr};

    std::shared_ptr<Expression> termination_condition_{nullptr};

    std::shared_ptr<Expression> increment_{nullptr};

    std::shared_ptr<BlockStatement> body_;
};

// ROOT //////////////////////

class Program : public Node
{
  public:
    Program() = default;
    std::string TokenLiteral() const override;
    std::string String() const override;

  public:
    std::vector<std::shared_ptr<Statement>> statements_;
};

} // namespace ast
