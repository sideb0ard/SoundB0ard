#include <cfloat>

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

#include <audiofile_data.h>
#include <defjams.h>
#include <midi_freq_table.h>
#include <pitch_detection.hpp>

constexpr double ktimeslice = 0.1;

namespace
{
std::vector<std::vector<float>> GetChunks(std::vector<float> container,
                                          size_t k)
{
    std::vector<std::vector<float>> ret;

    size_t size = container.size();
    size_t i = 0;

    if (size > k)
    {
        for (; i < size - k; i += k)
        {
            ret.push_back(std::vector<float>(container.begin() + i,
                                             container.begin() + i + k));
        }
    }

    if (i % k)
    {
        ret.push_back(std::vector<float>(container.begin() + i,
                                         container.begin() + i + (i % k)));
    }

    return ret;
}

std::vector<int> PeakPicking(const std::vector<float> &nsdf)
{
    std::vector<int> max_positions{};
    int pos = 0;
    int cur_max_pos = 0;
    ssize_t size = nsdf.size();

    while (pos < (size - 1) / 3 && nsdf[pos] > 0)
        pos++;
    while (pos < size - 1 && nsdf[pos] <= 0.0)
        pos++;

    if (pos == 0)
        pos = 1;

    while (pos < size - 1)
    {
        if (nsdf[pos] > nsdf[pos - 1] && nsdf[pos] >= nsdf[pos + 1] &&
            (cur_max_pos == 0 || nsdf[pos] > nsdf[cur_max_pos]))
        {
            cur_max_pos = pos;
        }
        pos++;
        if (pos < size - 1 && nsdf[pos] <= 0)
        {
            if (cur_max_pos > 0)
            {
                max_positions.push_back(cur_max_pos);
                // std::cout << "PUSHING BACK " << cur_max_pos << std::endl;
                cur_max_pos = 0;
            }
            while (pos < size - 1 && nsdf[pos] <= 0.0)
            {
                pos++;
            }
        }
    }
    if (cur_max_pos > 0)
    {
        max_positions.push_back(cur_max_pos);
    }
    return max_positions;
}

std::pair<float, float> ParabolicInterpolation(const std::vector<float> &array,
                                               int x_)
{
    int x_adjusted;
    float x = (float)x_;

    if (x < 1)
    {
        x_adjusted = (array[x] <= array[x + 1]) ? x : x + 1;
    }
    else if (x > signed(array.size()) - 1)
    {
        x_adjusted = (array[x] <= array[x - 1]) ? x : x - 1;
    }
    else
    {
        float den = array[x + 1] + array[x - 1] - 2 * array[x];
        float delta = array[x - 1] - array[x + 1];
        return (!den) ? std::make_pair(x, array[x])
                      : std::make_pair(x + delta / (2 * den),
                                       array[x] - delta * delta / (8 * den));
    }
    return std::make_pair(x_adjusted, array[x_adjusted]);
}

float GetClosestFrequence(float freq_estimate)
{

    float real_freq = -1;
    // TODO - magic number bad, mmmkay.
    for (int i = 0; i < 128 - 1; i++)
    {
        if (freq_estimate >= midi_freq_table[i] &&
            freq_estimate < midi_freq_table[i + 1])
        {
            if (abs(midi_freq_table[i] - freq_estimate) <
                abs(midi_freq_table[i + 1] - freq_estimate))
                real_freq = midi_freq_table[i];
            else
                real_freq = midi_freq_table[i + 1];
        }
    }

    return real_freq;
}

} //  namespace

MPMPitchDetector::MPMPitchDetector(long audio_buffer_size)
    : N_(audio_buffer_size), out_im_(std::vector<std::complex<float>>(N_ * 2)),
      out_real_(std::vector<float>(N_)),
      pitch_bins_(std::vector<float>(N_BINS)),
      real_pitches_(std::vector<float>(N_BINS))
{
    if (N_ == 0)
    {
        throw std::bad_alloc();
    }

    fft_forward_ = ffts_init_1d(N_ * 2, FFTS_FORWARD);
    fft_backward_ = ffts_init_1d(N_ * 2, FFTS_BACKWARD);
    InitPitchBins();
    hmm_ = BuildHmm();
}

MPMPitchDetector::~MPMPitchDetector()
{
    ffts_free(fft_forward_);
    ffts_free(fft_backward_);
}

void MPMPitchDetector::InitPitchBins()
{
    for (int i = 0; i < N_BINS; ++i)
    {
        float fi = F0 * std::pow(Apitch, i - NOTE_OFFSET);
        pitch_bins_[i] = fi;
    }
}

void MPMPitchDetector::AutoCorrelate(const std::vector<float> &audio_buffer)
{
    if (audio_buffer.size() == 0)
        throw std::invalid_argument("audio_buffer shouldn't be empty");

    std::transform(audio_buffer.begin(), audio_buffer.begin() + N_,
                   out_im_.begin(), [](float x) -> std::complex<float> {
                       return std::complex(x, static_cast<float>(0.0));
                   });

    ffts_execute(fft_forward_, out_im_.data(), out_im_.data());

    std::complex<float> scale = {1.0f / (float)(N_ * 2),
                                 static_cast<float>(0.0)};
    for (int i = 0; i < N_; ++i)
        out_im_[i] *= std::conj(out_im_[i]) * scale;

    ffts_execute(fft_backward_, out_im_.data(), out_im_.data());

    std::transform(
        out_im_.begin(), out_im_.begin() + N_, out_real_.begin(),
        [](std::complex<float> cplx) -> float { return std::real(cplx); });
}

