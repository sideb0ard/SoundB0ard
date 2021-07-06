#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <midi_freq_table.h>
#include <pitch_detection.hpp>
#include <sample_oscillator.h>
#include <utils.h>

SampleOscillator::SampleOscillator(std::string filename)
{
    Oscillator::Reset();

    audiofile_data_import_file_contents(&afd, filename);
    is_single_cycle = false;
    is_pitchless = false;
    loop_mode = SAMPLE_ONESHOT;
    // TODO - this duplicates the reading of the file data
    orig_pitch_midi_ = DetectMidiPitch(filename);
}

void SampleOscillator::StartOscillator()
{
    Reset();
    m_note_on = true;
}
void SampleOscillator::StopOscillator() { m_note_on = false; }

void SampleOscillator::Reset()
{
    Oscillator::Reset();
    m_read_idx = 0;
    Update();
}

double SampleOscillator::ReadSampleBuffer()
{
    double return_val = 0;

    int read_idx = m_read_idx;
    double frac = m_read_idx - read_idx;

    if (m_read_idx > afd.samplecount)
        printf("OSC IDX TOO BIG %f\n", m_read_idx);

    if (afd.channels == 1)
    {
        int next_read_idx =
            read_idx + 1 > afd.samplecount - 1 ? 0 : read_idx + 1;

        return_val = lin_terp(0, 1, afd.filecontents[read_idx],
                              afd.filecontents[next_read_idx], frac);

        m_read_idx += m_inc;
    }

    else if (afd.channels == 2)
    {
        int read_idx_left = (int)m_read_idx * 2;
        int next_read_idx_left =
            read_idx_left + 2 > afd.samplecount - 1 ? 0 : read_idx_left + 2;
        double left_sample =
            lin_terp(0, 1, afd.filecontents[read_idx_left],
                     afd.filecontents[next_read_idx_left], frac);

        int read_idx_right = read_idx_left + 1;
        int next_read_idx_right =
            read_idx_right + 2 > afd.samplecount - 1 ? 1 : read_idx_right + 2;
        double right_sample =
            lin_terp(0, 1, afd.filecontents[read_idx_right],
                     afd.filecontents[next_read_idx_right], frac);

        return_val = (left_sample + right_sample) / 2;
        m_read_idx += m_inc;
    }

    return return_val;
}

void SampleOscillator::Update()
{
    Oscillator::Update();

    if (is_pitchless)
    {
        m_inc = 1;
        return;
    }

    double unity_freq =
        is_single_cycle
            ? (SAMPLE_RATE / ((float)afd.samplecount / (float)afd.channels))
            : get_midi_freq(orig_pitch_midi_);

    double length = SAMPLE_RATE / unity_freq;

    m_inc *= length;

    // if (m_read_idx >= afd.samplecount)
    //{
    //    m_read_idx -= afd.samplecount;
    //}
}

double SampleOscillator::DoOscillate(double *quad_phase_output)
{

    if (quad_phase_output)
        *quad_phase_output = 0.0;

    if (!m_note_on)
        return 0.0;

    if (m_read_idx < 0)
        return 0.0;

    double left_output = ReadSampleBuffer();
    // double right_output;

    // check for wrap
    if (loop_mode == SAMPLE_ONESHOT)
    {
        if (m_read_idx >
            (double)(afd.samplecount - afd.channels - 1) / afd.channels)
        {
            m_read_idx = -1;
        }
    }
    else
    {
        if (m_read_idx >
            (double)(afd.samplecount - afd.channels - 1) / afd.channels)
        {
            printf("BEYOND! %f\n", m_read_idx);
            m_read_idx = 0;
        }
    }

    return left_output;
}
