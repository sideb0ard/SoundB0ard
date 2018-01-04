#include <iostream>

#define LINK_PLATFORM_MACOSX 1

#include <ableton/Link.hpp>
#include <ableton/link/HostTimeFilter.hpp>
#include <ableton/platforms/stl/Clock.hpp>

#include "ableton_link_wrapper.h"
#include "defjams.h"
#include "mixer.h"

extern mixer *mixr;

const auto MICROS_PER_SAMPLE = 1e6 / (double) SAMPLE_RATE;

struct AbletonLink
{
    ableton::Link m_link;
    ableton::Link::Timeline m_timeline; // should be updated in every callback
    ableton::link::HostTimeFilter<ableton::platforms::stl::Clock>
        m_host_time_filter;

    std::chrono::microseconds m_hosttime; // also updated every callback

    double m_quantum;
    double m_requested_tempo;
    bool m_reset_beat_time;
    bool m_have_started;

    int m_sample_time;
    int m_sample_time_last; // TEMP
    int m_sample_at_last_loop;
    int m_sample_at_last_quarter;
    int m_sample_at_last_midi_tick;
    int m_sample_at_last_sixteenth;

    double m_midi_ticks_per_ms;
    double m_samples_per_midi_tick;
    double m_loop_len_in_samples;
    double m_loop_len_in_ticks;

    std::chrono::microseconds m_output_latency;

    AbletonLink(double bpm)
        : m_link{bpm},
          m_timeline{m_link.captureAudioTimeline()},
          m_quantum{4.},
          m_requested_tempo{0},
          m_reset_beat_time{false},
          m_have_started{false},
          m_sample_time{0},
          m_sample_time_last{0}, // TMP
          m_sample_at_last_loop{0},
          m_sample_at_last_quarter{0},
          m_sample_at_last_midi_tick{0},
          m_sample_at_last_sixteenth{0},
          m_midi_ticks_per_ms{PPQN / ((60.0 / bpm) * 1000)},
          m_samples_per_midi_tick{(60.0 / bpm * SAMPLE_RATE) / PPQN},
          m_loop_len_in_samples{(60.0 / bpm * SAMPLE_RATE) * m_quantum},
          m_loop_len_in_ticks{PPBAR},
          m_output_latency(0)
    {
        m_link.setTempoCallback(update_bpm);
        m_link.enable(true);
    }
};

void update_bpm(double bpm)
{
    printf("Changing bpm to %.2f\n", bpm);
}

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
    const auto host_time = l->m_host_time_filter.sampleTimeToHostTime(l->m_sample_time);
    l->m_hosttime = host_time + l->m_output_latency; // save it for use in other functions during the audio callback
    l->m_sample_time += num_frames;

    l->m_timeline = l->m_link.captureAudioTimeline();

    if (l->m_reset_beat_time) {
        l->m_timeline.requestBeatAtTime(0, host_time, l->m_quantum);
        l->m_reset_beat_time = false;
    }

    if (l->m_requested_tempo > 0) {
        l->m_timeline.setTempo(l->m_requested_tempo, host_time);
        l->m_requested_tempo = 0;
    }

    l->m_link.commitAudioTimeline(l->m_timeline);
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// called every sample
link_callback_timing_data link_get_callback_timing_data(AbletonLink *l, int sample_number)
{
    link_callback_timing_data data;
    data.is_midi_tick = false;
    data.is_start_of_loop = false;
    data.is_start_of_quarter = false;
    data.is_start_of_sixteenth = false;
    data.sx_tick = 0;

    const auto this_sample_time = l->m_hosttime + std::chrono::microseconds(llround(sample_number * MICROS_PER_SAMPLE));
    const auto last_sample_time = this_sample_time - std::chrono::microseconds(llround(MICROS_PER_SAMPLE));

    if (l->m_timeline.phaseAtTime(this_sample_time, l->m_quantum) < l->m_timeline.phaseAtTime(last_sample_time, l->m_quantum))
    {
        const auto beat = l->m_timeline.beatAtTime(this_sample_time, l->m_quantum);
        printf("LOOPY! Beat: %f  --  %f(%d) %f(%d) // diff is %d looplen in samples is %f\n",
                beat,
                l->m_timeline.phaseAtTime(this_sample_time, l->m_quantum),
                l->m_sample_time,
                l->m_timeline.phaseAtTime(last_sample_time, l->m_quantum),
                l->m_sample_at_last_loop,
                l->m_sample_time - l->m_sample_at_last_loop,
                l->m_loop_len_in_samples
        );
        l->m_sample_at_last_loop = l->m_sample_time;
    }

    return data;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void link_set_bpm(AbletonLink *l, double bpm)
{
    l->m_requested_tempo = bpm;
}

void link_reset_beat_time(AbletonLink *l)
{
    l->m_reset_beat_time = true;
}

double link_get_bpm(AbletonLink *l)
{
    auto timeline = l->m_link.captureAppTimeline();
    return timeline.tempo();
}

double link_get_beat_at_time(AbletonLink *l, int sample_number)
{
    const auto host_time = l->m_hosttime + std::chrono::microseconds(llround(sample_number * MICROS_PER_SAMPLE));
    return l->m_timeline.beatAtTime(host_time, l->m_quantum);
}

int link_get_sample_time(AbletonLink *l) { return l->m_sample_time; }

void link_inc_sample_time(AbletonLink *l) { l->m_sample_time++; }

void link_set_old_sample_time(AbletonLink *l)
{
    l->m_sample_time_last = l->m_sample_time;
}
int link_get_old_sample_time(AbletonLink *l)
{
    return l->m_sample_time_last;
}

int link_get_samples_per_midi_tick(AbletonLink *l)
{
    return l->m_samples_per_midi_tick;
}

int link_get_loop_len_in_samples(AbletonLink *l)
{
    return l->m_loop_len_in_samples;
}

} // extern "C"
