#include "midi_freq_table.h"
#include <drum_synth.h>
#include <iostream>
#include <sstream>

DrumSynth::DrumSynth()
{
    osc1.m_waveform = SINE;
    // osc2.m_waveform = NOISE;

    mod_env.SetEgMode(ANALOG);
    mod_env.SetAttackTimeMsec(0);
    mod_env.SetDecayTimeMsec(3000);
    mod_env.SetSustainLevel(0);
    mod_env.SetReleaseTimeMsec(0);
    mod_env.ramp_mode = true;

    amp_env.SetEgMode(ANALOG);
    amp_env.SetAttackTimeMsec(0);
    amp_env.SetDecayTimeMsec(0);
    amp_env.SetSustainLevel(0);
    amp_env.SetReleaseTimeMsec(3000);
    amp_env.m_output_eg = true;
    amp_env.ramp_mode = true;
    amp_env.m_reset_to_zero = true;

    m_dca.m_mod_source_eg = DEST_DCA_EG;

    active = true;
}

stereo_val DrumSynth::genNext()
{

    stereo_val out = {.left = 0, .right = 0};
    if (!active)
        return out;

    if (osc1.m_note_on)
    {
        double accum_out_left = 0.0;
        double accum_out_right = 0.0;

        // Osc Pitch Modulation Envelope
        double mod_env_val = 0.0;
        mod_env.DoEnvelope(&mod_env_val);

        osc1.SetFoModExp(mod_env_int * OSC_FO_MOD_RANGE * mod_env_val);
        osc1.Update();
        double osc1_out = osc1.DoOscillate(nullptr);

        // Amp Envelope
        double eg_out = amp_env.DoEnvelope(nullptr);
        // std::cout << "AMP ENV:" << eg_out << std::endl;
        m_dca.SetEgMod(eg_out);
        m_dca.Update();

        double osc_mix = osc1_out;

        filter1.Update();
        double filter_out = filter1.DoFilter(osc_mix);

        double out_left = 0.0;
        double out_right = 0.0;

        m_dca.DoDCA(filter_out, filter_out, &out_left, &out_right);

        out = {.left = out_left, .right = out_right};
        out = Effector(out);
    }

    if (amp_env.GetState() == OFFF)
    {
        osc1.StopOscillator();
        amp_env.StopEg();
    }
    return out;
}

void DrumSynth::SetParam(std::string name, double val)
{

    if (name == "mod_env_attack")
        mod_env.SetAttackTimeMsec(val);
    if (name == "mod_env_decay")
        mod_env.SetDecayTimeMsec(val);
    if (name == "mod_env_sustain")
        mod_env.SetSustainLevel(val);
    if (name == "mod_env_release")
        mod_env.SetReleaseTimeMsec(val);
    if (name == "mod_env_int")
        mod_env_int = val;
    if (name == "amp_env_attack")
        amp_env.SetAttackTimeMsec(val);
    if (name == "amp_env_decay")
        amp_env.SetDecayTimeMsec(val);
    if (name == "amp_env_sustain")
        amp_env.SetSustainLevel(val);
    if (name == "amp_env_release")
        amp_env.SetReleaseTimeMsec(val);
    else if (name == "filter")
        filter1.SetType(val);
    else if (name == "fc")
        filter1.SetFcControl(val);
    else if (name == "fq")
        filter1.SetQControl(val);
}

std::string DrumSynth::Status()
{
    std::stringstream ss;
    if (!active || volume == 0)
        ss << ANSI_COLOR_RESET;
    else
        ss << ANSI_COLOR_CYAN;
    ss << "DrumSynth osc1:" << osc1.m_osc_fo
       << " filter:" << filter1.m_filter_type << " fc:" << filter1.m_fc
       << " fq:" << filter1.m_q << " mod_env_int:" << mod_env_int << std::endl;
    ss << "     mod_env_attack:" << mod_env.m_attack_time_msec
       << " mod_env_decay:" << mod_env.m_decay_time_msec
       << " mod_env_sustain:" << mod_env.m_sustain_level
       << " mod_env_release:" << mod_env.m_release_time_msec << std::endl;
    ss << "     amp_env_attack:" << amp_env.m_attack_time_msec
       << " amp_env_decay:" << amp_env.m_decay_time_msec
       << " amp_env_sustain:" << amp_env.m_sustain_level
       << " amp_env_release:" << amp_env.m_release_time_msec;

    return ss.str();
}

std::string DrumSynth::Info()
{
    std::stringstream ss;
    if (!active || volume == 0)
        ss << ANSI_COLOR_RESET;
    else
        ss << ANSI_COLOR_CYAN;
    ss << "Drumsynth~!";

    return ss.str();
}

double DrumSynth::GetParam(std::string name) { return 0; }

void DrumSynth::start()
{
    if (active)
        return; // no-op
    active = true;
}

void DrumSynth::noteOn(midi_event ev)
{

    unsigned int midinote = ev.data1;
    unsigned int velocity = ev.data2;

    osc1.m_note_on = true;
    osc1.m_osc_fo = get_midi_freq(midinote);
    osc1.StartOscillator();
    mod_env.StartEg();
    amp_env.StartEg();
}
