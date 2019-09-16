#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ast.hpp"
#include "builtins.hpp"
#include "evaluator.hpp"
#include "object.hpp"

namespace
{
bool IsTruthy(std::shared_ptr<object::Object> obj)
{
    if (obj == evaluator::NULLL)
        return false;
    else if (obj == evaluator::TRUE)
        return true;
    else if (obj == evaluator::FALSE)
        return false;

    return true;
}

bool IsError(std::shared_ptr<object::Object> obj)
{
    if (obj)
    {
        return obj->Type() == object::ERROR_OBJ;
    }
    return false;
}

bool IsHashable(std::shared_ptr<object::Object> obj)
{
    if (obj->Type() == object::BOOLEAN_OBJ ||
        obj->Type() == object::INTEGER_OBJ || obj->Type() == object::STRING_OBJ)
        return true;
    return false;
}

object::HashKey MakeHashKey(std::shared_ptr<object::Object> hashkey)
{
    std::shared_ptr<object::Boolean> hash_key_bool =
        std::dynamic_pointer_cast<object::Boolean>(hashkey);
    if (hash_key_bool)
        return hash_key_bool->HashKey();

    std::shared_ptr<object::Integer> hash_key_int =
        std::dynamic_pointer_cast<object::Integer>(hashkey);
    if (hash_key_int)
        return hash_key_int->HashKey();

    std::shared_ptr<object::String> hash_key_string =
        std::dynamic_pointer_cast<object::String>(hashkey);
    if (hash_key_string)
        return hash_key_string->HashKey();

    return object::HashKey{};
}

} // namespace

