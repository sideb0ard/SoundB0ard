#include <string>
#include <vector>

#include <interpreter/object.hpp>
#include <soundgenerator.h>

enum AudioAction
{
    NO_ACTION,
    ADD,
    UPDATE,
    STATUS,
    NOTE_ON,
};

struct audio_action_queue_item
{
    AudioAction type;
    int mixer_soundgen_idx{0};

    // ADD varz
    std::shared_ptr<SoundGenerator> sg{nullptr};

    // UPDATE varz
    int fx_id{0};
    std::string param_name{};
    double param_val{0};

    // NOTE_ON varz
    std::vector<std::shared_ptr<object::Object>> args;
};
