#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <utils.h>

#include <audio_action_queue.h>
#include <event_queue.h>
#include <interpreter/ast.hpp>
#include <interpreter/builtins.hpp>
#include <interpreter/evaluator.hpp>
#include <interpreter/object.hpp>
#include <tsqueue.hpp>

extern Tsqueue<event_queue_item> process_event_queue;
extern Tsqueue<audio_action_queue_item> audio_queue;
extern Tsqueue<std::string> repl_queue;

namespace
{
bool IsTruthy(std::shared_ptr<object::Object> obj)
{
    if (obj == evaluator::NULLL)
        return false;
    else if (obj == evaluator::TTRUE)
        return true;
    else if (obj == evaluator::FFALSE)
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
        obj->Type() == object::NUMBER_OBJ || obj->Type() == object::STRING_OBJ)
        return true;
    return false;
}

object::HashKey MakeHashKey(std::shared_ptr<object::Object> hashkey)
{
    std::shared_ptr<object::Boolean> hash_key_bool =
        std::dynamic_pointer_cast<object::Boolean>(hashkey);
    if (hash_key_bool)
        return hash_key_bool->HashKey();

    std::shared_ptr<object::Number> hash_key_int =
        std::dynamic_pointer_cast<object::Number>(hashkey);
    if (hash_key_int)
        return hash_key_int->HashKey();

    std::shared_ptr<object::String> hash_key_string =
        std::dynamic_pointer_cast<object::String>(hashkey);
    if (hash_key_string)
        return hash_key_string->HashKey();

    return object::HashKey{};
}

