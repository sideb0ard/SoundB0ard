#include <audio_action_queue.h>
#include <event_queue.h>
#include <obliquestrategies.h>
#include <utils.h>

#include <cstring>
#include <interpreter/ast.hpp>
#include <interpreter/builtins.hpp>
#include <interpreter/evaluator.hpp>
#include <interpreter/object.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <tsqueue.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

extern Tsqueue<event_queue_item> process_event_queue;
extern Tsqueue<std::unique_ptr<AudioActionItem>> audio_queue;
extern Tsqueue<std::string> repl_queue;
extern Tsqueue<int> audio_reply_queue;

namespace {

bool IsTruthy(std::shared_ptr<object::Object> obj) {
  if (obj == evaluator::NULLL || obj == evaluator::FFALSE ||
      obj->Type() == object::ERROR_OBJ)
    return false;

  return true;
}

bool IsError(std::shared_ptr<object::Object> obj) {
  if (obj) {
    return obj->Type() == object::ERROR_OBJ;
  }
  return false;
}

bool IsHashable(std::shared_ptr<object::Object> obj) {
  if (obj->Type() == object::BOOLEAN_OBJ || obj->Type() == object::NUMBER_OBJ ||
      obj->Type() == object::STRING_OBJ ||
      object::IsSoundGenerator(obj->Type()))
    return true;
  return false;
}

object::HashKey MakeHashKey(std::shared_ptr<object::Object> hashkey) {
  auto hash_key_bool = std::dynamic_pointer_cast<object::Boolean>(hashkey);
  if (hash_key_bool) return hash_key_bool->HashKey();

  auto hash_key_int = std::dynamic_pointer_cast<object::Number>(hashkey);
  if (hash_key_int) return hash_key_int->HashKey();

  auto hash_key_string = std::dynamic_pointer_cast<object::String>(hashkey);
  if (hash_key_string) return hash_key_string->HashKey();

  auto hash_key_soundgenerator =
      std::dynamic_pointer_cast<object::SoundGenerator>(hashkey);
  if (hash_key_soundgenerator) return hash_key_soundgenerator->HashKey();

  return object::HashKey{};
}

bool IsValidHex(const std::string &mask) {
  if (mask.size() != 4) return false;
  for (auto c : mask) {
    if (!('a' <= c && c <= 'f') && !('A' <= c && c <= 'Z') && !IsDigit(c))
      return false;
  }
  return true;
}

std::shared_ptr<object::String> NumberToString(
    std::shared_ptr<object::Number> num_obj) {
  auto string_value = std::to_string(num_obj->value_);
  return std::make_shared<object::String>(string_value);
}

}  // namespace

