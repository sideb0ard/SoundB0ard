#include <iostream>

#define LINK_PLATFORM_MACOSX 1

#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#include <ableton/platforms/stl/Clock.hpp>

#include "ableton_link_wrapper.h"
#include "mixer.h"

extern mixer *mixr;

void bump_bpm(double bpm)
{
    printf("BUMPY!\n");
    mixr->bpm = bpm;
}

struct AbletonLink {
    ableton::Link m_link;
    std::atomic<bool> m_running;

    double m_requested_tempo;
    bool m_reset_beat_time;
    bool m_is_playing;
    double m_quantum;
    std::chrono::microseconds m_output_latency;
    int m_sample_time;

    ableton::link::HostTimeFilter<ableton::platforms::stl::Clock>
        m_host_time_filter;

    AbletonLink(double bpm)
        : m_running(true), m_link(bpm), m_requested_tempo{bpm},
          m_reset_beat_time{false}, m_is_playing{false}, m_quantum{4.}
    {
        m_link.setTempoCallback(bump_bpm);
        m_link.enable(true);
    }
};

// c++ wrapper for Ableton Link to call from sbsh
extern "C" {
AbletonLink *new_ableton_link(double bpm)
{
    std::cout << "New Ableton Link object!" << std::endl;
    return new AbletonLink(bpm);
}

LinkData link_get_timing_data(AbletonLink *l)
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

void link_update_from_main_callback(AbletonLink *l)
{
    const auto host_time =
        l->m_host_time_filter.sampleTimeToHostTime(l->m_sample_time);
    l->m_sample_time++;

    const auto buffer_begin_at_output = host_time + l->m_output_latency;

    auto timeline = l->m_link.captureAudioTimeline();

    if (l->m_reset_beat_time) {
        timeline.requestBeatAtTime(0, host_time, l->m_quantum);
        l->m_reset_beat_time = false;
    }

    if (l->m_requested_tempo > 0) {
        timeline.setTempo(l->m_requested_tempo, host_time);
        l->m_requested_tempo = 0;
    }

    l->m_link.commitAudioTimeline(timeline);
}

void link_set_bpm(AbletonLink *l, double bpm)
{
    // TODO - do i need locks here?
    l->m_requested_tempo = bpm;
    // auto timeline = l->link.captureAppTimeline();
}
}