namespace evaluator
{

std::shared_ptr<object::Object> Eval(std::shared_ptr<ast::Node> node,
                                     std::shared_ptr<object::Environment> env)
{
    std::shared_ptr<ast::Program> prog_node =
        std::dynamic_pointer_cast<ast::Program>(node);
    if (prog_node)
    {
        return EvalProgram(prog_node->statements_, env);
    }

    std::shared_ptr<ast::BlockStatement> block_statement_node =
        std::dynamic_pointer_cast<ast::BlockStatement>(node);
    if (block_statement_node)
    {
        return EvalBlockStatement(block_statement_node, env);
    }

    std::shared_ptr<ast::ForStatement> for_statement_node =
        std::dynamic_pointer_cast<ast::ForStatement>(node);
    if (for_statement_node)
    {
        return EvalForStatement(for_statement_node, env);
    }

    std::shared_ptr<ast::ExpressionStatement> expr_statement_node =
        std::dynamic_pointer_cast<ast::ExpressionStatement>(node);
    if (expr_statement_node)
    {
        return Eval(expr_statement_node->expression_, env);
    }

    std::shared_ptr<ast::ReturnStatement> return_statement_node =
        std::dynamic_pointer_cast<ast::ReturnStatement>(node);
    if (return_statement_node)
    {
        auto val = Eval(return_statement_node->return_value_, env);
        if (IsError(val))
            return val;
        return std::make_shared<object::ReturnValue>(val);
    }

    // Expressions
    std::shared_ptr<ast::IntegerLiteral> il =
        std::dynamic_pointer_cast<ast::IntegerLiteral>(node);
    if (il)
    {
        return std::make_shared<object::Integer>(il->value_);
    }

    std::shared_ptr<ast::BooleanExpression> be =
        std::dynamic_pointer_cast<ast::BooleanExpression>(node);
    if (be)
    {
        return NativeBoolToBooleanObject(be->value_);
    }

    std::shared_ptr<ast::PrefixExpression> pe =
        std::dynamic_pointer_cast<ast::PrefixExpression>(node);
    if (pe)
    {
        auto right = Eval(pe->right_, env);
        if (IsError(right))
            return right;
        return EvalPrefixExpression(pe->operator_, right);
    }

    std::shared_ptr<ast::InfixExpression> ie =
        std::dynamic_pointer_cast<ast::InfixExpression>(node);
    if (ie)
    {
        auto left = Eval(ie->left_, env);
        if (IsError(left))
            return left;

        auto right = Eval(ie->right_, env);
        if (IsError(right))
            return right;

        return EvalInfixExpression(ie->operator_, left, right);
    }

    std::shared_ptr<ast::IfExpression> if_expr =
        std::dynamic_pointer_cast<ast::IfExpression>(node);
    if (if_expr)
    {
        return EvalIfExpression(if_expr, env);
    }

    std::shared_ptr<ast::LetStatement> let_expr =
        std::dynamic_pointer_cast<ast::LetStatement>(node);
    if (let_expr)
    {
        auto val = Eval(let_expr->value_, env);
        if (IsError(val))
        {
            return val;
        }
        env->Set(let_expr->name_->value_, val);
    }

    std::shared_ptr<ast::Identifier> ident =
        std::dynamic_pointer_cast<ast::Identifier>(node);
    if (ident)
    {
        return EvalIdentifier(ident, env);
    }

    std::shared_ptr<ast::FunctionLiteral> fn =
        std::dynamic_pointer_cast<ast::FunctionLiteral>(node);
    if (fn)
    {
        auto params = fn->parameters_;
        auto body = fn->body_;
        return std::make_shared<object::Function>(params, env, body);
    }

    std::shared_ptr<ast::CallExpression> call_expr =
        std::dynamic_pointer_cast<ast::CallExpression>(node);
    if (call_expr)
    {
        auto fun = Eval(call_expr->function_, env);
        if (IsError(fun))
            return fun;

        std::vector<std::shared_ptr<object::Object>> args =
            EvalExpressions(call_expr->arguments_, env);
        if (args.size() == 1 && IsError(args[0]))
            return args[0];

        auto func_obj = std::dynamic_pointer_cast<object::Function>(fun);
        if (func_obj)
            return ApplyFunction(func_obj, args);

        auto builtin_func = std::dynamic_pointer_cast<object::BuiltIn>(fun);
        if (builtin_func)
            return ApplyFunction(builtin_func, args);

        return NewError("Not a function object, mate:%s!", fun->Type());
    }

    std::shared_ptr<ast::StringLiteral> sliteral =
        std::dynamic_pointer_cast<ast::StringLiteral>(node);
    if (sliteral)
    {
        return std::make_shared<object::String>(sliteral->value_);
    }

    std::shared_ptr<ast::ArrayLiteral> aliteral =
        std::dynamic_pointer_cast<ast::ArrayLiteral>(node);
    if (aliteral)
    {
        std::vector<std::shared_ptr<object::Object>> elements =
            EvalExpressions(aliteral->elements_, env);
        if (elements.size() == 1 && IsError(elements[0]))
            return elements[0];
        return std::make_shared<object::Array>(elements);
    }

    std::shared_ptr<ast::IndexExpression> index_x =
        std::dynamic_pointer_cast<ast::IndexExpression>(node);
    if (index_x)
    {
        std::shared_ptr<object::Object> left = Eval(index_x->left_, env);
        if (IsError(left))
            return left;

        std::shared_ptr<object::Object> index = Eval(index_x->index_, env);
        if (IsError(index))
            return index;

        return EvalIndexExpression(left, index);
    }

    std::shared_ptr<ast::HashLiteral> hash_lit =
        std::dynamic_pointer_cast<ast::HashLiteral>(node);
    if (hash_lit)
    {
        return EvalHashLiteral(hash_lit, env);
    }

    return NULLL;
}

std::shared_ptr<object::Object>
EvalIndexExpression(std::shared_ptr<object::Object> left,
                    std::shared_ptr<object::Object> index)
{
    if (left->Type() == object::ARRAY_OBJ &&
        index->Type() == object::INTEGER_OBJ)
        return EvalArrayIndexExpression(left, index);
    else if (left->Type() == object::HASH_OBJ)
        return EvalHashIndexExpression(left, index);

    return NewError("index operation not supported: %s", left->Type());
}

std::shared_ptr<object::Object>
EvalArrayIndexExpression(std::shared_ptr<object::Object> array_obj,
                         std::shared_ptr<object::Object> index)
{
    std::shared_ptr<object::Array> my_array =
        std::dynamic_pointer_cast<object::Array>(array_obj);

    std::shared_ptr<object::Integer> int_obj =
        std::dynamic_pointer_cast<object::Integer>(index);
    if (my_array && int_obj)
    {
        int idx = int_obj->value_;
        int num_elems = my_array->elements_.size();
        if (idx >= 0 && idx < num_elems)
            return my_array->elements_[idx];
        else
            return NULLL;
    }
    return NewError("Couldn't unpack yer Array OBJ!");
}

std::shared_ptr<object::Object>
EvalHashIndexExpression(std::shared_ptr<object::Object> hash_obj,
                        std::shared_ptr<object::Object> key)
{
    std::shared_ptr<object::Hash> my_hash =
        std::dynamic_pointer_cast<object::Hash>(hash_obj);

    if (!IsHashable(key))
        return NewError("Unusable as hash key: %s", key->Type());

    object::HashKey hashed = MakeHashKey(key);
    auto hpair_it = my_hash->pairs_.find(hashed);
    if (hpair_it != my_hash->pairs_.end())
        return hpair_it->second.value_;

    return evaluator::NULLL;
}

std::shared_ptr<object::Object>
EvalPrefixExpression(std::string op, std::shared_ptr<object::Object> right)
{
    if (op.compare("!") == 0)
        return EvalBangOperatorExpression(right);
    else if (op.compare("-") == 0)
        return EvalMinusPrefixOperatorExpression(right);
    else if (op.compare("++") == 0)
        return EvalIncrementOperatorExpression(right);
    else if (op.compare("--") == 0)
        return EvalDecrementOperatorExpression(right);
    else
        return NewError("unknown operator: %s %s ", op, right->Type());
}

std::shared_ptr<object::Object>
EvalForStatement(std::shared_ptr<ast::ForStatement> for_loop,
                 std::shared_ptr<object::Environment> env)
{
    std::cout << "I'm A FOR LOOPO!\n";

    std::shared_ptr<object::Environment> new_env =
        std::make_shared<object::Environment>(env);

    auto val = Eval(for_loop->iterator_value_, env);
    if (IsError(val))
    {
        return val;
    }
    new_env->Set(for_loop->iterator_->value_, val);

    std::cout << "SET " << for_loop->iterator_->String() << " with "
              << val->Inspect() << std::endl;

    std::shared_ptr<object::Object> result;
    while (IsTruthy(Eval(for_loop->termination_condition_, new_env)))
    {
        result = Eval(for_loop->body_, new_env);
        Eval(for_loop->increment_, new_env);
    }

    return result;
}

std::shared_ptr<object::Object>
EvalIfExpression(std::shared_ptr<ast::IfExpression> if_expr,
                 std::shared_ptr<object::Environment> env)
{
    auto condition = Eval(if_expr->condition_, env);
    if (IsError(condition))
        return condition;

    if (IsTruthy(condition))
    {
        return Eval(if_expr->consequence_, env);
    }
    else if (if_expr->alternative_)
    {
        return Eval(if_expr->alternative_, env);
    }

    return evaluator::NULLL;
}
std::shared_ptr<object::Object>
EvalInfixExpression(std::string op, std::shared_ptr<object::Object> left,
                    std::shared_ptr<object::Object> right)
{
    if (left->Type() == object::INTEGER_OBJ &&
        right->Type() == object::INTEGER_OBJ)
    {
        auto leftie = std::dynamic_pointer_cast<object::Integer>(left);
        auto rightie = std::dynamic_pointer_cast<object::Integer>(right);
        return EvalIntegerInfixExpression(op, leftie, rightie);
    }
    else if (left->Type() == object::STRING_OBJ &&
             right->Type() == object::STRING_OBJ)
    {
        auto leftie = std::dynamic_pointer_cast<object::String>(left);
        auto rightie = std::dynamic_pointer_cast<object::String>(right);
        return EvalStringInfixExpression(op, leftie, rightie);
    }
    else if (op.compare("==") == 0)
        return NativeBoolToBooleanObject(left == right);
    else if (op.compare("!=") == 0)
        return NativeBoolToBooleanObject(left != right);
    else if (left->Type() != right->Type())
    {
        std::cerr << "LEFT AND  RIGHT AIN'T CORRECT!\n";
        return NewError("type mismatch: %s %s %s", left->Type(), op,
                        right->Type());
    }

    return NewError("unknown operator: %s %s %s", left->Type(), op,
                    right->Type());
}

std::shared_ptr<object::Object>
EvalIntegerInfixExpression(std::string op,
                           std::shared_ptr<object::Integer> left,
                           std::shared_ptr<object::Integer> right)
{

    if (op.compare("+") == 0)
        return std::make_shared<object::Integer>(left->value_ + right->value_);
    else if (op.compare("-") == 0)
        return std::make_shared<object::Integer>(left->value_ - right->value_);
    else if (op.compare("*") == 0)
        return std::make_shared<object::Integer>(left->value_ * right->value_);
    else if (op.compare("/") == 0)
        return std::make_shared<object::Integer>(left->value_ / right->value_);
    else if (op.compare("<") == 0)
        return NativeBoolToBooleanObject(left->value_ < right->value_);
    else if (op.compare(">") == 0)
        return NativeBoolToBooleanObject(left->value_ > right->value_);
    else if (op.compare("==") == 0)
        return NativeBoolToBooleanObject(left->value_ == right->value_);
    else if (op.compare("!=") == 0)
        return NativeBoolToBooleanObject(left->value_ != right->value_);

    return NewError("unknown operator: %s %s %s", left->Type(), op,
                    right->Type());
}

std::shared_ptr<object::Object>
EvalStringInfixExpression(std::string op, std::shared_ptr<object::String> left,
                          std::shared_ptr<object::String> right)
{
    if (op.compare("+") != 0)
        return NewError("unknown operator: %s %s %s", left->Type(), op,
                        right->Type());

    return std::make_shared<object::String>(left->value_ + right->value_);
}

std::shared_ptr<object::Object>
EvalBangOperatorExpression(std::shared_ptr<object::Object> right)
{
    if (right == TRUE)
        return FALSE;
    else if (right == FALSE)
        return TRUE;
    else if (right == NULLL)
        return TRUE;
    else
        return FALSE;
}

std::shared_ptr<object::Object>
EvalMinusPrefixOperatorExpression(std::shared_ptr<object::Object> right)
{
    std::shared_ptr<object::Integer> i =
        std::dynamic_pointer_cast<object::Integer>(right);
    if (!i)
    {
        return NewError("unknown operator: -%s", right->Type());
    }

    return std::make_shared<object::Integer>(-i->value_);
}

std::shared_ptr<object::Object>
EvalIncrementOperatorExpression(std::shared_ptr<object::Object> right)
{
    std::shared_ptr<object::Integer> i =
        std::dynamic_pointer_cast<object::Integer>(right);
    if (!i)
    {
        return NewError("unknown operator: ++%s", right->Type());
    }

    return std::make_shared<object::Integer>(++(i->value_));
}

std::shared_ptr<object::Object>
EvalDecrementOperatorExpression(std::shared_ptr<object::Object> right)
{
    std::shared_ptr<object::Integer> i =
        std::dynamic_pointer_cast<object::Integer>(right);
    if (!i)
    {
        return NewError("unknown operator: --%s", right->Type());
    }

    return std::make_shared<object::Integer>(--(i->value_));
}

std::shared_ptr<object::Object>
EvalProgram(std::vector<std::shared_ptr<ast::Statement>> const &stmts,
            std::shared_ptr<object::Environment> env)
{
    std::shared_ptr<object::Object> result;
    for (auto &s : stmts)
    {
        result = Eval(s, env);

        std::shared_ptr<object::ReturnValue> r =
            std::dynamic_pointer_cast<object::ReturnValue>(result);
        if (r)
            return r->value_;

        std::shared_ptr<object::Error> e =
            std::dynamic_pointer_cast<object::Error>(result);
        if (e)
            return e;
    }

    return result;
}

std::shared_ptr<object::Object>
EvalBlockStatement(std::shared_ptr<ast::BlockStatement> block,
                   std::shared_ptr<object::Environment> env)
{
    std::shared_ptr<object::Object> result;
    for (auto &s : block->statements_)
    {
        result = Eval(s, env);
        if (result != evaluator::NULLL)
        {
            if (result->Type() == object::RETURN_VALUE_OBJ ||
                result->Type() == object::ERROR_OBJ)
                return result;
        }
    }
    return result;
}

std::shared_ptr<object::Boolean> NativeBoolToBooleanObject(bool input)
{
    if (input)
        return TRUE;
    return FALSE;
}

std::shared_ptr<object::Object>
EvalIdentifier(std::shared_ptr<ast::Identifier> ident,
               std::shared_ptr<object::Environment> env)
{
    auto val = env->Get(ident->value_);
    if (val)
        return val;

    auto builtin = builtin::built_ins[ident->value_];
    if (builtin)
        return builtin;

    return NewError("identifier not found: %s", ident->value_);
}

std::shared_ptr<object::Object>
EvalHashLiteral(std::shared_ptr<ast::HashLiteral> hash_literal,
                std::shared_ptr<object::Environment> env)
{
    std::map<object::HashKey, object::HashPair> pairs;
    for (auto const &it : hash_literal->pairs_)
    {
        std::shared_ptr<object::Object> hashkey = Eval(it.first, env);
        if (IsError(hashkey))
            return hashkey;

        if (!IsHashable(hashkey))
            return NewError("unusable as hash key: %s", hashkey->Type());
        object::HashKey hashed = MakeHashKey(hashkey);

        std::shared_ptr<object::Object> val = Eval(it.second, env);
        if (IsError(val))
            return val;

        pairs.insert(std::pair<object::HashKey, object::HashPair>(
            hashed, object::HashPair{hashkey, val}));
    }

    return std::make_shared<object::Hash>(pairs);
}

std::vector<std::shared_ptr<object::Object>>
EvalExpressions(std::vector<std::shared_ptr<ast::Expression>> exps,
                std::shared_ptr<object::Environment> env)
{
    std::vector<std::shared_ptr<object::Object>> result;

    for (auto const &e : exps)
    {
        auto evaluated = Eval(e, env);
        if (IsError(evaluated))
            return std::vector<std::shared_ptr<object::Object>>{evaluated};

        result.push_back(evaluated);
    }

    return result;
}

std::shared_ptr<object::Object>
ApplyFunction(std::shared_ptr<object::Object> callable,
              std::vector<std::shared_ptr<object::Object>> args)
{
    std::shared_ptr<object::Function> func =
        std::dynamic_pointer_cast<object::Function>(callable);
    if (func)
    {
        auto extended_env = ExtendFunctionEnv(func, args);
        auto evaluated = Eval(func->body_, extended_env);
        return UnwrapReturnValue(evaluated);
    }

    std::shared_ptr<object::BuiltIn> builtin =
        std::dynamic_pointer_cast<object::BuiltIn>(callable);
    if (builtin)
    {
        return builtin->func_(args);
    }

    return NewError("Something funky with yer functions, mate!");
}

std::shared_ptr<object::Environment>
ExtendFunctionEnv(std::shared_ptr<object::Function> fun,
                  std::vector<std::shared_ptr<object::Object>> const &args)
{
    std::shared_ptr<object::Environment> new_env =
        std::make_shared<object::Environment>(fun->env_);
    if (fun->parameters_.size() != args.size())
    {
        std::cerr
            << "Function Eval - args and params not same size, ya numpty!\n";
        return new_env;
    }

    int args_len = args.size();
    for (int i = 0; i < args_len; i++)
    {
        auto param = fun->parameters_[i];
        new_env->Set(param->value_, args[i]);
    }
    return new_env;
}

std::shared_ptr<object::Object>
UnwrapReturnValue(std::shared_ptr<object::Object> obj)
{
    std::shared_ptr<object::ReturnValue> ret =
        std::dynamic_pointer_cast<object::ReturnValue>(obj);
    if (ret)
        return ret->value_;
    return obj;
}

//////////// Error shizzle below

void SSprintF(std::ostringstream &msg, const char *s)
{
    while (*s)
    {
        if (*s == '%')
        {
            if (*(s + 1) == '%')
                ++s;
            else
                msg << "ooft, really fucked up mate. disappointing.";
        }
        msg << *s++;
    }
}

template <typename T, typename... Args>
void SSprintF(std::ostringstream &msg, const char *format, T value,
              Args... args)
{
    while (*format)
    {
        if (*format == '%')
        {
            if (*(format + 1) != '%')
            {
                msg << value;

                format += 2; // only work on 2 char format strings
                SSprintF(msg, format, args...);
                return;
            }
            ++format;
        }
        msg << *format++;
    }
}

template <typename... Args>
std::shared_ptr<object::Error> NewError(std::string format, Args... args)
{

    std::ostringstream error_msg;

    SSprintF(error_msg, format.c_str(), std::forward<Args>(args)...);

    return std::make_shared<object::Error>(error_msg.str());
}

} // namespace evaluator
