#pragma once

#include <memory>
#include <pattern_functions.hpp>
#include <string>
#include <vector>

#include "ast.hpp"
#include "object.hpp"

namespace evaluator {

inline auto const TTRUE = std::make_shared<object::Boolean>(true);
inline auto const FFALSE = std::make_shared<object::Boolean>(false);
inline auto const NULLL = std::make_shared<object::Null>();

std::shared_ptr<object::Object> Eval(std::shared_ptr<ast::Node> node,
                                     std::shared_ptr<object::Environment> env);

std::shared_ptr<object::Object> EvalProgram(
    std::vector<std::shared_ptr<ast::Statement>> const &stmts,
    std::shared_ptr<object::Environment> env);

std::shared_ptr<object::Object> EvalBlockStatement(
    std::shared_ptr<ast::BlockStatement> block,
    std::shared_ptr<object::Environment> env);

std::shared_ptr<object::Object> EvalForLoop(
    std::shared_ptr<object::ForLoop> for_loop);

std::shared_ptr<PatternFunction> EvalPatternFunctionExpression(
    std::shared_ptr<ast::Expression> func);

std::shared_ptr<object::Object> EvalProcessStatement(
    std::shared_ptr<ast::ProcessStatement> proc,
    std::shared_ptr<object::Environment> env);

std::shared_ptr<object::Object> EvalProcessSetStatement(
    std::shared_ptr<ast::ProcessSetStatement> proc,
    std::shared_ptr<object::Environment> env);

std::shared_ptr<object::Object> EvalPrefixExpression(
    std::string op, std::shared_ptr<object::Object> obj);

std::shared_ptr<object::Object> EvalInfixExpression(
    std::string op, std::shared_ptr<object::Object> left,
    std::shared_ptr<object::Object> right);

std::shared_ptr<object::Object> EvalNumberInfixExpression(
    std::string op, std::shared_ptr<object::Number> left,
    std::shared_ptr<object::Number> right);

std::shared_ptr<object::Object> EvalStringInfixExpression(
    std::string op, std::shared_ptr<object::String> left,
    std::shared_ptr<object::String> right);

std::shared_ptr<object::Object> EvalArrayInfixExpression(
    std::string op, std::shared_ptr<object::Array> left,
    std::shared_ptr<object::Array> right);

std::shared_ptr<object::Object> EvalMultiplyArrayExpression(
    std::string op, std::shared_ptr<object::Array> left,
    std::shared_ptr<object::Number> right);

std::shared_ptr<object::Object> EvalBangOperatorExpression(
    std::shared_ptr<object::Object> right);

std::shared_ptr<object::Object> EvalMinusPrefixOperatorExpression(
    std::shared_ptr<object::Object> right);

std::shared_ptr<object::Object> EvalDecrementOperatorExpression(
    std::shared_ptr<object::Object> right);

std::shared_ptr<object::Object> EvalIncrementOperatorExpression(
    std::shared_ptr<object::Object> right);

std::shared_ptr<object::Boolean> NativeBoolToBooleanObject(bool input);

std::shared_ptr<object::Object> EvalIfExpression(
    std::shared_ptr<ast::IfExpression> if_expr,
    std::shared_ptr<object::Environment> env);

std::shared_ptr<object::Object> EvalIdentifier(
    std::shared_ptr<ast::Identifier> ident,
    std::shared_ptr<object::Environment> env);

std::shared_ptr<object::Object> EvalHashLiteral(
    std::shared_ptr<ast::HashLiteral> hash_literal,
    std::shared_ptr<object::Environment> env);

std::shared_ptr<object::Object> EvalIndexExpression(
    std::shared_ptr<object::Object> left,
    std::shared_ptr<object::Object> index);

std::shared_ptr<object::Object> EvalHashIndexExpression(
    std::shared_ptr<object::Object> hash_obj,
    std::shared_ptr<object::Object> key);

std::shared_ptr<object::Object> EvalArrayIndexExpression(
    std::shared_ptr<object::Object> left,
    std::shared_ptr<object::Object> index);

std::shared_ptr<object::Object> EvalStringIndexExpression(
    std::shared_ptr<object::Object> left,
    std::shared_ptr<object::Object> index);

std::shared_ptr<object::Object> EvalIndexExpressionUpdate(
    std::shared_ptr<object::Object> left, std::shared_ptr<object::Object> index,
    std::shared_ptr<object::Object> new_value);

std::shared_ptr<object::Object> EvalHashIndexExpressionUpdate(
    std::shared_ptr<object::Object> left, std::shared_ptr<object::Object> index,
    std::shared_ptr<object::Object> new_value);

std::shared_ptr<object::Object> EvalArrayIndexExpressionUpdate(
    std::shared_ptr<object::Object> left, std::shared_ptr<object::Object> index,
    std::shared_ptr<object::Object> new_value);

std::shared_ptr<object::Object> EvalStringIndexExpressionUpdate(
    std::shared_ptr<object::Object> left, std::shared_ptr<object::Object> index,
    std::shared_ptr<object::Object> new_value);

std::vector<std::shared_ptr<object::Object>> EvalExpressions(
    std::vector<std::shared_ptr<ast::Expression>> exps,
    std::shared_ptr<object::Environment> env);

std::shared_ptr<object::Object> ApplyFunction(
    std::shared_ptr<object::Object> callable,
    std::vector<std::shared_ptr<object::Object>> args);

std::shared_ptr<object::Object> ApplyGeneratorRun(
    std::shared_ptr<object::Object> callable);

std::shared_ptr<object::Environment> ExtendFunctionEnv(
    std::shared_ptr<object::Function> fun,
    std::vector<std::shared_ptr<object::Object>> const &args);

std::shared_ptr<object::Object> UnwrapReturnValue(
    std::shared_ptr<object::Object> obj);

template <typename... Args>
std::shared_ptr<object::Error> NewError(std::string format, Args... args);

}  // namespace evaluator
