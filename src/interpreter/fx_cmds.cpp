#include <iostream>

#include <dynamics_processor.h>
#include <interpreter/fx_cmds.hpp>
#include <mixer.h>

extern mixer *mixr;

namespace fxcmds
{

void ParseFXCmd(std::vector<std::shared_ptr<object::Object>> &args)
{
    std::cout << "PARSXXXXFX\n";

    auto soundgen = std::dynamic_pointer_cast<object::SoundGenerator>(args[0]);
    if (soundgen)
    {
        if (mixer_is_valid_soundgen_num(mixr, soundgen->soundgen_id_))
        {
            std::cout << "VALID SOUND GEN!\n";
            SoundGenerator *sg = mixr->SoundGenerators[soundgen->soundgen_id_];

            std::shared_ptr<object::String> str_obj =
                std::dynamic_pointer_cast<object::String>(args[1]);
            if (str_obj)
            {
                std::cout << "Adding a " << str_obj->value_ << std::endl;

                if (str_obj->value_ == "beatrepeat")
                    add_beatrepeat_soundgen(sg, 3, 12);
                else if (str_obj->value_ == "bitcrush")
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
                            dynamics_processor *dp =
                                (dynamics_processor *)sg->effects[fx_num];
                            dynamics_processor_set_external_source(
                                dp, soundgen_sidechain_src->soundgen_id_);
                            dynamics_processor_set_default_sidechain_params(dp);
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
} // namespace fxcmds