namespace evaluator {

std::shared_ptr<object::Object> Eval(std::shared_ptr<ast::Node> node,
                                     std::shared_ptr<object::Environment> env) {
  if (!node) return NULLL;

  std::shared_ptr<ast::Program> prog_node =
      std::dynamic_pointer_cast<ast::Program>(node);
  if (prog_node) return EvalProgram(prog_node->statements_, env);

  std::shared_ptr<ast::BreakStatement> break_statement_node =
      std::dynamic_pointer_cast<ast::BreakStatement>(node);
  if (break_statement_node) return std::make_shared<object::Break>();

  std::shared_ptr<ast::BlockStatement> block_statement_node =
      std::dynamic_pointer_cast<ast::BlockStatement>(node);
  if (block_statement_node)
    return EvalBlockStatement(block_statement_node, env);

  std::shared_ptr<ast::ExpressionStatement> expr_statement_node =
      std::dynamic_pointer_cast<ast::ExpressionStatement>(node);
  if (expr_statement_node) {
    return Eval(expr_statement_node->expression_, env);
  }

  std::shared_ptr<ast::ReturnStatement> return_statement_node =
      std::dynamic_pointer_cast<ast::ReturnStatement>(node);
  if (return_statement_node) {
    auto val = Eval(return_statement_node->return_value_, env);
    if (IsError(val)) return val;
    return std::make_shared<object::ReturnValue>(val);
  }

  // Expressions
  std::shared_ptr<ast::NumberLiteral> il =
      std::dynamic_pointer_cast<ast::NumberLiteral>(node);
  if (il) {
    return std::make_shared<object::Number>(il->value_);
  }

  std::shared_ptr<ast::BooleanExpression> be =
      std::dynamic_pointer_cast<ast::BooleanExpression>(node);
  if (be) {
    return NativeBoolToBooleanObject(be->value_);
  }

  std::shared_ptr<ast::PrefixExpression> pe =
      std::dynamic_pointer_cast<ast::PrefixExpression>(node);
  if (pe) {
    auto right = Eval(pe->right_, env);
    if (IsError(right)) return right;
    return EvalPrefixExpression(pe->operator_, right);
  }

  std::shared_ptr<ast::InfixExpression> ie =
      std::dynamic_pointer_cast<ast::InfixExpression>(node);
  if (ie) {
    auto left = Eval(ie->left_, env);
    if (IsError(left)) return left;

    if (ie->operator_ == "&&" && !IsTruthy(left)) {
      return FFALSE;
    }

    auto right = Eval(ie->right_, env);
    if (IsError(right)) return right;

    return EvalInfixExpression(ie->operator_, left, right);
  }

  std::shared_ptr<ast::IfExpression> if_expr =
      std::dynamic_pointer_cast<ast::IfExpression>(node);
  if (if_expr) {
    return EvalIfExpression(if_expr, env);
  }

  std::shared_ptr<ast::LetStatement> let_expr =
      std::dynamic_pointer_cast<ast::LetStatement>(node);
  if (let_expr) {
    // first check if its a sample expression with same path and if so, do
    // nothing so we don't keep reloading same sample
    auto sample_expression =
        std::dynamic_pointer_cast<ast::SampleExpression>(let_expr->value_);
    if (sample_expression) {
      auto sample = env->Get(let_expr->name_->value_);
      if (sample && sample->Type() == "SAMPLE") {
        auto sample_obj = std::dynamic_pointer_cast<object::Sample>(sample);
        if (sample_obj)
          if (sample_obj->sample_path_.compare(sample_expression->path_) == 0)
            return NULLL;
      }
    }

    auto val = Eval(let_expr->value_, env);
    if (IsError(val)) {
      return val;
    }
    env->Set(let_expr->name_->value_, val, let_expr->is_new_item);
  }

  std::shared_ptr<ast::LsStatement> ls_stmt =
      std::dynamic_pointer_cast<ast::LsStatement>(node);
  if (ls_stmt) {
    std::string lspath_string = "/";
    std::shared_ptr<ast::StringLiteral> lspath =
        std::dynamic_pointer_cast<ast::StringLiteral>(ls_stmt->path_);
    if (lspath) lspath_string = lspath->value_;
    std::string listing = list_sample_dir(lspath_string);
    repl_queue.push(listing);
  }

  std::shared_ptr<ast::BpmStatement> bpm_stmt =
      std::dynamic_pointer_cast<ast::BpmStatement>(node);
  if (bpm_stmt) {
    std::shared_ptr<ast::NumberLiteral> bpm =
        std::dynamic_pointer_cast<ast::NumberLiteral>(bpm_stmt->bpm_val_);
    if (bpm) {
      auto action = std::make_unique<AudioActionItem>(AudioAction::BPM);
      action->new_bpm = bpm->value_;
      audio_queue.push(std::move(action));
    }
  }

  auto bitop_stmt = std::dynamic_pointer_cast<ast::BitOpExpression>(node);
  if (bitop_stmt) {
    auto bitoo_obj = std::make_shared<object::BitOp>(bitop_stmt->value_);
    return bitoo_obj;
  }

  std::shared_ptr<ast::InfoStatement> info_stmt =
      std::dynamic_pointer_cast<ast::InfoStatement>(node);
  if (info_stmt) {
    auto soundgen_var_name = std::dynamic_pointer_cast<ast::Identifier>(
        info_stmt->soundgen_identifier_);
    if (soundgen_var_name) {
      auto target = Eval(soundgen_var_name, env);
      auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(target);
      if (soundgen) {
        auto action = std::make_unique<AudioActionItem>(AudioAction::INFO);
        action->param_name = soundgen_var_name->value_;
        action->mixer_soundgen_idx = soundgen->soundgen_id_;

        audio_queue.push(std::move(action));
      }
    }
  }

  auto pset_stmt = std::dynamic_pointer_cast<ast::ProcessSetStatement>(node);
  if (pset_stmt) {
    return EvalProcessSetStatement(pset_stmt, env);
  }

  std::shared_ptr<ast::SetStatement> set_stmt =
      std::dynamic_pointer_cast<ast::SetStatement>(node);
  if (set_stmt) {
    auto eval_val = Eval(set_stmt->value_, env);
    if (eval_val->Type() == "ERROR") {
      std::cerr << "COuldn't EVAL your statement value!!\n";
      return NULLL;
    }
    auto val = eval_val->Inspect();

    int delayed_by = 0;
    auto eval_when = Eval(set_stmt->when_, env);
    if (eval_when->Type() == "AT") {
      delayed_by = std::stoi(eval_when->Inspect());
    }

    if (set_stmt->target_->token_.literal_ == ("mixer")) {
      auto action =
          std::make_unique<AudioActionItem>(AudioAction::MIXER_UPDATE);
      action->mixer_fx_id = set_stmt->mixer_fx_num_;
      action->is_xfader = set_stmt->is_xfader_component_;
      action->delayed_by = delayed_by;
      action->param_name = set_stmt->param_;
      action->param_val = val;
      audio_queue.push(std::move(action));
      return NULLL;
    }
    auto target = Eval(set_stmt->target_, env);
    auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(target);
    if (soundgen) {
      auto action = std::make_unique<AudioActionItem>(AudioAction::UPDATE);
      action->mixer_soundgen_idx = soundgen->soundgen_id_;
      action->fx_id = set_stmt->fx_num_;
      action->param_name = set_stmt->param_;
      action->delayed_by = delayed_by;
      action->param_val = val;
      audio_queue.push(std::move(action));
      return NULLL;
    }

    auto stepper = std::dynamic_pointer_cast<object::StepSequencer>(target);
    if (stepper) {
      try {
        double param_val = std::stod(val);
        stepper->steppa_.SetParam(set_stmt->param_, param_val);
      } catch (std::invalid_argument const &ex) {
        std::cerr << "Not a number:" << val << " " << ex.what() << std::endl;
      }
    }

    if (target->Type() == "ERROR") {
      // this section allows use of sX type identifier e.g. 's0' - needed
      // to address and disable a sound generator when i accidentally
      // create one without assigning it to a variable
      auto ident_sound_gen =
          std::dynamic_pointer_cast<ast::Identifier>(set_stmt->target_);
      if (ident_sound_gen) {
        std::string &sound_gen_ident = ident_sound_gen->value_;
        if (sound_gen_ident.size() >= 2) {
          if (sound_gen_ident[0] == 's' && IsDigit(sound_gen_ident[1])) {
            std::stringstream ss;
            int len_ident = sound_gen_ident.size();
            for (int i = 1; i < len_ident; i++) {
              if (IsDigit(sound_gen_ident[i])) {
                ss << sound_gen_ident[i];
              } else {
                return NULLL;
              }
            }
            auto action =
                std::make_unique<AudioActionItem>(AudioAction::UPDATE);
            action->mixer_soundgen_idx = std::stoi(ss.str());
            action->fx_id = set_stmt->fx_num_;
            action->param_name = set_stmt->param_;
            action->param_val = val;
            audio_queue.push(std::move(action));
          }
        }
      }
    }
  }

  std::shared_ptr<ast::PanStatement> pan_stmt =
      std::dynamic_pointer_cast<ast::PanStatement>(node);
  if (pan_stmt) {
    auto target = Eval(pan_stmt->target_, env);
    auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(target);
    auto value = Eval(pan_stmt->value_, env);
    if (soundgen && value) {
      auto action = std::make_unique<AudioActionItem>(AudioAction::UPDATE);
      action->mixer_soundgen_idx = soundgen->soundgen_id_;
      action->param_name = "pan";
      action->param_val = value->Inspect();
      audio_queue.push(std::move(action));
    }
  }

  std::shared_ptr<ast::PlayStatement> play_expr =
      std::dynamic_pointer_cast<ast::PlayStatement>(node);
  if (play_expr) {
    if (!play_expr->path_.empty()) {
      //  import file and add decoded content
      auto action = std::make_unique<AudioActionItem>(AudioAction::PREVIEW);
      action->preview_filename = play_expr->path_;
      action->audio_buffer_details =
          ImportFileContents(action->buffer, action->preview_filename);
      audio_queue.push(std::move(action));
    }
  }

  auto step_expr =
      std::dynamic_pointer_cast<ast::StepSequencerExpression>(node);
  if (step_expr) {
    auto sequ = Eval(step_expr->sequence_, env);
    auto sequ_array = std::dynamic_pointer_cast<object::Array>(sequ);
    if (sequ_array) {
      std::vector<double> the_vals{};
      for (const auto &i : sequ_array->elements_) {
        auto num = std::dynamic_pointer_cast<object::Number>(i);
        if (num) the_vals.push_back(num->value_);
      }
      return std::make_shared<object::StepSequencer>(the_vals);
    }
    // if seq == Array ->
    //  construct a vector from elements
    //  and create a new StepSequencer
    // if (!play_expr->path_.empty()) {
    //  //  import file and add decoded content
    //  auto action = std::make_unique<AudioActionItem>(AudioAction::PREVIEW);
    //  action->preview_filename = play_expr->path_;
    //  action->audio_buffer_details =
    //      ImportFileContents(action->buffer, action->preview_filename);
    //  audio_queue.push(std::move(action));
    //}
  }

  std::shared_ptr<ast::PsStatement> ps_expr =
      std::dynamic_pointer_cast<ast::PsStatement>(node);
  if (ps_expr) {
    auto action = std::make_unique<AudioActionItem>(AudioAction::STATUS);
    action->status_all = ps_expr->all_;
    audio_queue.push(std::move(action));
  }

  std::shared_ptr<ast::HelpStatement> help_expr =
      std::dynamic_pointer_cast<ast::HelpStatement>(node);
  if (help_expr) {
    auto action = std::make_unique<AudioActionItem>(AudioAction::HELP);
    audio_queue.push(std::move(action));
  }

  std::shared_ptr<ast::StrategyStatement> strat_expr =
      std::dynamic_pointer_cast<ast::StrategyStatement>(node);
  if (strat_expr) {
    std::string strategy = oblique_strategy();
    repl_queue.push(strategy);
  }

  std::shared_ptr<ast::VolumeStatement> vol_stmt =
      std::dynamic_pointer_cast<ast::VolumeStatement>(node);
  if (vol_stmt) {
    auto action = std::make_unique<AudioActionItem>(AudioAction::UPDATE);
    action->param_name = "volume";

    auto target = Eval(vol_stmt->target_, env);
    auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(target);

    auto value = Eval(vol_stmt->value_, env);

    if (soundgen && value) {
      action->param_val = value->Inspect();
      action->mixer_soundgen_idx = soundgen->soundgen_id_;
      audio_queue.push(std::move(action));
    }
  }

  ///////////////////////////////////////////////////////////////

  std::shared_ptr<ast::Identifier> ident =
      std::dynamic_pointer_cast<ast::Identifier>(node);
  if (ident) {
    return EvalIdentifier(ident, env);
  }

  std::shared_ptr<ast::PhasorLiteral> ph =
      std::dynamic_pointer_cast<ast::PhasorLiteral>(node);
  if (ph) {
    std::cout << "GOT A PHASOR!\n";
    auto val = Eval(ph->frequency_, env);
    auto int_obj = std::dynamic_pointer_cast<object::Number>(val);
    if (int_obj) {
      std::cout << "Oh, we good!- freq is " << int_obj->value_ << "\n";
      return std::make_shared<object::Phasor>(int_obj->value_);
    }
  }

  std::shared_ptr<ast::FunctionLiteral> fn =
      std::dynamic_pointer_cast<ast::FunctionLiteral>(node);
  if (fn) {
    auto params = fn->parameters_;
    auto body = fn->body_;
    return std::make_shared<object::Function>(params, env, body);
  }

  std::shared_ptr<ast::ForStatement> forloop =
      std::dynamic_pointer_cast<ast::ForStatement>(node);
  if (forloop) {
    auto it = forloop->iterator_;
    auto iterator_value = forloop->iterator_value_;
    auto termination_condition = forloop->termination_condition_;
    auto increment = forloop->increment_;
    auto body = forloop->body_;
    auto new_env = std::make_shared<object::Environment>(env);
    auto forloop_obj = std::make_shared<object::ForLoop>(
        new_env, it, iterator_value, termination_condition, increment, body);

    return EvalForLoop(forloop_obj);
  }

  std::shared_ptr<ast::GeneratorLiteral> gn =
      std::dynamic_pointer_cast<ast::GeneratorLiteral>(node);
  if (gn) {
    auto params = gn->parameters_;
    auto setup = gn->setup_;
    auto run = gn->run_;
    auto signal_generator = gn->signal_generator_;
    auto new_env = std::make_shared<object::Environment>(env);
    auto gen = std::make_shared<object::Generator>(params, new_env, setup, run,
                                                   signal_generator);
    Eval(gen->setup_, gen->env_);
    return gen;
  }

  std::shared_ptr<ast::CallExpression> call_expr =
      std::dynamic_pointer_cast<ast::CallExpression>(node);
  if (call_expr) {
    auto fun = Eval(call_expr->function_, env);
    if (IsError(fun)) return fun;

    std::vector<std::shared_ptr<object::Object>> args =
        EvalExpressions(call_expr->arguments_, env);
    if (args.size() == 1 && IsError(args[0])) return args[0];

    auto func_obj = std::dynamic_pointer_cast<object::Function>(fun);
    if (func_obj) return ApplyFunction(func_obj, args);

    auto gen_obj = std::dynamic_pointer_cast<object::Generator>(fun);
    if (gen_obj) return ApplyGeneratorRun(gen_obj);

    auto builtin_func = std::dynamic_pointer_cast<object::BuiltIn>(fun);
    if (builtin_func) {
      return ApplyFunction(builtin_func, args);
    }

    return NewError("Not a function object, mate:%s!", fun->Type());
  }

  std::shared_ptr<ast::StringLiteral> sliteral =
      std::dynamic_pointer_cast<ast::StringLiteral>(node);
  if (sliteral) {
    return std::make_shared<object::String>(sliteral->value_);
  }

  std::shared_ptr<ast::ArrayLiteral> aliteral =
      std::dynamic_pointer_cast<ast::ArrayLiteral>(node);
  if (aliteral) {
    std::vector<std::shared_ptr<object::Object>> elements =
        EvalExpressions(aliteral->elements_, env);
    if (elements.size() == 1 && IsError(elements[0])) return elements[0];
    return std::make_shared<object::Array>(elements);
  }

  std::shared_ptr<ast::IndexExpression> index_x =
      std::dynamic_pointer_cast<ast::IndexExpression>(node);
  if (index_x) {
    std::shared_ptr<object::Object> left = Eval(index_x->left_, env);
    if (IsError(left)) return left;

    std::shared_ptr<object::Object> index = Eval(index_x->index_, env);
    if (IsError(index)) return index;

    if (index_x->new_value_) {
      std::shared_ptr<object::Object> new_value =
          Eval(index_x->new_value_, env);
      if (IsError(new_value)) return new_value;
      EvalIndexExpressionUpdate(left, index, new_value);
    }

    return EvalIndexExpression(left, index);
  }

  std::shared_ptr<ast::HashLiteral> hash_lit =
      std::dynamic_pointer_cast<ast::HashLiteral>(node);
  if (hash_lit) {
    return EvalHashLiteral(hash_lit, env);
  }

  std::shared_ptr<ast::SynthExpression> synth =
      std::dynamic_pointer_cast<ast::SynthExpression>(node);
  if (synth) {
    if (synth->token_.type_ == token::SLANG_MOOG_SYNTH)
      return std::make_shared<object::MoogSynth>();
    else if (synth->token_.type_ == token::SLANG_FM_SYNTH)
      return std::make_shared<object::FMSynth>();
    else if (synth->token_.type_ == token::SLANG_SB_SYNTH)
      return std::make_shared<object::SBSynth>();
    else if (synth->token_.type_ == token::SLANG_DRUM_SYNTH)
      return std::make_shared<object::DrumSynth>();
  }

  std::shared_ptr<ast::SynthPresetExpression> synth_preset =
      std::dynamic_pointer_cast<ast::SynthPresetExpression>(node);
  if (synth_preset) {
    if (synth_preset->token_.type_ == token::SLANG_MOOG_SYNTH) {
      GetSynthPresets(MINISYNTH_TYPE);
      std::cout << "NOOP - FIX ME?\n";
    } else if (synth_preset->token_.type_ == token::SLANG_FM_SYNTH) {
      GetSynthPresets(DXSYNTH_TYPE);
      std::cout << "NOOP - FIX ME?\n";
    } else
      std::cerr << "NOT A SYNTH TYPE!\n";
  }

  std::shared_ptr<ast::SampleExpression> sample =
      std::dynamic_pointer_cast<ast::SampleExpression>(node);
  if (sample) {
    if (utils::FileExists(sample->path_)) {
      return std::make_shared<object::Sample>(sample->path_);
    } else
      std::cerr << "Nae sample path!!\n";
  }

  std::shared_ptr<ast::PatternExpression> pattern =
      std::dynamic_pointer_cast<ast::PatternExpression>(node);
  if (pattern) {
    if (!pattern->string_pattern.empty())
      return std::make_shared<object::Pattern>(pattern->string_pattern);
    else
      std::cerr << "Nae pattern?!!\n";
  }

  std::shared_ptr<ast::GranularExpression> gran =
      std::dynamic_pointer_cast<ast::GranularExpression>(node);
  if (gran) {
    if (!gran->path_.empty()) {
      return std::make_shared<object::Granular>(gran->path_, gran->loop_mode_);
    } else
      std::cerr << "Nae sample path!!\n";
  }

  std::shared_ptr<ast::ProcessStatement> proc =
      std::dynamic_pointer_cast<ast::ProcessStatement>(node);
  if (proc) return EvalProcessStatement(proc, env);

  std::shared_ptr<ast::AtExpression> at_exp =
      std::dynamic_pointer_cast<ast::AtExpression>(node);
  if (at_exp) {
    auto val = Eval(at_exp->midi_ticks_from_now, env);
    auto int_obj = std::dynamic_pointer_cast<object::Number>(val);
    if (int_obj) {
      return std::make_shared<object::At>(int_obj->value_);
    }
  }

  std::shared_ptr<ast::DurationExpression> dur_exp =
      std::dynamic_pointer_cast<ast::DurationExpression>(node);
  if (dur_exp) {
    auto val = Eval(dur_exp->duration_val, env);
    auto int_obj = std::dynamic_pointer_cast<object::Number>(val);
    if (int_obj) {
      return std::make_shared<object::Duration>(int_obj->value_);
    }
  }

  std::shared_ptr<ast::VelocityExpression> vel_exp =
      std::dynamic_pointer_cast<ast::VelocityExpression>(node);
  if (vel_exp) {
    auto val = Eval(vel_exp->velocity_val, env);
    auto int_obj = std::dynamic_pointer_cast<object::Number>(val);
    if (int_obj) {
      return std::make_shared<object::Velocity>(int_obj->value_);
    }
  }

  auto midi_array_exp =
      std::dynamic_pointer_cast<ast::MidiArrayExpression>(node);
  if (midi_array_exp) {
    auto elements = Eval(midi_array_exp->elements_, env);
    std::vector<midi_event> events;

    auto el_events = std::dynamic_pointer_cast<object::Hash>(elements);
    if (el_events) {
      std::string delimiter = ":";
      for (auto const &it : el_events->pairs_) {
        std::string midi_tick = it.second.key_->Inspect();
        std::string note_and_dur = it.second.value_->Inspect();

        int num_delim = count(note_and_dur.begin(), note_and_dur.end(), ':');

        if (num_delim == 2) {
          int first_delim = note_and_dur.find(delimiter);
          std::string midi_note = note_and_dur.substr(0, first_delim);

          int second_delim = note_and_dur.find(delimiter, first_delim + 1);
          std::string velo = note_and_dur.substr(
              first_delim + 1, second_delim - first_delim - 1);

          std::string dura = note_and_dur.substr(second_delim + 1);

          midi_event ev{.event_type = MIDI_ON,
                        .data1 = std::stoi(midi_note),
                        .data2 = std::stoi(velo),
                        .dur = std::stoi(dura),
                        .playback_tick = std::stoi(midi_tick)};
          events.push_back(ev);
        }
      }
    }

    return std::make_shared<object::MidiArray>(events);
  }

  return NULLL;
}

std::shared_ptr<object::Object> EvalIndexExpressionUpdate(
    std::shared_ptr<object::Object> left, std::shared_ptr<object::Object> index,
    std::shared_ptr<object::Object> new_value) {
  if (left->Type() == object::ARRAY_OBJ && index->Type() == object::NUMBER_OBJ)
    return EvalArrayIndexExpressionUpdate(left, index, new_value);
  else if (left->Type() == object::HASH_OBJ)
    return EvalHashIndexExpressionUpdate(left, index, new_value);
  else if (left->Type() == object::STRING_OBJ &&
           index->Type() == object::NUMBER_OBJ)
    return EvalStringIndexExpressionUpdate(left, index, new_value);

  return NewError("index UPDATE operation not supported: %s", left->Type());
}

std::shared_ptr<object::Object> EvalHashIndexExpressionUpdate(
    std::shared_ptr<object::Object> hash_obj,
    std::shared_ptr<object::Object> key,
    std::shared_ptr<object::Object> new_value) {
  std::shared_ptr<object::Hash> my_hash =
      std::dynamic_pointer_cast<object::Hash>(hash_obj);

  if (!IsHashable(key))
    return NewError("Unusable as hash key: %s", key->Type());

  object::HashKey hashed = MakeHashKey(key);
  auto hpair_it = my_hash->pairs_.find(hashed);
  if (hpair_it != my_hash->pairs_.end())
    hpair_it->second.value_ = new_value;
  else {
    if (!IsHashable(key))
      return NewError("unusable as hash key: %s", key->Type());

    object::HashKey hashed = MakeHashKey(key);

    auto new_pair = std::make_shared<object::HashPair>(key, new_value);

    my_hash->pairs_.insert(std::pair<object::HashKey, object::HashPair>(
        hashed, object::HashPair{key, new_value}));
  }

  return NULLL;
}

std::shared_ptr<object::Object> EvalArrayIndexExpressionUpdate(
    std::shared_ptr<object::Object> array_obj,
    std::shared_ptr<object::Object> index,
    std::shared_ptr<object::Object> new_value) {
  std::shared_ptr<object::Array> my_array =
      std::dynamic_pointer_cast<object::Array>(array_obj);

  std::shared_ptr<object::Number> int_obj =
      std::dynamic_pointer_cast<object::Number>(index);
  if (my_array && int_obj) {
    int idx = int_obj->value_;
    int num_elems = my_array->elements_.size();
    if (idx >= 0 && idx < num_elems) my_array->elements_[idx] = new_value;
    return NULLL;
  }
  return NewError("Couldn't unpack yer Array OBJ to UPDATE IT!");
}

std::shared_ptr<object::Object> EvalStringIndexExpressionUpdate(
    std::shared_ptr<object::Object> string_obj,
    std::shared_ptr<object::Object> index,
    std::shared_ptr<object::Object> new_value) {
  std::shared_ptr<object::String> my_string =
      std::dynamic_pointer_cast<object::String>(string_obj);

  std::shared_ptr<object::Number> int_obj =
      std::dynamic_pointer_cast<object::Number>(index);
  if (my_string && int_obj) {
    int idx = int_obj->value_;
    int str_len = my_string->value_.length();
    if (idx < str_len) {
      std::shared_ptr<object::String> my_new_string_index_value =
          std::dynamic_pointer_cast<object::String>(new_value);

      if (my_new_string_index_value) {
        std::string begin = "";
        if (idx > 0) {
          begin = my_string->value_.substr(0, idx);
        }
        std::string end = my_string->value_.substr(idx + 1);

        std::string updated_string =
            begin + my_new_string_index_value->value_ + end;

        my_string->value_ = updated_string;
      }

      return evaluator::NULLL;
    }
  }
  return NewError("Couldn't unpack yer STRING OBJ to UPDATE IT!");
}

std::shared_ptr<object::Object> EvalIndexExpression(
    std::shared_ptr<object::Object> left,
    std::shared_ptr<object::Object> index) {
  if (left->Type() == object::ARRAY_OBJ && index->Type() == object::NUMBER_OBJ)
    return EvalArrayIndexExpression(left, index);
  else if (left->Type() == object::HASH_OBJ)
    return EvalHashIndexExpression(left, index);
  else if (left->Type() == object::STRING_OBJ &&
           index->Type() == object::NUMBER_OBJ)
    return EvalStringIndexExpression(left, index);

  return NewError("index operation not supported: %s", left->Type());
}

std::shared_ptr<object::Object> EvalArrayIndexExpression(
    std::shared_ptr<object::Object> array_obj,
    std::shared_ptr<object::Object> index) {
  std::shared_ptr<object::Array> my_array =
      std::dynamic_pointer_cast<object::Array>(array_obj);

  std::shared_ptr<object::Number> int_obj =
      std::dynamic_pointer_cast<object::Number>(index);
  if (my_array && int_obj) {
    int idx = int_obj->value_;
    int num_elems = my_array->elements_.size();
    if (idx >= 0 && idx < num_elems)
      return my_array->elements_[idx];
    else
      return NULLL;
  }
  return NewError("Couldn't unpack yer Array OBJ!");
}

std::shared_ptr<object::Object> EvalStringIndexExpression(
    std::shared_ptr<object::Object> string_obj,
    std::shared_ptr<object::Object> index) {
  std::shared_ptr<object::String> my_string =
      std::dynamic_pointer_cast<object::String>(string_obj);

  std::shared_ptr<object::Number> int_obj =
      std::dynamic_pointer_cast<object::Number>(index);
  if (my_string && int_obj) {
    int idx = int_obj->value_;
    int str_len = my_string->value_.length();
    if (idx < str_len) {
      return std::make_shared<object::String>(
          std::string(1, my_string->value_[idx]));
    }
  }

  return evaluator::NULLL;
}

std::shared_ptr<object::Object> EvalHashIndexExpression(
    std::shared_ptr<object::Object> hash_obj,
    std::shared_ptr<object::Object> key) {
  std::shared_ptr<object::Hash> my_hash =
      std::dynamic_pointer_cast<object::Hash>(hash_obj);

  if (!IsHashable(key))
    return NewError("Unusable as hash key: %s", key->Type());

  object::HashKey hashed = MakeHashKey(key);
  auto hpair_it = my_hash->pairs_.find(hashed);
  if (hpair_it != my_hash->pairs_.end()) return hpair_it->second.value_;

  return evaluator::NULLL;
}

std::shared_ptr<object::Object> EvalPrefixExpression(
    std::string op, std::shared_ptr<object::Object> right) {
  if (op.compare("!") == 0)
    return EvalBangOperatorExpression(right);
  else if (op.compare("~") == 0)
    return EvalNotOperatorExpression(right);
  else if (op.compare("-") == 0)
    return EvalMinusPrefixOperatorExpression(right);
  else if (op.compare("++") == 0)
    return EvalIncrementOperatorExpression(right);
  else if (op.compare("--") == 0)
    return EvalDecrementOperatorExpression(right);
  else
    return NewError("unknown operator: %s %s ", op, right->Type());
}

std::shared_ptr<object::Object> EvalForLoop(
    std::shared_ptr<object::ForLoop> for_loop) {
  auto initial_iterator_val = Eval(for_loop->iterator_value_, for_loop->env_);
  if (IsError(initial_iterator_val)) {
    std::cerr << "OOPS< ERR!\n";
    return initial_iterator_val;
  }
  auto initial_val =
      std::dynamic_pointer_cast<object::Number>(initial_iterator_val);
  if (!initial_val) {
    std::cerr << "DUH! Need a NUMBER for Iterator value!!\n";
    return evaluator::NULLL;
  }
  for_loop->env_->Set(for_loop->iterator_->value_, initial_val);

  std::shared_ptr<object::Object> result = evaluator::NULLL;
  while (IsTruthy(Eval(for_loop->termination_condition_, for_loop->env_))) {
    result = Eval(for_loop->body_, for_loop->env_);
    if (result->Type() == object::BREAK_OBJ) {
      break;
    }
    if (result->Type() == object::RETURN_VALUE_OBJ) {
      return result;
    }
    auto new_iterator_val = Eval(for_loop->increment_, for_loop->env_);
    if (IsError(new_iterator_val)) {
      std::cerr << "OOPS< ERR!\n";
      return new_iterator_val;
    }
    for_loop->env_->Set(for_loop->iterator_->value_, new_iterator_val);
  }

  return result;
}

std::shared_ptr<object::Object> EvalIfExpression(
    std::shared_ptr<ast::IfExpression> if_expr,
    std::shared_ptr<object::Environment> env) {
  auto condition = Eval(if_expr->condition_, env);
  if (IsError(condition)) return condition;

  if (IsTruthy(condition)) {
    return Eval(if_expr->consequence_, env);
  } else if (if_expr->alternative_) {
    return Eval(if_expr->alternative_, env);
  }

  return evaluator::NULLL;
}
std::shared_ptr<object::Object> EvalInfixExpression(
    std::string op, std::shared_ptr<object::Object> left,
    std::shared_ptr<object::Object> right) {
  if (left->Type() == object::NUMBER_OBJ &&
      right->Type() == object::NUMBER_OBJ) {
    auto leftie = std::dynamic_pointer_cast<object::Number>(left);
    auto rightie = std::dynamic_pointer_cast<object::Number>(right);
    return EvalNumberInfixExpression(op, leftie, rightie);
  } else if (left->Type() == object::BOOLEAN_OBJ &&
             right->Type() == object::NUMBER_OBJ) {
    auto leftie = std::dynamic_pointer_cast<object::Boolean>(left);
    auto rightie = std::dynamic_pointer_cast<object::Number>(right);
    // TODO - implement for other infix operators, not just '-'
    return NativeBoolToBooleanObject(leftie->value_ - rightie->value_);
  } else if (left->Type() == object::NUMBER_OBJ &&
             right->Type() == object::BOOLEAN_OBJ) {
    auto leftie = std::dynamic_pointer_cast<object::Number>(left);
    auto rightie = std::dynamic_pointer_cast<object::Boolean>(right);
    // TODO - implement for other infix operators, not just '-'
    return NativeBoolToBooleanObject(leftie->value_ - rightie->value_);
  } else if (left->Type() == object::STRING_OBJ &&
             right->Type() == object::STRING_OBJ) {
    auto leftie = std::dynamic_pointer_cast<object::String>(left);
    auto rightie = std::dynamic_pointer_cast<object::String>(right);
    return EvalStringInfixExpression(op, leftie, rightie);
  } else if (left->Type() == object::STRING_OBJ &&
             right->Type() == object::NUMBER_OBJ) {
    auto leftie = std::dynamic_pointer_cast<object::String>(left);
    auto rightie = std::dynamic_pointer_cast<object::Number>(right);
    return EvalStringInfixExpression(op, leftie, NumberToString(rightie));
  } else if (left->Type() == object::ARRAY_OBJ &&
             right->Type() == object::ARRAY_OBJ) {
    auto leftie = std::dynamic_pointer_cast<object::Array>(left);
    auto rightie = std::dynamic_pointer_cast<object::Array>(right);
    return EvalArrayInfixExpression(op, leftie, rightie);
  } else if (left->Type() == object::ARRAY_OBJ &&
             right->Type() == object::NUMBER_OBJ) {
    auto leftie_array = std::dynamic_pointer_cast<object::Array>(left);
    auto rightie_num = std::dynamic_pointer_cast<object::Number>(right);
    return EvalMultiplyArrayExpression(op, leftie_array, rightie_num);
  } else if (left->Type() == object::NUMBER_OBJ &&
             right->Type() == object::ARRAY_OBJ) {
    auto leftie_array = std::dynamic_pointer_cast<object::Array>(right);
    auto rightie_num = std::dynamic_pointer_cast<object::Number>(left);
    return EvalMultiplyArrayExpression(op, leftie_array, rightie_num);
  } else if (left->Type() == object::NUMBER_OBJ &&
             right->Type() == object::STRING_OBJ) {
    auto leftie = std::dynamic_pointer_cast<object::Number>(left);
    auto rightie = std::dynamic_pointer_cast<object::String>(right);
    return EvalStringInfixExpression(op, NumberToString(leftie), rightie);
  } else if (op.compare("==") == 0)
    return NativeBoolToBooleanObject(left == right);
  else if (op.compare("!=") == 0)
    return NativeBoolToBooleanObject(left != right);
  else if (op.compare("&&") == 0 || op.compare("||") == 0) {
    if (left->Type() == object::BOOLEAN_OBJ &&
        right->Type() == object::BOOLEAN_OBJ) {
      auto leftie = std::dynamic_pointer_cast<object::Boolean>(left);
      auto rightie = std::dynamic_pointer_cast<object::Boolean>(right);

      if (op.compare("&&") == 0) {
        return NativeBoolToBooleanObject(leftie->value_ && rightie->value_);
      } else {
        return NativeBoolToBooleanObject(leftie->value_ || rightie->value_);
      }
    }
  } else if (left->Type() != right->Type()) {
    return NewError("type mismatch: %s %s %s", left->Type(), op, right->Type());
  }

  return NewError("unknown operator: %s %s %s", left->Type(), op,
                  right->Type());
}

std::shared_ptr<object::Object> EvalNumberInfixExpression(
    std::string op, std::shared_ptr<object::Number> left,
    std::shared_ptr<object::Number> right) {
  if (op.compare("+") == 0)
    return std::make_shared<object::Number>(left->value_ + right->value_);
  else if (op.compare("-") == 0)
    return std::make_shared<object::Number>(left->value_ - right->value_);
  else if (op.compare("*") == 0)
    return std::make_shared<object::Number>(left->value_ * right->value_);
  else if (op.compare("/") == 0) {
    auto val = left->value_ / right->value_;
    if (std::isinf(val)) {
      val = 0;
    }
    return std::make_shared<object::Number>(val);
  } else if (op.compare("%") == 0)
    return std::make_shared<object::Number>(fmod(left->value_, right->value_));
  else if (op.compare("&") == 0)
    return std::make_shared<object::Number>(int(left->value_) &
                                            int(right->value_));
  else if (op.compare("|") == 0)
    return std::make_shared<object::Number>(int(left->value_) |
                                            int(right->value_));
  else if (op.compare("^") == 0)
    return std::make_shared<object::Number>(int(left->value_) ^
                                            int(right->value_));
  else if (op.compare("<<") == 0) {
    auto val = int(left->value_) << int(right->value_);
    if (std::isinf(val)) {
      val = 0;
    }
    return std::make_shared<object::Number>(val);
  } else if (op.compare(">>") == 0) {
    auto val = int(left->value_) >> int(right->value_);
    if (std::isinf(val)) {
      val = 0;
    }
    return std::make_shared<object::Number>(val);
  } else if (op.compare("<") == 0)
    return NativeBoolToBooleanObject(left->value_ < right->value_);
  else if (op.compare("<=") == 0)
    return NativeBoolToBooleanObject(left->value_ <= right->value_);
  else if (op.compare(">") == 0)
    return NativeBoolToBooleanObject(left->value_ > right->value_);
  else if (op.compare(">=") == 0)
    return NativeBoolToBooleanObject(left->value_ >= right->value_);
  else if (op.compare("==") == 0)
    return NativeBoolToBooleanObject(left->value_ == right->value_);
  else if (op.compare("!=") == 0)
    return NativeBoolToBooleanObject(left->value_ != right->value_);

  return NewError("unknown operator: %s %s %s", left->Type(), op,
                  right->Type());
}

std::shared_ptr<object::Object> EvalStringInfixExpression(
    std::string op, std::shared_ptr<object::String> left,
    std::shared_ptr<object::String> right) {
  if (op.compare("+") == 0)
    return std::make_shared<object::String>(left->value_ + right->value_);
  else if (op.compare("==") == 0)
    return NativeBoolToBooleanObject(left->value_ == right->value_);
  else if (op.compare("!=") == 0)
    return NativeBoolToBooleanObject(left->value_ != right->value_);

  return NewError("unknown operator: %s %s %s", left->Type(), op,
                  right->Type());
}

std::shared_ptr<object::Object> EvalArrayInfixExpression(
    std::string op, std::shared_ptr<object::Array> left,
    std::shared_ptr<object::Array> right) {
  if (op.compare("+") == 0) {
    auto return_vec(left->elements_);
    return_vec.insert(return_vec.end(), right->elements_.begin(),
                      right->elements_.end());
    return std::make_shared<object::Array>(return_vec);
  }
  return NewError("unknown operator: %s %s %s", left->Type(), op,
                  right->Type());
}

std::shared_ptr<object::Object> EvalMultiplyArrayExpression(
    std::string op, std::shared_ptr<object::Array> left,
    std::shared_ptr<object::Number> right) {
  if (op.compare("*") == 0) {
    auto return_vec(left->elements_);
    for (int i = 1; i < right->value_; i++) {
      return_vec.insert(return_vec.end(), return_vec.begin(), return_vec.end());
    }
    return std::make_shared<object::Array>(return_vec);
  }
  return NewError("unknown operator: %s %s %s", left->Type(), op,
                  right->Type());
}

std::shared_ptr<object::Object> EvalBangOperatorExpression(
    std::shared_ptr<object::Object> right) {
  if (right == TTRUE) {
    return FFALSE;
  } else if (right == FFALSE) {
    return TTRUE;
  } else if (right == NULLL) {
    return TTRUE;
  } else {
    auto bool_obj = std::dynamic_pointer_cast<object::Boolean>(right);
    if (bool_obj) {
      if (bool_obj->value_)
        return FFALSE;
      else
        return TTRUE;
    }
    return FFALSE;
  }
}

std::shared_ptr<object::Object> EvalNotOperatorExpression(
    std::shared_ptr<object::Object> right) {
  std::shared_ptr<object::Number> i =
      std::dynamic_pointer_cast<object::Number>(right);
  if (!i) {
    return NewError("unknown operator: ~%s", right->Type());
  }
  return std::make_shared<object::Number>(~int(i->value_));
}

std::shared_ptr<object::Object> EvalMinusPrefixOperatorExpression(
    std::shared_ptr<object::Object> right) {
  std::shared_ptr<object::Number> i =
      std::dynamic_pointer_cast<object::Number>(right);
  if (!i) {
    return NewError("unknown operator: -%s", right->Type());
  }

  return std::make_shared<object::Number>(-i->value_);
}

std::shared_ptr<object::Object> EvalIncrementOperatorExpression(
    std::shared_ptr<object::Object> right) {
  std::shared_ptr<object::Number> i =
      std::dynamic_pointer_cast<object::Number>(right);
  if (!i) {
    return NewError("unknown operator: ++%s", right->Type());
  }

  return std::make_shared<object::Number>(++(i->value_));
}

std::shared_ptr<object::Object> EvalDecrementOperatorExpression(
    std::shared_ptr<object::Object> right) {
  std::shared_ptr<object::Number> i =
      std::dynamic_pointer_cast<object::Number>(right);
  if (!i) {
    return NewError("unknown operator: --%s", right->Type());
  }

  return std::make_shared<object::Number>(--(i->value_));
}

std::shared_ptr<object::Object> EvalProgram(
    std::vector<std::shared_ptr<ast::Statement>> const &stmts,
    std::shared_ptr<object::Environment> env) {
  std::shared_ptr<object::Object> result;
  for (auto &s : stmts) {
    result = Eval(s, env);

    std::shared_ptr<object::ReturnValue> r =
        std::dynamic_pointer_cast<object::ReturnValue>(result);
    if (r) return r->value_;

    std::shared_ptr<object::Error> e =
        std::dynamic_pointer_cast<object::Error>(result);
    if (e) return e;
  }

  return result;
}

std::shared_ptr<object::Object> EvalBlockStatement(
    std::shared_ptr<ast::BlockStatement> block,
    std::shared_ptr<object::Environment> env) {
  std::shared_ptr<object::Object> result = evaluator::NULLL;
  for (auto &s : block->statements_) {
    result = Eval(s, env);
    if (result != evaluator::NULLL) {
      if (result->Type() == object::RETURN_VALUE_OBJ ||
          result->Type() == object::ERROR_OBJ)
        return result;
    }
  }
  return result;
}

std::shared_ptr<object::Boolean> NativeBoolToBooleanObject(bool input) {
  if (input) return TTRUE;
  return FFALSE;
}

bool ObjectToNativeBool(std::shared_ptr<object::Object> obj) {
  auto boolobj = std::dynamic_pointer_cast<object::Boolean>(obj);
  if (boolobj) return boolobj->value_;
  auto numobj = std::dynamic_pointer_cast<object::Number>(obj);
  if (numobj) return numobj->value_;
}

std::shared_ptr<object::Object> EvalIdentifier(
    std::shared_ptr<ast::Identifier> ident,
    std::shared_ptr<object::Environment> env) {
  auto val = env->Get(ident->value_);
  if (val) return val;

  auto builtin = builtin::built_ins[ident->value_];
  if (builtin) return builtin;

  return NewError("identifier not found: %s", ident->value_);
}

std::shared_ptr<object::Object> EvalHashLiteral(
    std::shared_ptr<ast::HashLiteral> hash_literal,
    std::shared_ptr<object::Environment> env) {
  std::map<object::HashKey, object::HashPair> pairs;
  for (auto const &it : hash_literal->pairs_) {
    std::shared_ptr<object::Object> hashkey = Eval(it.first, env);
    if (IsError(hashkey)) return hashkey;

    if (!IsHashable(hashkey))
      return NewError("unusable as hash key: %s", hashkey->Type());
    object::HashKey hashed = MakeHashKey(hashkey);

    std::shared_ptr<object::Object> val = Eval(it.second, env);
    if (IsError(val)) return val;

    pairs.insert(std::pair<object::HashKey, object::HashPair>(
        hashed, object::HashPair{hashkey, val}));
  }

  return std::make_shared<object::Hash>(pairs);
}

std::vector<std::shared_ptr<object::Object>> EvalExpressions(
    std::vector<std::shared_ptr<ast::Expression>> exps,
    std::shared_ptr<object::Environment> env) {
  std::vector<std::shared_ptr<object::Object>> result;

  for (auto const &e : exps) {
    auto evaluated = Eval(e, env);
    if (IsError(evaluated))
      return std::vector<std::shared_ptr<object::Object>>{evaluated};

    result.push_back(evaluated);
  }

  return result;
}

std::shared_ptr<object::Object> ApplyGeneratorRun(
    std::shared_ptr<object::Object> callable) {
  std::shared_ptr<object::Generator> gen =
      std::dynamic_pointer_cast<object::Generator>(callable);
  if (gen) {
    // func->env_ = ExtendFunctionEnv(func, args);
    auto evaluated = Eval(gen->run_, gen->env_);
    return UnwrapReturnValue(evaluated);
  }

  return NewError("Something stinky wit yer GENERaTOR , mate!");
}

std::shared_ptr<object::Object> ApplyGeneratorSignalGenerator(
    std::shared_ptr<object::Object> callable) {
  std::shared_ptr<object::Generator> gen =
      std::dynamic_pointer_cast<object::Generator>(callable);
  if (gen) {
    // func->env_ = ExtendFunctionEnv(func, args);
    if (gen->signal_generator_) {
      auto evaluated = Eval(gen->signal_generator_, gen->env_);
      return UnwrapReturnValue(evaluated);
    }
  }

  return NewError("Something stinky wit yer GENERaTOR , mate!");
}

std::shared_ptr<object::Object> ApplyFunction(
    std::shared_ptr<object::Object> callable,
    std::vector<std::shared_ptr<object::Object>> args) {
  std::shared_ptr<object::Function> func =
      std::dynamic_pointer_cast<object::Function>(callable);
  if (func) {
    auto extended_env = ExtendFunctionEnv(func, args);
    auto evaluated = Eval(func->body_, extended_env);
    return UnwrapReturnValue(evaluated);
  }

  std::shared_ptr<object::BuiltIn> builtin =
      std::dynamic_pointer_cast<object::BuiltIn>(callable);
  if (builtin) {
    return builtin->func_(args);
  }

  return NewError("Something stinky wit yer functions, mate!");
}

std::shared_ptr<object::Environment> ExtendFunctionEnv(
    std::shared_ptr<object::Function> fun,
    std::vector<std::shared_ptr<object::Object>> const &args) {
  std::shared_ptr<object::Environment> new_env =
      std::make_shared<object::Environment>(fun->env_);
  if (fun->parameters_.size() != args.size()) {
    std::cerr << "Function Eval - args and params not same size, ya "
                 "numpty!\n";
    return new_env;
  }

  int args_len = args.size();
  for (int i = 0; i < args_len; i++) {
    auto param = fun->parameters_[i];
    new_env->Set(param->value_, args[i]);
  }
  return new_env;
}

std::shared_ptr<object::Object> UnwrapReturnValue(
    std::shared_ptr<object::Object> obj) {
  std::shared_ptr<object::ReturnValue> ret =
      std::dynamic_pointer_cast<object::ReturnValue>(obj);
  if (ret) return ret->value_;
  return obj;
}

std::shared_ptr<PatternFunction> EvalPatternFunctionExpression(
    std::shared_ptr<ast::Expression> funct) {
  auto func = std::dynamic_pointer_cast<ast::PatternFunctionExpression>(funct);

  if (!func) {
    std::cerr << "DIDNAE CAST YER FUNC\n";
    return nullptr;
  }

  if (func->token_.literal_ == "arp") {
    auto arp_func = std::make_shared<PatternArp>();
    for (auto a : func->arguments_) {
      auto speedliteral = std::dynamic_pointer_cast<ast::NumberLiteral>(a);
      if (speedliteral) {
        int speedval = speedliteral->value_;

        if (speedval == 16)
          arp_func->speed_ = ArpSpeed::ARP_16;
        else if (speedval == 8)
          arp_func->speed_ = ArpSpeed::ARP_8;
        else if (speedval == 4)
          arp_func->speed_ = ArpSpeed::ARP_4;
      }
      auto directionliteral = std::dynamic_pointer_cast<ast ::Identifier>(a);
      if (directionliteral) {
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
  } else if (func->token_.literal_ == "brak") {
    return std::make_shared<PatternBrak>();
  } else if (func->token_.literal_ == "bump") {
    auto bump_func = std::make_shared<PatternBump>();
    if (func->arguments_.size() == 1) {
      auto bump_literal =
          std::dynamic_pointer_cast<ast::NumberLiteral>(func->arguments_[0]);
      if (bump_literal) {
        if (bump_literal->value_ >= 0 && bump_literal->value_ < PPSIXTEENTH)
          bump_func->bump_amount_ = bump_literal->value_;
      }
    }
    return bump_func;
  } else if (func->token_.literal_ == "chord") {
    return std::make_shared<PatternChord>();
  } else if (func->token_.literal_ == "every") {
    auto intval =
        std::dynamic_pointer_cast<ast::NumberLiteral>(func->arguments_[0]);
    if (!intval) {
      std::cerr << "NAE NAE!\n";
      return nullptr;
    }

    auto func_arg_ast = std::make_shared<ast::PatternFunctionExpression>(
        func->arguments_[1]->token_);

    if (func_arg_ast) {
      int args_size = func->arguments_.size();
      if (args_size > 2) {
        for (int i = 2; i < args_size; i++)
          func_arg_ast->arguments_.push_back(func->arguments_[i]);
      }
      auto func_arg = EvalPatternFunctionExpression(func_arg_ast);
      if (func_arg) {
        auto p_every = std::make_shared<PatternEvery>(intval->value_, func_arg);
        return p_every;
      }
    }
  } else if (func->token_.literal_ == "mask") {
    if (func->arguments_.size() == 1) {
      auto mask_string =
          std::dynamic_pointer_cast<ast::StringLiteral>(func->arguments_[0]);
      if (mask_string) {
        auto mask = mask_string->value_;
        if (IsValidHex(mask)) {
          return std::make_shared<PatternMask>(mask);
        }
      }
    }
  } else if (func->token_.literal_ == "power") {
    return std::make_shared<PatternPowerChord>();
  } else if (func->token_.literal_ == "rev") {
    return std::make_shared<PatternReverse>();
  } else if (func->token_.literal_ == "rotl" ||
             func->token_.literal_ == "rotr") {
    int num_sixteenth_steps = 1;
    if (func->arguments_.size() > 0) {
      auto intval =
          std::dynamic_pointer_cast<ast::NumberLiteral>(func->arguments_[0]);
      if (intval) num_sixteenth_steps = intval->value_;
    }

    if (func->token_.literal_ == "rotl")
      return std::make_shared<PatternRotate>(LEFT, num_sixteenth_steps);
    else
      return std::make_shared<PatternRotate>(RIGHT, num_sixteenth_steps);
  } else if (func->token_.literal_ == "scramble") {
    return std::make_shared<PatternScramble>();
  } else if (func->token_.literal_ == "speed") {
    if (func->arguments_.size() == 1) {
      auto multi =
          std::dynamic_pointer_cast<ast::NumberLiteral>(func->arguments_[0]);
      if (multi) {
        double speed_multi = multi->value_;
        return std::make_shared<PatternSpeed>(speed_multi);
      }
    }
    std::cerr << "Need a speed adjustment multiplier.." << std::endl;
  } else if (func->token_.literal_ == "swing") {
    int swing_setting = 50;
    if (func->arguments_.size() > 0) {
      auto intval =
          std::dynamic_pointer_cast<ast::NumberLiteral>(func->arguments_[0]);
      if (intval) swing_setting = intval->value_;
    }
    return std::make_shared<PatternSwing>(swing_setting);
  } else if (func->token_.literal_ == "up" || func->token_.literal_ == "down") {
    int num_octaves = 1;
    if (func->arguments_.size() > 0) {
      auto intval =
          std::dynamic_pointer_cast<ast::NumberLiteral>(func->arguments_[0]);
      if (intval) num_octaves = intval->value_;
    }
    if (func->token_.literal_ == "up")
      return std::make_shared<PatternTranspose>(UP, num_octaves);
    else
      return std::make_shared<PatternTranspose>(DOWN, num_octaves);
  } else if (func->token_.literal_ == "while") {
    std::cout << "WHILE!!\n";
  } else {
    std::cout << "NAH MAN, DIDN't GET YER FUNCTION 0- i got " << func->String()
              << std::endl;
  }
  return nullptr;
}

std::shared_ptr<object::Object> EvalProcessStatement(
    std::shared_ptr<ast::ProcessStatement> proc,
    std::shared_ptr<object::Environment> env) {
  event_queue_item ev;

  ev.pattern_expression = proc->pattern_expression_;

  auto eval_val = Eval(proc->loop_len_, env);
  if (eval_val->Type() == "ERROR") {
    std::cerr << "COuldn't EVAL your statement value!!\n";
    return NULLL;
  }
  auto loop_len = std::dynamic_pointer_cast<object::Number>(eval_val);
  if (loop_len) {
    ev.loop_len = loop_len->value_;
  } else {
    ev.loop_len = 1;
  }

  std::vector<std::shared_ptr<PatternFunction>> process_funcz;

  for (auto &f : proc->functions_) {
    auto funcy = EvalPatternFunctionExpression(f);
    if (funcy) process_funcz.push_back(funcy);
  }

  ev.type = Event::PROCESS_UPDATE_EVENT;
  ev.target_process_id = proc->mixer_process_id_;
  ev.process_name = proc->name;
  ev.process_type = proc->process_type_;
  ev.timer_type = proc->process_timer_type_;
  ev.command = proc->command_;
  ev.target_type = proc->target_type_;
  ev.targets = proc->targets_;
  ev.funcz = process_funcz;

  process_event_queue.push(ev);

  return NULLL;
}
std::shared_ptr<object::Object> EvalProcessSetStatement(
    std::shared_ptr<ast::ProcessSetStatement> proc_set,
    std::shared_ptr<object::Environment> env) {
  event_queue_item ev;
  ev.type = Event::PROCESS_SET_PARAM_EVENT;
  ev.target_process_id = proc_set->mixer_process_id_;

  auto eval_val = Eval(proc_set->value_, env);
  if (eval_val->Type() == "ERROR") {
    std::cerr << "COuldn't EVAL your statement value!!\n";
    return NULLL;
  }
  auto num = std::dynamic_pointer_cast<object::Number>(eval_val);
  if (num) {
    ev.loop_len = num->value_;
    process_event_queue.push(ev);
  }

  return NULLL;
}

//////////// Error shizzle below

void SSprintF(std::ostringstream &msg, const char *s);
void SSprintF(std::ostringstream &msg, const char *s) {
  while (*s) {
    if (*s == '%') {
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
              Args... args) {
  while (*format) {
    if (*format == '%') {
      if (*(format + 1) != '%') {
        msg << value;

        format += 2;  // only work on 2 char format strings
        SSprintF(msg, format, args...);
        return;
      }
      ++format;
    }
    msg << *format++;
  }
}

template <typename... Args>
std::shared_ptr<object::Error> NewError(std::string format, Args... args) {
  std::ostringstream error_msg;

  SSprintF(error_msg, format.c_str(), std::forward<Args>(args)...);

  return std::make_shared<object::Error>(error_msg.str());
}

}  // namespace evaluator