float MPMPitchDetector::GetPitch(const std::vector<float> &audio_buffer)
{
    AutoCorrelate(audio_buffer);

    std::vector<int> max_positions = PeakPicking(out_real_);
    std::vector<std::pair<float, float>> estimates;

    float highest_amplitude = -DBL_MAX;

    for (int i : max_positions)
    {
        highest_amplitude = std::max(highest_amplitude, out_real_[i]);
        if (out_real_[i] > MPM_SMALL_CUTOFF)
        {
            auto x = ParabolicInterpolation(out_real_, i);
            estimates.push_back(x);
            highest_amplitude = std::max(highest_amplitude, std::get<1>(x));
        }
    }

    if (estimates.empty())
        return -1;

    float actual_cutoff = MPM_CUTOFF * highest_amplitude;
    float period = 0;

    for (auto i : estimates)
    {
        if (std::get<1>(i) >= actual_cutoff)
        {
            period = std::get<0>(i);
            break;
        }
    }

    float pitch_estimate = (SAMPLE_RATE / period);

    Clear();

    return (pitch_estimate > MPM_LOWER_PITCH_CUTOFF) ? pitch_estimate : -1;
}

mlpack::hmm::HMM<mlpack::distribution::DiscreteDistribution> BuildHmm()
{
    size_t hmm_size = 2 * N_BINS + 1;
    // initial
    arma::vec initial(hmm_size);
    initial.fill(1.0 / double(hmm_size));

    arma::mat transition(hmm_size, hmm_size, arma::fill::zeros);

    // transitions
    for (int i = 0; i < N_BINS; ++i)
    {
        int half_transition = static_cast<int>(TRANSITION_WIDTH / 2.0);
        int theoretical_min_next_pitch = i - half_transition;
        int min_next_pitch = i > half_transition ? i - half_transition : 0;
        int max_next_pitch =
            i < N_BINS - half_transition ? i + half_transition : N_BINS - 1;

        double weight_sum = 0.0;
        std::vector<double> weights;

        for (int j = min_next_pitch; j <= max_next_pitch; ++j)
        {
            if (j <= i)
            {
                weights.push_back(j - theoretical_min_next_pitch + 1);
            }
            else
            {
                weights.push_back(i - theoretical_min_next_pitch + 1 - j + i);
            }
            weight_sum += weights[weights.size() - 1];
        }

        for (int j = min_next_pitch; j <= max_next_pitch; ++j)
        {
            transition(i, j) =
                (weights[j - min_next_pitch] / weight_sum * SELF_TRANS);
            transition(i, j + N_BINS) =
                (weights[j - min_next_pitch] / weight_sum * (1.0 - SELF_TRANS));
            transition(i + N_BINS, j + N_BINS) =
                (weights[j - min_next_pitch] / weight_sum * SELF_TRANS);
            transition(i + N_BINS, j) =
                (weights[j - min_next_pitch] / weight_sum * (1.0 - SELF_TRANS));
        }
    }

    // the only valid emissions are exact notes
    // i.e. an identity matrix of emissions
    std::vector<mlpack::distribution::DiscreteDistribution> emissions(hmm_size);

    for (size_t i = 0; i < hmm_size; ++i)
    {
        emissions[i] = mlpack::distribution::DiscreteDistribution(
            std::vector<arma::vec>{arma::vec(hmm_size, arma::fill::zeros)});
        emissions[i].Probabilities()[i] = 1.0;
    }

    auto hmm = mlpack::hmm::HMM(initial, transition, emissions);
    return hmm;
}

///////////////////////////////////////////
//
std::string DetectPitch(std::string sample_path)
{
    std::stringstream ss;
    ss << "Pitch of " << sample_path << " is - FAKED!";

    audiofile_data afd;
    audiofile_data_import_file_contents(&afd, sample_path);

    std::vector<float> audio;

    if (afd.channels == 2)
    {
        // convert stereo to mono
        for (int i = 0; i < afd.samplecount; i += 2)
            audio.push_back(afd.filecontents[i]);
    }
    else
    {
        for (int i = 0; i < afd.samplecount; i++)
            audio.push_back(afd.filecontents[i]);
    }
    // i have no idea why i'm multiplying this by 10,000!
    // this code is pinched from
    // https://github.com/sevagh/pitch-detection/blob/master/wav_analyzer/wav_analyzer.cpp
    // (pinched sounds so much more polite than stolen!)
    // without doing it, my pitch calculations were off by a large factor, so
    // it's necessary for the computation at some stage.
    std::transform(
        audio.begin(), audio.end(), audio.begin(),
        std::bind(std::multiplies<float>(), std::placeholders::_1, 10000));

    auto chunk_size = size_t(SAMPLE_RATE * ktimeslice);
    auto chunks = GetChunks(audio, chunk_size);

    double t = 0.;

    std::vector<float> pitches;
    for (auto chunk : chunks)
    {
        MPMPitchDetector mpmd(chunk.size());
        auto pitch_mpm = mpmd.GetPitch(chunk);
        t += ktimeslice;
        if (pitch_mpm != -1)
            pitches.push_back(pitch_mpm);
    }
    for (auto p : pitches)
        std::cout << "PITCHES ARE:" << p << " normD:" << GetClosestFrequence(p)
                  << std::endl;

    return ss.str();
}

