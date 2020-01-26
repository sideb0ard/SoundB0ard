#include <iostream>

#include <fx/dynamics_processor.h>
#include <interpreter/sound_cmds.hpp>
#include <mixer.h>

extern mixer *mixr;

namespace interpreter_sound_cmds
{

void ParseFXCmd(std::vector<std::shared_ptr<object::Object>> &args)
{
    auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
    if (soundgen)
    {
        if (mixer_is_valid_soundgen_num(mixr, soundgen->soundgen_id_))
        {
            SoundGenerator *sg = mixr->SoundGenerators[soundgen->soundgen_id_];

            std::shared_ptr<object::String> str_obj =
                std::dynamic_pointer_cast<object::String>(args[1]);
            if (str_obj)
            {
                std::cout << "Adding a " << str_obj->value_ << std::endl;

                if (str_obj->value_ == "bitcrush")
                    add_bitcrush_soundgen(sg);
                else if (str_obj->value_ == "compressor")
                    add_compressor_soundgen(sg);
                else if (str_obj->value_ == "delay")
                    add_delay_soundgen(sg, 200);
                else if (str_obj->value_ == "distort")
                    add_distortion_soundgen(sg);
                else if (str_obj->value_ == "reverb")
                    add_reverb_soundgen(sg);
                else if (str_obj->value_ == "sidechain")
                {
                    if (args.size() > 2)
                    {
                        std::cout << "Got a source!\n";
                        auto soundgen_sidechain_src =
                            std::dynamic_pointer_cast<object::SoundGenerator>(
                                args[2]);
                        if (soundgen_sidechain_src &&
                            mixer_is_valid_soundgen_num(
                                mixr, soundgen_sidechain_src->soundgen_id_))
                        {
                            int fx_num = add_compressor_soundgen(sg);
                            DynamicsProcessor *dp =
                                (DynamicsProcessor *)sg->effects[fx_num];
                            dp->SetExternalSource(
                                soundgen_sidechain_src->soundgen_id_);
                            dp->SetDefaultSidechainParams();
                        }
                    }
                }
                else if (str_obj->value_ == "moddelay")
                    add_moddelay_soundgen(sg);
                else if (str_obj->value_ == "modfilter")
                    add_modfilter_soundgen(sg);
                else if (str_obj->value_ == "waveshape")
                    add_waveshape_soundgen(sg);
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
        if (mixer_is_valid_soundgen_num(mixr, soundgen->soundgen_id_))
        {
            SoundGenerator *sg = mixr->SoundGenerators[soundgen->soundgen_id_];
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

} // namespace interpreter_sound_cmds
