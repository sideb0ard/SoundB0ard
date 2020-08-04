#include <pitch_detection.hpp>
#include <sstream>

std::string DetectPitch(std::string sample_path)
{
    std::stringstream ss;
    ss << "Pitch of " << sample_path << " is - FAKED!";

    // 1. Open file - save as mono buffer
    //
    // https://miracle.otago.ac.nz/tartini/papers/Visualization_of_Musical_Pitch.pdf
    // https://miracle.otago.ac.nz/tartini/papers.html
    // 2. McLeod Pitch Method algo -
    //    a. select a sampling Window
    //    b. apply gaussian function to the window
    //    c. perform the FFT analysis
    //    d. identify the principal frequencies
    //    e. identify the fundamental as a sub-multiple of the frequency of
    //    greatest amplitude f. recognize the fundamental as a note of the
    //    musical scale
    //
    // 3. Close file
    //
    // 4. boom!

    return ss.str();
}
