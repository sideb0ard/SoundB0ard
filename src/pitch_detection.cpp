#include <audiofile_data.h>
#include <defjams.h>
#include <pitch_detection.hpp>

#include <iostream>
#include <sstream>
#include <vector>

constexpr double ktimeslice = 0.1;

namespace
{
std::vector<std::vector<float>> get_chunks(std::vector<float> container,
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

float mpm(std::vector<float> chunks)
{

    // https://miracle.otago.ac.nz/tartini/papers/Visualization_of_Musical_Pitch.pdf
    // https://miracle.otago.ac.nz/tartini/papers.html
    // 2. McLeod Pitch Method algo -
    //    a. select a sampling Window
    //    b. apply gaussian function to the window
    //    c. perform the FFT analysis
    //    d. identify the principal frequencies
    //    e. identify the fundamental as a sub-multiple of the frequency
    //    of greatest amplitude f. recognize the fundamental as a note of
    //    the musical scale
    //
    // implementation half-inched from
    // https://github.com/sevagh/pitch-detection/blob/master/src/mpm.cpp

    // util::acorr_r(audio_buffer, this);

    // std::vector<int> max_positions = peak_picking(this->out_real);
    // std::vector<std::pair<T, T>> estimates;

    // T highest_amplitude = -DBL_MAX;

    // for (int i : max_positions) {
    //	highest_amplitude = std::max(highest_amplitude,
    // this->out_real[i]); 	if (this->out_real[i] > MPM_SMALL_CUTOFF)
    // { 		auto x = util::parabolic_interpolation(this->out_real,
    // i); estimates.push_back(x); 		highest_amplitude =
    // std::max(highest_amplitude, std::get<1>(x));
    //	}
    //}

    // if (estimates.empty())
    //	return -1;

    // T actual_cutoff = MPM_CUTOFF * highest_amplitude;
    // T period = 0;

    // for (auto i : estimates) {
    //	if (std::get<1>(i) >= actual_cutoff) {
    //		period = std::get<0>(i);
    //		break;
    //	}
    //}

    // T pitch_estimate = (sample_rate / period);

    // this->clear();

    // return (pitch_estimate > MPM_LOWER_PITCH_CUTOFF) ? pitch_estimate
    // : -1; nqr::NyquistIO loader;

    // if (argc != 2) {
    //	std::cerr << "Usage: wav_analyzer /path/to/audio/file" <<
    // std::endl; 	return -1;
    //}

    // std::shared_ptr<nqr::AudioData> file_data =
    //    std::make_shared<nqr::AudioData>();
    // loader.Load(file_data.get(), argv[1]);

    // std::cout << "Audio file info:" << std::endl;

    // std::cout << "\tsample rate: " << file_data->sampleRate <<
    // std::endl; std::cout << "\tlen samples: " <<
    // file_data->samples.size() << std::endl; std::cout << "\tframe
    // size: " << file_data->frameSize << std::endl; std::cout <<
    // "\tseconds: " << file_data->lengthSeconds << std::endl; std::cout
    // << "\tchannels: " << file_data->channelCount << std::endl;

    // std::transform(file_data->samples.begin(),
    // file_data->samples.end(),
    //    file_data->samples.begin(),
    //    std::bind(std::multiplies<float>(), std::placeholders::_1,
    //    10000));

    // std::vector<float> audio;

    // if (file_data->channelCount == 2) {
    //	// convert stereo to mono
    //	std::vector<float> audio_copy(file_data->samples.size() / 2);
    //	nqr::StereoToMono(file_data->samples.data(), audio_copy.data(),
    //	    file_data->samples.size());
    //	audio = std::vector<float>(audio_copy.begin(),
    // audio_copy.end()); } else { 	audio = std::vector<float>(
    //	    file_data->samples.begin(), file_data->samples.end());
    //}

    // auto sample_rate = file_data->sampleRate;

    // auto chunk_size = size_t(sample_rate * FLAGS_timeslice);

    // auto chunks = get_chunks(audio, chunk_size);

    // std::cout << "Slicing buffer size " << audio.size() << " into "
    //          << chunks.size() << " chunks of size " << chunk_size <<
    //          std::endl;

    // double t = 0.;

    return 0.1;
}

} //  namespace

std::string DetectPitch(std::string sample_path)
{
    std::stringstream ss;
    ss << "Pitch of " << sample_path << " is - FAKED!";

    // 1. Open file - save as mono buffer
    audiofile_data afd;
    audiofile_data_import_file_contents(&afd, sample_path);

    std::cout << "Opened file: " << afd.filename << " which has "
              << afd.channels << " channels" << std::endl;

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

    std::cout << "Aiight got a MONO vector of length:" << audio.size()
              << " ORIG HAS:" << afd.samplecount << std::endl;

    auto chunk_size = size_t(SAMPLE_RATE * ktimeslice);
    auto chunks = get_chunks(audio, chunk_size);

    double t = 0.;

    for (auto chunk : chunks)
    {
        auto pitch_mpm = mpm(chunk);
        std::cout << "At t: " << t << " : " << pitch_mpm << std::endl;
        t += ktimeslice;
    }
    return ss.str();
}
