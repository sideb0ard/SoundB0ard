/* Copyright 2016, Ableton AG, Berlin. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  If you would like to incorporate Link into a proprietary software
 * application, please contact <link-devs@ableton.com>.
 */

#pragma once

#include <mixer.h>
#include <portaudio.h>

#include <ableton/link/HostTimeFilter.hpp>
#include <ableton/platforms/Config.hpp>

#include "AudioEngine.hpp"

namespace ableton {
namespace linkaudio {

class AudioPlatform {
 public:
  AudioPlatform(Link& link, Mixer& mixr);
  ~AudioPlatform();

  AudioEngine mEngine;

 private:
  static int audioCallback(const void* inputBuffer, void* outputBuffer,
                           unsigned long inNumFrames,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags, void* userData);

  void initialize();
  void uninitialize();
  void start();
  void stop();

  link::HostTimeFilter<link::platform::Clock> mHostTimeFilter;
  double mSampleTime;
  PaStream* pStream;
};

}  // namespace linkaudio
}  // namespace ableton
