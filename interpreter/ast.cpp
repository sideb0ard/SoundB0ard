#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "ast.hpp"

namespace ast
{

std::string Program::TokenLiteral() const
{
    if (!statements_.empty())
        return statements_[0]->TokenLiteral();
    else
        return "";
}

std::string Program::String() const
{
    std::stringstream ss;

    for (auto s : statements_)
        ss << s->String();

    return ss.str();
}

std::string LetStatement::String() const
{
    std::stringstream ss;
    if (name_)
        ss << TokenLiteral() << " " << name_->String() << " = ";
    if (value_)
        ss << value_->String();
    ss << ";";

    return ss.str();
}

std::string ForStatement::String() const
{
    std::stringstream ss;
    ss << TokenLiteral() << "(";
    if (iterator_)
        ss << iterator_->String() << " = ";
    if (iterator_value_)
        ss << iterator_value_->String();
    ss << ";";
    if (termination_condition_)
        ss << termination_condition_->String();
    ss << ";";
    if (increment_)
        ss << increment_->String();
    ss << ")";
    if (body_)
        ss << body_->String();

    return ss.str();
}

std::string ReturnStatement::String() const
{
    std::stringstream ss;
    ss << TokenLiteral();
    if (return_value_)
        ss << " " << return_value_->String();
    ss << ";";

    return ss.str();
}

std::string ExpressionStatement::String() const
{
    std::stringstream ss;
    if (expression_)
        ss << expression_->String();

    return ss.str();
}

std::string PrefixExpression::String() const
{
    std::stringstream ss;
    ss << "(";
    if (!operator_.empty())
        ss << operator_;
    if (right_)
        ss << right_->String();
    ss << ")";

    return ss.str();
}

std::string BlockStatement::String() const
{
    std::stringstream ss;
    for (auto &s : statements_)
        ss << s->String();

    return ss.str();
}

std::string IfExpression::String() const
{
    std::stringstream ss;
    ss << "if ";
    if (!condition_)
        ss << condition_->String();
    ss << " ";
    if (consequence_)
        ss << consequence_->String();
    if (alternative_)
    {
        ss << " else ";
        ss << alternative_->String();
    }

    return ss.str();
}

std::string FunctionLiteral::String() const
{
    std::stringstream ss;

    std::vector<std::string> params;
    for (auto p : parameters_)
        params.push_back(p->String());

    ss << TokenLiteral() << "("
       << std::accumulate(params.begin(), params.end(), std::string(),
                          [](const std::string &lhs, const std::string &rhs) {
                              std::string ret{lhs};
                              if (!lhs.empty() && !rhs.empty())
                                  ret += ", ";
                              ret += rhs;
                              return ret;
                          })
       << ")" << body_->String();

    return ss.str();
}

std::string CallExpression::String() const
{
    std::stringstream ss;

    std::vector<std::string> arguments;
    for (auto a : arguments_)
        arguments.push_back(a->String());

    ss << function_->String() << "("
       << std::accumulate(arguments.begin(), arguments.end(), std::string(),
                          [](const std::string &lhs, const std::string &rhs) {
                              std::string ret{lhs};
                              if (!lhs.empty() && !rhs.empty())
                                  ret += ", ";
                              ret += rhs;
                              return ret;
                          })
       << ")";

    return ss.str();
}

std::string ArrayLiteral::String() const
{
    std::stringstream ss;

    std::vector<std::string> elements;
    for (auto const &e : elements_)
        elements.push_back(e->String());

    int elements_size = elements.size();
    ss << "[";
    for (int i = 0; i < elements_size; i++)
    {
        ss << elements[i];
        if (i != elements_size - 1)
            ss << ", ";
    }
    ss << "]";

    return ss.str();
}

std::string HashLiteral::String() const
{
    std::stringstream ss;

    std::vector<std::string> pairs;
    for (auto const &it : pairs_)
        pairs.push_back(it.first->String() + ":" + it.second->String());

    int pairs_size = pairs.size();
    ss << "{";
    for (int i = 0; i < pairs_size; i++)
    {
        ss << pairs[i];
        if (i != pairs_size - 1)
            ss << ", ";
    }
    ss << "}";

    return ss.str();
}

std::string IndexExpression::String() const
{
    std::stringstream ss;
    ss << "(";
    if (left_)
        ss << left_->String();
    ss << "[";
    if (index_)
        ss << index_->String();
    ss << "])";
    return ss.str();
}

std::string InfixExpression::String() const
{
    std::stringstream ss;
    ss << "(";
    if (left_)
        ss << left_->String();
    if (!operator_.empty())
        ss << operator_;
    if (right_)
        ss << right_->String();
    ss << ")";

    return ss.str();
}

std::string Identifier::String() const { return value_; }
std::string IntegerLiteral::String() const { return token_.literal_; }
std::string BooleanExpression::String() const { return token_.literal_; }

} // namespace ast