bool IsValidHex(const std::string &mask)
{
    if (mask.size() != 4)
        return false;
    for (auto c : mask)
    {
        if (!('a' <= c && c <= 'f') && !('A' <= c && c <= 'Z') && !IsDigit(c))
            return false;
    }
    return true;
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
    std::shared_ptr<ast::NumberLiteral> il =
        std::dynamic_pointer_cast<ast::NumberLiteral>(node);
    if (il)
    {
        return std::make_shared<object::Number>(il->value_);
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

    std::shared_ptr<ast::LsStatement> ls_stmt =
        std::dynamic_pointer_cast<ast::LsStatement>(node);
    if (ls_stmt)
    {
        std::string lspath_string = "/";
        std::shared_ptr<ast::StringLiteral> lspath =
            std::dynamic_pointer_cast<ast::StringLiteral>(ls_stmt->path_);
        if (lspath)
            lspath_string = lspath->value_;
        std::string listing = list_sample_dir(lspath_string);
        repl_queue.push(listing);
    }

    std::shared_ptr<ast::BpmStatement> bpm_stmt =
        std::dynamic_pointer_cast<ast::BpmStatement>(node);
    if (bpm_stmt)
    {
        std::shared_ptr<ast::NumberLiteral> bpm =
            std::dynamic_pointer_cast<ast::NumberLiteral>(bpm_stmt->bpm_val_);
        if (bpm)
        {
            audio_action_queue_item action{.type = AudioAction::BPM,
                                           .new_bpm = bpm->value_};
            audio_queue.push(action);
        }
    }

    std::shared_ptr<ast::InfoStatement> info_stmt =
        std::dynamic_pointer_cast<ast::InfoStatement>(node);
    if (info_stmt)
    {
        auto soundgen_var_name = std::dynamic_pointer_cast<ast::Identifier>(
            info_stmt->soundgen_identifier_);
        if (soundgen_var_name)
        {
            auto target = Eval(soundgen_var_name, env);
            auto soundgen =
                std::dynamic_pointer_cast<object::SoundGenerator>(target);
            if (soundgen)
            {
                audio_action_queue_item action{
                    .type = AudioAction::INFO,
                    .param_name = soundgen_var_name->value_,
                    .mixer_soundgen_idx = soundgen->soundgen_id_};

                audio_queue.push(action);
            }
        }
    }

    std::shared_ptr<ast::SetStatement> set_stmt =
        std::dynamic_pointer_cast<ast::SetStatement>(node);
    if (set_stmt)
    {

        if (set_stmt->target_->token_.literal_ == ("mixer"))
        {
            audio_action_queue_item action{.type = AudioAction::MIXER_UPDATE,
                                           .param_name = set_stmt->param_,
                                           .param_val = set_stmt->value_};
            audio_queue.push(action);
            return NULLL;
        }
        auto target = Eval(set_stmt->target_, env);
        auto soundgen =
            std::dynamic_pointer_cast<object::SoundGenerator>(target);
        if (soundgen)
        {
            audio_action_queue_item action{.type = AudioAction::UPDATE,
                                           .mixer_soundgen_idx =
                                               soundgen->soundgen_id_,
                                           .fx_id = set_stmt->fx_num_,
                                           .param_name = set_stmt->param_,
                                           .param_val = set_stmt->value_};
            audio_queue.push(action);
        }
        else if (target->Type() == "ERROR")
        {
            // this section allows use of sX type identifier e.g. 's0' - needed
            // to address and disable a sound generator when i accidentally
            // create one without assigning it to a variable
            auto ident_sound_gen =
                std::dynamic_pointer_cast<ast::Identifier>(set_stmt->target_);
            if (ident_sound_gen)
            {
                std::string &sound_gen_ident = ident_sound_gen->value_;
                if (sound_gen_ident.size() >= 2)
                {
                    if (sound_gen_ident[0] == 's' &&
                        IsDigit(sound_gen_ident[1]))
                    {
                        std::stringstream ss;
                        int len_ident = sound_gen_ident.size();
                        for (int i = 1; i < len_ident; i++)
                        {
                            if (IsDigit(sound_gen_ident[i]))
                            {
                                ss << sound_gen_ident[i];
                            }
                            else
                            {
                                return NULLL;
                            }
                        }
                        audio_action_queue_item action{
                            .type = AudioAction::UPDATE,
                            .mixer_soundgen_idx = std::stoi(ss.str()),
                            .fx_id = set_stmt->fx_num_,
                            .param_name = set_stmt->param_,
                            .param_val = set_stmt->value_};
                        audio_queue.push(action);
                    }
                }
            }
        }
    }

    std::shared_ptr<ast::PanStatement> pan_stmt =
        std::dynamic_pointer_cast<ast::PanStatement>(node);
    if (pan_stmt)
    {
        auto target = Eval(pan_stmt->target_, env);
        auto soundgen =
            std::dynamic_pointer_cast<object::SoundGenerator>(target);
        if (soundgen)
        {
            audio_action_queue_item action{.type = AudioAction::UPDATE,
                                           .mixer_soundgen_idx =
                                               soundgen->soundgen_id_,
                                           .param_name = "pan",
                                           .param_val = pan_stmt->value_};
            audio_queue.push(action);
        }
    }

    std::shared_ptr<ast::PlayStatement> play_expr =
        std::dynamic_pointer_cast<ast::PlayStatement>(node);
    if (play_expr)
    {
        if (!play_expr->path_.empty())
        {
            audio_action_queue_item action{.type = AudioAction::PREVIEW,
                                           .preview_filename =
                                               play_expr->path_};
            audio_queue.push(action);
        }
    }

    std::shared_ptr<ast::PsStatement> ps_expr =
        std::dynamic_pointer_cast<ast::PsStatement>(node);
    if (ps_expr)
    {
        audio_action_queue_item action_req{.type = AudioAction::STATUS,
                                           .status_all = ps_expr->all_};
        audio_queue.push(action_req);
    }

    std::shared_ptr<ast::HelpStatement> help_expr =
        std::dynamic_pointer_cast<ast::HelpStatement>(node);
    if (help_expr)
    {
        audio_action_queue_item action_req{.type = AudioAction::HELP};
        audio_queue.push(action_req);
    }

    std::shared_ptr<ast::VolumeStatement> vol_stmt =
        std::dynamic_pointer_cast<ast::VolumeStatement>(node);
    if (vol_stmt)
    {
        auto target = Eval(vol_stmt->target_, env);
        auto soundgen =
            std::dynamic_pointer_cast<object::SoundGenerator>(target);
        if (soundgen)
        {
            audio_action_queue_item action{.type = AudioAction::UPDATE,
                                           .mixer_soundgen_idx =
                                               soundgen->soundgen_id_,
                                           .param_name = "volume",
                                           .param_val = vol_stmt->value_};
            audio_queue.push(action);
        }
    }

    ///////////////////////////////////////////////////////////////

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
        {
            return ApplyFunction(builtin_func, args);
        }

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

    std::shared_ptr<ast::SynthExpression> synth =
        std::dynamic_pointer_cast<ast::SynthExpression>(node);
    if (synth)
    {
        if (synth->token_.type_ == token::SLANG_MOOG_SYNTH)
            return std::make_shared<object::MoogSynth>();
        else if (synth->token_.type_ == token::SLANG_FM_SYNTH)
            return std::make_shared<object::FMSynth>();
        else if (synth->token_.type_ == token::SLANG_DRUM_SYNTH)
            return std::make_shared<object::DrumSynth>();
    }

    std::shared_ptr<ast::SynthPresetExpression> synth_preset =
        std::dynamic_pointer_cast<ast::SynthPresetExpression>(node);
    if (synth_preset)
    {
        if (synth_preset->token_.type_ == token::SLANG_MOOG_SYNTH)
        {
            sequence_engine_list_presets(MINISYNTH_TYPE);
        }
        else if (synth_preset->token_.type_ == token::SLANG_FM_SYNTH)
        {
            sequence_engine_list_presets(DXSYNTH_TYPE);
        }
        else
            std::cerr << "NOT A SYNTH TYPE!\n";
    }

    std::shared_ptr<ast::SampleExpression> sample =
        std::dynamic_pointer_cast<ast::SampleExpression>(node);
    if (sample)
    {
        if (!sample->path_.empty())
            return std::make_shared<object::Sample>(sample->path_);
        else
            std::cerr << "Nae sample path!!\n";
    }

    std::shared_ptr<ast::GranularExpression> gran =
        std::dynamic_pointer_cast<ast::GranularExpression>(node);
    if (gran)
    {
        if (!gran->path_.empty())
        {
            if (gran->loop_mode_)
            {
                return std::make_shared<object::Granular>(gran->path_, true);
            }
            else
                return std::make_shared<object::Granular>(gran->path_, false);
        }
        else
            std::cerr << "Nae sample path!!\n";
    }

    std::shared_ptr<ast::ProcessStatement> proc =
        std::dynamic_pointer_cast<ast::ProcessStatement>(node);
    if (proc)
        return EvalProcessStatement(proc, env);

    return NULLL;
}

std::shared_ptr<object::Object>
EvalIndexExpression(std::shared_ptr<object::Object> left,
                    std::shared_ptr<object::Object> index)
{
    if (left->Type() == object::ARRAY_OBJ &&
        index->Type() == object::NUMBER_OBJ)
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

    std::shared_ptr<object::Number> int_obj =
        std::dynamic_pointer_cast<object::Number>(index);
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
        std::cout << "TRUTHY\n";
        result = Eval(for_loop->body_, new_env);
        std::cout << "RESULT? " << result << std::endl;
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
    if (left->Type() == object::NUMBER_OBJ &&
        right->Type() == object::NUMBER_OBJ)
    {
        auto leftie = std::dynamic_pointer_cast<object::Number>(left);
        auto rightie = std::dynamic_pointer_cast<object::Number>(right);
        return EvalNumberInfixExpression(op, leftie, rightie);
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
EvalNumberInfixExpression(std::string op, std::shared_ptr<object::Number> left,
                          std::shared_ptr<object::Number> right)
{

    if (op.compare("+") == 0)
        return std::make_shared<object::Number>(left->value_ + right->value_);
    else if (op.compare("-") == 0)
        return std::make_shared<object::Number>(left->value_ - right->value_);
    else if (op.compare("*") == 0)
        return std::make_shared<object::Number>(left->value_ * right->value_);
    else if (op.compare("/") == 0)
        return std::make_shared<object::Number>(left->value_ / right->value_);
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
    if (right == TTRUE)
        return FFALSE;
    else if (right == FFALSE)
        return TTRUE;
    else if (right == NULLL)
        return TTRUE;
    else
        return FFALSE;
}

std::shared_ptr<object::Object>
EvalMinusPrefixOperatorExpression(std::shared_ptr<object::Object> right)
{
    std::shared_ptr<object::Number> i =
        std::dynamic_pointer_cast<object::Number>(right);
    if (!i)
    {
        return NewError("unknown operator: -%s", right->Type());
    }

    return std::make_shared<object::Number>(-i->value_);
}

std::shared_ptr<object::Object>
EvalIncrementOperatorExpression(std::shared_ptr<object::Object> right)
{
    std::shared_ptr<object::Number> i =
        std::dynamic_pointer_cast<object::Number>(right);
    if (!i)
    {
        return NewError("unknown operator: ++%s", right->Type());
    }

    return std::make_shared<object::Number>(++(i->value_));
}

std::shared_ptr<object::Object>
EvalDecrementOperatorExpression(std::shared_ptr<object::Object> right)
{
    std::shared_ptr<object::Number> i =
        std::dynamic_pointer_cast<object::Number>(right);
    if (!i)
    {
        return NewError("unknown operator: --%s", right->Type());
    }

    return std::make_shared<object::Number>(--(i->value_));
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
        return TTRUE;
    return FFALSE;
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

    return NewError("Something stinky wit yer functions, mate!");
}

std::shared_ptr<object::Environment>
ExtendFunctionEnv(std::shared_ptr<object::Function> fun,
                  std::vector<std::shared_ptr<object::Object>> const &args)
{
    std::shared_ptr<object::Environment> new_env =
        std::make_shared<object::Environment>(fun->env_);
    if (fun->parameters_.size() != args.size())
    {
        std::cerr << "Function Eval - args and params not same size, ya "
                     "numpty!\n";
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

std::shared_ptr<PatternFunction>
EvalPatternFunctionExpression(std::shared_ptr<ast::Expression> funct)
{
    auto func =
        std::dynamic_pointer_cast<ast::PatternFunctionExpression>(funct);

    if (!func)
    {
        std::cerr << "DIDNAE CAST YER FUNC\n";
        return nullptr;
    }

    if (func->token_.literal_ == "arp")
    {
        auto arp_func = std::make_shared<PatternArp>();
        for (auto a : func->arguments_)
        {
            auto speedliteral =
                std::dynamic_pointer_cast<ast::NumberLiteral>(a);
            if (speedliteral)
            {
                int speedval = speedliteral->value_;

                if (speedval == 16)
                    arp_func->speed_ = ArpSpeed::ARP_16;
                else if (speedval == 8)
                    arp_func->speed_ = ArpSpeed::ARP_8;
                else if (speedval == 4)
                    arp_func->speed_ = ArpSpeed::ARP_4;
            }
            auto directionliteral =
                std::dynamic_pointer_cast<ast ::Identifier>(a);
            if (directionliteral)
            {
                std::string directionval = directionliteral->value_;
                if (directionval == "up")
                    arp_func->direction_ = ArpDirection::ARP_UP;
                else if (directionval == "down")
                    arp_func->direction_ = ArpDirection::ARP_DOWN;
                else if (directionval == "updown")
                    arp_func->direction_ = ArpDirection::ARP_UPDOWN;
                else if (directionval == "rand")
                    arp_func->direction_ = ArpDirection::ARP_RAND;
                else if (directionval == "repeat")
                    arp_func->direction_ = ArpDirection::ARP_REPEAT;
            }
        }
        return arp_func;
    }
    else if (func->token_.literal_ == "brak")
    {
        return std::make_shared<PatternBrak>();
    }
    else if (func->token_.literal_ == "bump")
    {
        auto bump_func = std::make_shared<PatternBump>();
        if (func->arguments_.size() == 1)
        {
            auto bump_literal = std::dynamic_pointer_cast<ast::NumberLiteral>(
                func->arguments_[0]);
            if (bump_literal)
            {
                if (bump_literal->value_ >= 0 &&
                    bump_literal->value_ < PPSIXTEENTH)
                    bump_func->bump_amount_ = bump_literal->value_;
            }
        }
        return bump_func;
    }
    else if (func->token_.literal_ == "chord")
    {
        return std::make_shared<PatternChord>();
    }
    else if (func->token_.literal_ == "every")
    {
        auto intval =
            std::dynamic_pointer_cast<ast::NumberLiteral>(func->arguments_[0]);
        if (!intval)
        {
            std::cerr << "NAE NAE!\n";
            return nullptr;
        }

        auto func_arg_ast = std::make_shared<ast::PatternFunctionExpression>(
            func->arguments_[1]->token_);

        if (func_arg_ast)
        {
            int args_size = func->arguments_.size();
            if (args_size > 2)
            {
                for (int i = 2; i < args_size; i++)
                    func_arg_ast->arguments_.push_back(func->arguments_[i]);
            }
            auto func_arg = EvalPatternFunctionExpression(func_arg_ast);
            if (func_arg)
            {
                auto p_every =
                    std::make_shared<PatternEvery>(intval->value_, func_arg);
                return p_every;
            }
        }
    }
    else if (func->token_.literal_ == "mask")
    {
        if (func->arguments_.size() == 1)
        {
            auto mask_string = std::dynamic_pointer_cast<ast::StringLiteral>(
                func->arguments_[0]);
            if (mask_string)
            {
                auto mask = mask_string->value_;
                if (IsValidHex(mask))
                {
                    return std::make_shared<PatternMask>(mask);
                }
            }
        }
    }
    else if (func->token_.literal_ == "power")
    {
        return std::make_shared<PatternPowerChord>();
    }
    else if (func->token_.literal_ == "rev")
    {
        return std::make_shared<PatternReverse>();
    }
    else if (func->token_.literal_ == "rotl" || func->token_.literal_ == "rotr")
    {
        int num_sixteenth_steps = 1;
        if (func->arguments_.size() > 0)
        {
            auto intval = std::dynamic_pointer_cast<ast::NumberLiteral>(
                func->arguments_[0]);
            if (intval)
                num_sixteenth_steps = intval->value_;
        }

        if (func->token_.literal_ == "rotl")
            return std::make_shared<PatternRotate>(LEFT, num_sixteenth_steps);
        else
            return std::make_shared<PatternRotate>(RIGHT, num_sixteenth_steps);
    }
    else if (func->token_.literal_ == "scramble")
    {
        return std::make_shared<PatternScramble>();
    }
    else if (func->token_.literal_ == "speed")
    {
        if (func->arguments_.size() == 1)
        {
            auto multi = std::dynamic_pointer_cast<ast::NumberLiteral>(
                func->arguments_[0]);
            if (multi)
            {
                double speed_multi = multi->value_;
                return std::make_shared<PatternSpeed>(speed_multi);
            }
        }
        std::cerr << "Need a speed adjustment multiplier.." << std::endl;
    }
    else if (func->token_.literal_ == "swing")
    {
        int swing_setting = 50;
        if (func->arguments_.size() > 0)
        {
            auto intval = std::dynamic_pointer_cast<ast::NumberLiteral>(
                func->arguments_[0]);
            if (intval)
                swing_setting = intval->value_;
        }
        return std::make_shared<PatternSwing>(swing_setting);
    }
    else if (func->token_.literal_ == "up" || func->token_.literal_ == "down")
    {
        int num_octaves = 1;
        if (func->arguments_.size() > 0)
        {
            auto intval = std::dynamic_pointer_cast<ast::NumberLiteral>(
                func->arguments_[0]);
            if (intval)
                num_octaves = intval->value_;
        }
        if (func->token_.literal_ == "up")
            return std::make_shared<PatternTranspose>(UP, num_octaves);
        else
            return std::make_shared<PatternTranspose>(DOWN, num_octaves);
    }
    else if (func->token_.literal_ == "while")
    {
        std::cout << "WHILE!!\n";
    }
    else
    {
        std::cout << "NAH MAN, DIDN't GET YER FUNCTION 0- i got "
                  << func->String() << std::endl;
    }
    return nullptr;
}

std::shared_ptr<object::Object>
EvalProcessStatement(std::shared_ptr<ast::ProcessStatement> proc,
                     std::shared_ptr<object::Environment> env)
{
    auto pattern_obj = Eval(proc->pattern_, env);
    if (pattern_obj->Type() == "STRING" ||
        proc->process_timer_type_ == ProcessTimerType::WHILE)
    {
        auto pattern = std::dynamic_pointer_cast<object::String>(pattern_obj);

        std::vector<std::shared_ptr<PatternFunction>> process_funcz;

        for (auto &f : proc->functions_)
        {
            auto funcy = EvalPatternFunctionExpression(f);
            if (funcy)
                process_funcz.push_back(funcy);
        }

        event_queue_item ev;
        ev.type = Event::PROCESS_UPDATE_EVENT;
        ev.target_process_id = proc->mixer_process_id_;
        ev.process_type = proc->process_type_;
        ev.timer_type = proc->process_timer_type_;
        ev.loop_len = proc->loop_len_;
        ev.command = proc->command_;
        ev.target_type = proc->target_type_;
        ev.targets = proc->targets_;
        ev.pattern = pattern->value_;
        ev.while_condition = proc->while_condition_;
        ev.while_body = proc->while_body_;
        ev.while_then_body = proc->while_then_body_;
        ev.funcz = process_funcz;
        process_event_queue.push(ev);
    }
    else
        std::cout << "Nae PATTERMN!! its a " << pattern_obj->Type() << "\n";

    return NULLL;
} // namespace evaluator

//////////// Error shizzle below

void SSprintF(std::ostringstream &msg, const char *s);
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
