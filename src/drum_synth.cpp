#include "midi_freq_table.h"
#include <drum_synth.h>
#include <iostream>
#include <sstream>

DrumSynth::DrumSynth()
{
    osc1.m_waveform = SINE;
    osc2.m_waveform = NOISE;

    env1.SetEgMode(ANALOG);
    env1.SetAttackTimeMsec(1);
    env1.SetDecayTimeMsec(1);
    env1.m_output_eg = true;
    env1.ramp_mode = true;

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

        double eg_out = env1.DoEnvelope(nullptr);

        osc1.Update();
        double osc1_out = osc1.DoOscillate(nullptr);

        osc2.Update();
        double osc2_out = osc2.DoOscillate(nullptr);

        double osc_mix = osc1_out + osc2_out;

        filter1.Update();
        double filter_out = filter1.DoFilter(osc_mix);

        double out_left = 0.0;
        double out_right = 0.0;

        m_dca.DoDCA(filter_out, filter_out, &out_left, &out_right);

        out = {.left = out_left, .right = out_right};
        out = Effector(out);
    }

    if (env1.GetState() == OFFF)
    {
        osc1.StopOscillator();
        env1.StopEg();
    }
    return out;
}

void DrumSynth::SetParam(std::string name, double val)
{

    if (name == "e1attack")
        env1.SetAttackTimeMsec(val);
    else if (name == "e1decay")
        env1.SetDecayTimeMsec(val);
    else if (name == "e1release")
        env1.SetReleaseTimeMsec(val);
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
       << " e1attack:" << env1.m_attack_time_msec
       << " e1decay:" << env1.m_decay_time_msec
       << " e1release:" << env1.m_release_time_msec
       << " filter:" << filter1.m_filter_type << " fc:" << filter1.m_fc
       << " fq:" << filter1.m_q;

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
    env1.StartEg();
}
