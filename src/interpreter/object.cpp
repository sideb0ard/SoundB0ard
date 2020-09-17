#include <interpreter/object.hpp>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <mixer.h>
#include <process.hpp>

extern mixer *mixr;

namespace
{
bool IsSoundGenerator(object::ObjectType type)
{
    if (type == object::SYNTH_OBJ || type == object::SAMPLE_OBJ ||
        type == object::GRANULAR_OBJ)
        return true;
    return false;
}
} // namespace

namespace object
{

std::string Number::Inspect()
{
    std::stringstream val;
    val << value_;
    return val.str();
}

bool operator<(HashKey const &lhs, HashKey const &rhs)
{
    if (lhs.Type() < rhs.Type())
        return true;
    else if (lhs.Type() > rhs.Type())
        return false;
    else // same type - compare values
    {

        if (lhs.Value() < rhs.Value())
            return true;
        else if (lhs.Value() > rhs.Value())
            return false;
        else // value_ is equal
            return false;
    }
}

ObjectType Number::Type() { return NUMBER_OBJ; }
object::HashKey Number::HashKey()
{
    return object::HashKey(Type(), (uint64_t)value_);
}

std::string Boolean::Inspect()
{
    std::stringstream val;
    val << (value_ ? "true" : "false");
    return val.str();
}
ObjectType Boolean::Type() { return BOOLEAN_OBJ; }
object::HashKey Boolean::HashKey()
{
    uint64_t val = 0;
    if (value_)
        val = 1;

    return object::HashKey(Type(), (uint64_t)val);
}

object::HashKey String::HashKey()
{
    std::hash<std::string> hasher;
    return object::HashKey(Type(), (uint64_t)hasher(value_));
}

std::string Null::Inspect() { return "null"; }
ObjectType Null::Type() { return NULL_OBJ; }

FMSynth::FMSynth() { soundgen_id_ = add_dxsynth(mixr); }
std::string FMSynth::Inspect() { return "FM synth."; }
ObjectType FMSynth::Type() { return SYNTH_OBJ; }

MoogSynth::MoogSynth() { soundgen_id_ = add_minisynth(mixr); }
std::string MoogSynth::Inspect() { return "Moog synth."; }
ObjectType MoogSynth::Type() { return SYNTH_OBJ; }

DrumSynth::DrumSynth() { soundgen_id_ = add_drumsynth(mixr); }
std::string DrumSynth::Inspect() { return "Drum synth."; }
ObjectType DrumSynth::Type() { return SYNTH_OBJ; }

DigiSynth::DigiSynth(std::string sample_path)
{
    std::cout << "SKELP ME - PATH:" << sample_path << std::endl;
    soundgen_id_ = add_digisynth(mixr, sample_path);
}
std::string DigiSynth::Inspect() { return "Digi synth."; }
ObjectType DigiSynth::Type() { return SYNTH_OBJ; }

Sample::Sample(std::string sample_path)
{
    soundgen_id_ = add_sample(mixr, sample_path);
}
std::string Sample::Inspect() { return "sample."; }
ObjectType Sample::Type() { return SAMPLE_OBJ; }

Granular::Granular(std::string sample_path, bool loop_mode)
{
    std::cout << "OBJECT! LOOPM ODE IS " << loop_mode << std::endl;
    soundgen_id_ = add_looper(mixr, sample_path, loop_mode);
}
std::string Granular::Inspect() { return "Granular."; }
ObjectType Granular::Type() { return GRANULAR_OBJ; }

std::string ReturnValue::Inspect() { return value_->Inspect(); }
ObjectType ReturnValue::Type() { return RETURN_VALUE_OBJ; }

Error::Error(std::string err_msg) : message_{err_msg} {}
std::string Error::Inspect() { return "ERROR: " + message_; }
ObjectType Error::Type() { return ERROR_OBJ; }

ObjectType Function::Type() { return FUNCTION_OBJ; }
std::string Function::Inspect()
{
    std::stringstream params;
    int len = parameters_.size();
    int i = 0;
    for (auto &p : parameters_)
    {
        params << p->String();
        if (i < len - 1)
            params << ", ";
        i++;
    }
    std::stringstream return_val;
    return_val << "fn(" << params.str() << ") {\n";
    return_val << body_->String() << "\n)";

    return return_val.str();
}

ObjectType Generator::Type() { return GENERATOR_OBJ; }
std::string Generator::Inspect()
{
    std::stringstream params;
    int len = parameters_.size();
    int i = 0;
    for (auto &p : parameters_)
    {
        params << p->String();
        if (i < len - 1)
            params << ", ";
        i++;
    }
    std::stringstream return_val;
    return_val << "gn(" << params.str() << ") \n";
    return_val << "setup() { " << setup_->String() << "\n}\n";
    return_val << "run() { " << run_->String() << "\n}\n";

    return return_val.str();
}

ObjectType ForLoop::Type() { return FORLOOP_OBJ; }
std::string ForLoop::Inspect() { return "FOR LOOP"; }

std::string Array::Inspect()
{
    std::stringstream elems;
    int len = elements_.size();
    int i = 0;
    for (auto &e : elements_)
    {
        elems << e->Inspect();
        if (i < len - 1)
            elems << ", ";
        i++;
    }
    std::stringstream return_val;
    return_val << "[" << elems.str() << "]";

    return return_val.str();
}

std::string Environment::Debug()
{
    std::stringstream ss;
    for (const auto &it : store_)
    {
        if (!IsSoundGenerator(it.second->Type()))
            ss << "Key: " << it.first << " // Val:" << it.second->Inspect()
               << std::endl;
    }
    return ss.str();
}

std::unordered_map<std::string, int> Environment::GetSoundGenerators()
{
    std::unordered_map<std::string, int> soundgens;
    for (const auto &it : store_)
    {
        if (IsSoundGenerator(it.second->Type()))
        {
            auto gen =
                std::dynamic_pointer_cast<object::SoundGenerator>(it.second);
            if (gen)
            {
                soundgens.insert({it.first, gen->soundgen_id_});
            }
        }
    }
    return soundgens;
}

std::shared_ptr<Object> Environment::Get(std::string name)
{
    auto entry = store_.find(name);
    if (entry == store_.end())
    {
        if (outer_env_)
            return outer_env_->Get(name);
        return nullptr;
    }
    return entry->second;
}

std::shared_ptr<Object>
Environment::Set(std::string key, std::shared_ptr<Object> val, bool create)
{
    if (create)
    {
        store_[key] = val;
    }
    else
    {
        auto entry = store_.find(key);
        if (entry != store_.end())
        {
            store_[key] = val;
        }
        else
        {
            if (outer_env_)
                return outer_env_->Set(key, val, create);
            std::cerr << key << " not found.\n";
            return nullptr;
        }
    }
    return val;
}

std::string Hash::Inspect()
{
    std::stringstream out;
    std::vector<std::string> pairs;
    for (auto const &it : pairs_)
    {
        pairs.push_back(it.second.key_->Inspect() + ": " +
                        it.second.value_->Inspect());
    }

    int pairs_size = pairs.size();
    out << "{";
    for (int i = 0; i < pairs_size; i++)
    {
        out << pairs[i];
        if (i != pairs_size - 1)
            out << ", ";
    }
    out << "}";

    return out.str();
}

} // namespace object
