#include <iostream>

#define LINK_PLATFORM_MACOSX 1

#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#include <ableton/platforms/stl/Clock.hpp>

#include <chrono>

#include "ableton_link_wrapper.h"
#include "mixer.h"

extern Mixer *mixr;

const auto MICROS_PER_SAMPLE = 1e6 / (double)SAMPLE_RATE;
const auto MIDI_TICK_FRAC_OF_BEAT = 1. / 960;

struct AbletonLink
{
    ableton::Link m_link;
    ableton::Link::SessionState
        m_sessionstate; // should be updated in every callback
    ableton::link::HostTimeFilter<ableton::platforms::stl::Clock>
        m_host_time_filter;

    std::chrono::microseconds m_hosttime; // also updated every callback
    std::chrono::microseconds m_buffer_begin_at_output;

    double m_quantum;
    double m_requested_tempo;
    bool m_reset_beat_time;

    int m_sample_time;

    std::chrono::microseconds m_output_latency;

    AbletonLink(double bpm)
        : m_link{bpm},
          m_sessionstate{m_link.captureAudioSessionState()}, m_quantum{4.},
          m_requested_tempo{0}, m_reset_beat_time{false}, m_sample_time{0},
          m_output_latency(0)
    {
        // m_link.setTempoCallback(update_bpm);
        m_link.enable(true);
    }
};

void update_bpm(double bpm){/* no-op */};

AbletonLink *new_ableton_link(double bpm)
{
    // std::cout << "New Ableton Link object!" << std::endl;
    return new AbletonLink(bpm);
}

void link_set_latency(AbletonLink *l, double latency)
{
    l->m_output_latency = std::chrono::microseconds(llround(latency * 1.0e6));
}

void link_update_from_main_callback(AbletonLink *l, int num_frames)
{
    const auto host_time =
        l->m_host_time_filter.sampleTimeToHostTime(l->m_sample_time);
    l->m_buffer_begin_at_output =
        host_time + l->m_output_latency; // save it for use in other functions
                                         // during the audio callback

    l->m_sample_time += num_frames;

    l->m_sessionstate = l->m_link.captureAudioSessionState();

    if (l->m_reset_beat_time)
    {
        l->m_sessionstate.requestBeatAtTime(0, host_time, l->m_quantum);
        l->m_reset_beat_time = false;
    }

    if (l->m_requested_tempo > 0)
    {
        l->m_sessionstate.setTempo(l->m_requested_tempo, host_time);
        l->m_requested_tempo = 0;
    }

    l->m_link.commitAudioSessionState(l->m_sessionstate);
}

inline void link_inc_midi(AbletonLink *l, mixer_timing_info *timing_info,
                          double beat_at_time)
{
    timing_info->time_of_next_midi_tick =
        (double)((int)beat_at_time) +
        ((timing_info->midi_tick % PPQN) * MIDI_TICK_FRAC_OF_BEAT);
    timing_info->midi_tick++;
    timing_info->is_midi_tick = true;
}

bool link_is_midi_tick(AbletonLink *l, mixer_timing_info *timing_info,
                       int frame_num)
{
    timing_info->is_midi_tick = false;

    const auto hostTime =
        l->m_buffer_begin_at_output +
        std::chrono::microseconds(llround(frame_num * MICROS_PER_SAMPLE));

    auto beat_time = l->m_sessionstate.beatAtTime(hostTime, l->m_quantum);
    if (beat_time >= 0.)
    {
        if (beat_time > timing_info->time_of_next_midi_tick)
            link_inc_midi(l, timing_info, beat_time);
    }

    return timing_info->is_midi_tick;
}

LinkData link_get_timing_data_for_display(AbletonLink *l)
{
    auto sessionstate = l->m_link.captureAppSessionState();
    const auto time = l->m_link.clock().micros();

    LinkData data;
    data.num_peers = l->m_link.numPeers();
    data.quantum = l->m_quantum;
    data.tempo = sessionstate.tempo();
    data.beat = sessionstate.beatAtTime(time, l->m_quantum);
    data.phase = sessionstate.phaseAtTime(time, l->m_quantum);

    return data;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void link_set_bpm(AbletonLink *l, double bpm) { l->m_requested_tempo = bpm; }

void link_reset_beat_time(AbletonLink *l) { l->m_reset_beat_time = true; }

double link_get_bpm(AbletonLink *l)
{
    auto sessionstate = l->m_link.captureAppSessionState();
    return sessionstate.tempo();
}

double link_get_beat_at_time(AbletonLink *l, long long int sample_number)
{
    const auto host_time = std::chrono::microseconds(llround(sample_number));
    return l->m_sessionstate.beatAtTime(host_time, l->m_quantum);
}

double link_get_phase_at_time(AbletonLink *l, long long int sample_number,
                              int quantum)
{
    // const auto host_time = l->m_hosttime +
    // std::chrono::microseconds(llround(sample_number * MICROS_PER_SAMPLE));
    const auto host_time = std::chrono::microseconds(llround(sample_number));
    return l->m_sessionstate.phaseAtTime(host_time, quantum);
}

int link_get_sample_time(AbletonLink *l) { return l->m_sample_time; }
