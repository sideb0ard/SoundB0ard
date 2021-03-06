#include <iostream>

#include <audioutils.h>
#include <fx/dynamics_processor.h>
#include <interpreter/sound_cmds.hpp>
#include <mixer.h>

extern Mixer *mixr;

namespace interpreter_sound_cmds
{

void ParseFXCmd(std::vector<std::shared_ptr<object::Object>> &args)
{
    auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
    if (soundgen)
    {
        if (mixr->IsValidSoundgenNum(soundgen->soundgen_id_))
        {
            auto sg = mixr->sound_generators_[soundgen->soundgen_id_];

            std::shared_ptr<object::String> str_obj =
                std::dynamic_pointer_cast<object::String>(args[1]);
            if (str_obj)
            {
                if (str_obj->value_ == "bitcrush")
                    sg->AddBitcrush();
                else if (str_obj->value_ == "compressor")
                    sg->AddCompressor();
                else if (str_obj->value_ == "delay")
                    sg->AddDelay(200);
                else if (str_obj->value_ == "distort")
                    sg->AddDistortion();
                else if (str_obj->value_ == "filter")
                    sg->AddBasicfilter();
                else if (str_obj->value_ == "reverb")
                    sg->AddReverb();
                else if (str_obj->value_ == "sidechain")
                {
                    if (args.size() > 2)
                    {
                        std::cout << "Got a source!\n";
                        auto soundgen_sidechain_src =
                            std::dynamic_pointer_cast<object::SoundGenerator>(
                                args[2]);
                        if (soundgen_sidechain_src &&
                            mixr->IsValidSoundgenNum(
                                soundgen_sidechain_src->soundgen_id_))
                        {
                            int fx_num = sg->AddCompressor();
                            DynamicsProcessor *dp =
                                (DynamicsProcessor *)sg->effects[fx_num];
                            dp->SetExternalSource(
                                soundgen_sidechain_src->soundgen_id_);
                            dp->SetDefaultSidechainParams();
                        }
                    }
                }
                else if (str_obj->value_ == "moddelay")
                    sg->AddModdelay();
                else if (str_obj->value_ == "modfilter")
                    sg->AddModfilter();
                else if (str_obj->value_ == "waveshape")
                    sg->AddWaveshape();
            }
        }
    }
}

void ParseSynthCmd(std::vector<std::shared_ptr<object::Object>> &args)
{
    assert(args.size() == 3);

    auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
    if (soundgen)
    {
        if (mixr->IsValidSoundgenNum(soundgen->soundgen_id_))
        {
            auto sg = mixr->sound_generators_[soundgen->soundgen_id_];
            std::shared_ptr<object::String> str_obj =
                std::dynamic_pointer_cast<object::String>(args[1]);
            if (str_obj)
            {
                std::shared_ptr<object::String> str_cmd =
                    std::dynamic_pointer_cast<object::String>(args[2]);
                if (str_cmd->value_ == "load")
                    sg->Load(str_obj->value_);
                else if (str_cmd->value_ == "save")
                    sg->Save(str_obj->value_);
            }
        }
    }
}

std::vector<std::vector<std::string>> GenerateMelody()
{

    std::vector<std::vector<std::string>> melody;
    // TODO - tests - i think this is brittle, especially getting this key_str
    std::string key_str = GetNoteFromMidiNum(mixr->timing_info.key);
    for (int i = 0; i < mixr->prog_len; i++)
    {
        std::vector<std::string> chord;

        int root_note = GetNthDegree(mixr->timing_info.key,
                                     mixr->prog_degrees[i], key_str[0]);
        chord.push_back(GetNoteFromMidiNum(root_note));

        int third = GetThird(root_note, key_str[0]);
        chord.push_back(GetNoteFromMidiNum(third));

        int fifth = GetFifth(root_note, key_str[0]);
        chord.push_back(GetNoteFromMidiNum(fifth));

        melody.push_back(chord);
    }

    return melody;
}

std::vector<int> GetNotesInCurrentKey()
{
    std::vector<int> notes;

    mixer_timing_info my_info = mixr->timing_info;

    notes.push_back(my_info.key + 0);
    notes.push_back(my_info.key + 2);
    notes.push_back(my_info.key + 4);
    notes.push_back(my_info.key + 5);
    notes.push_back(my_info.key + 7);
    notes.push_back(my_info.key + 9);
    notes.push_back(my_info.key + 11);

    return notes;
}

std::vector<int> GetNotesInCurrentChord()
{
    std::vector<std::string> notes;

    mixer_timing_info my_info = mixr->timing_info;

    auto root_note = GetRootNote(my_info.key, my_info.chord);

    auto notes_in_chord =
        GetMidiNotesInChord(root_note, get_chord_type(my_info.chord));

    // for (auto n : notes_in_chord)
    // notes.push_back(GetNoteFromMidiNum(n));

    return notes_in_chord;
}

} // namespace interpreter_sound_cmds
