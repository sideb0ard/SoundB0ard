#include <iostream>

#define LINK_PLATFORM_MACOSX 1

#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#include <ableton/platforms/stl/Clock.hpp>

#include <chrono>

#include "ableton_link_wrapper.h"
#include "defjams.h"
#include "mixer.h"

extern mixer *mixr;

const auto MICROS_PER_SAMPLE = 1e6 / (double)SAMPLE_RATE;
const auto MIDI_TICK_FRAC_OF_BEAT = 1. / 960;

struct AbletonLink
{
    ableton::Link m_link;
    ableton::Link::Timeline m_timeline; // should be updated in every callback
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
        : m_link{bpm}, m_timeline{m_link.captureAudioTimeline()}, m_quantum{4.},
          m_requested_tempo{0}, m_reset_beat_time{false}, m_sample_time{0},
          m_output_latency(0)
    {
        // m_link.setTempoCallback(update_bpm);
        m_link.enable(true);
    }
};

void update_bpm(double bpm) { printf("Changing bpm to %.2f\n", bpm); }

// c++ wrapper for Ableton Link to call from sbsh
extern "C" {

AbletonLink *new_ableton_link(double bpm)
{
    std::cout << "New Ableton Link object!" << std::endl;
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

    l->m_timeline = l->m_link.captureAudioTimeline();

    if (l->m_reset_beat_time)
    {
        l->m_timeline.requestBeatAtTime(0, host_time, l->m_quantum);
        l->m_reset_beat_time = false;
    }

    if (l->m_requested_tempo > 0)
    {
        l->m_timeline.setTempo(l->m_requested_tempo, host_time);
        l->m_requested_tempo = 0;
    }

    l->m_link.commitAudioTimeline(l->m_timeline);
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
    const auto lastSampleHostTime =
        hostTime - std::chrono::microseconds(llround(MICROS_PER_SAMPLE));

    auto beat_time = l->m_timeline.beatAtTime(hostTime, l->m_quantum);
    if (beat_time >= 0.)
    {
        if (beat_time > timing_info->time_of_next_midi_tick)
            link_inc_midi(l, timing_info, beat_time);
    }

    return timing_info->is_midi_tick;
}

LinkData link_get_timing_data_for_display(AbletonLink *l)
{
    auto timeline = l->m_link.captureAppTimeline();
    const auto time = l->m_link.clock().micros();

    LinkData data;
    data.num_peers = l->m_link.numPeers();
    data.quantum = l->m_quantum;
    data.tempo = timeline.tempo();
    data.beat = timeline.beatAtTime(time, l->m_quantum);
    data.phase = timeline.phaseAtTime(time, l->m_quantum);

    return data;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void link_set_bpm(AbletonLink *l, double bpm) { l->m_requested_tempo = bpm; }

void link_reset_beat_time(AbletonLink *l) { l->m_reset_beat_time = true; }

double link_get_bpm(AbletonLink *l)
{
    auto timeline = l->m_link.captureAppTimeline();
    return timeline.tempo();
}

double link_get_beat_at_time(AbletonLink *l, long long int sample_number)
{
    const auto host_time = std::chrono::microseconds(llround(sample_number));
    return l->m_timeline.beatAtTime(host_time, l->m_quantum);
}

double link_get_phase_at_time(AbletonLink *l, long long int sample_number,
                              int quantum)
{
    // const auto host_time = l->m_hosttime +
    // std::chrono::microseconds(llround(sample_number * MICROS_PER_SAMPLE));
    const auto host_time = std::chrono::microseconds(llround(sample_number));
    return l->m_timeline.phaseAtTime(host_time, quantum);
}

int link_get_sample_time(AbletonLink *l) { return l->m_sample_time; }

} // extern "C"
