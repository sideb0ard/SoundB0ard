#include "object.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace object
{

std::string Integer::Inspect()
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

ObjectType Integer::Type() { return INTEGER_OBJ; }
object::HashKey Integer::HashKey()
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

std::shared_ptr<Object> Environment::Set(std::string key,
                                         std::shared_ptr<Object> val)
{
    store_[key] = val;
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
