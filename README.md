# Soundb0ard Shell

```

 _____                       _ _     _____               _
/  ___|                     | | |   |  _  |             | |
\ `--.  ___  _   _ _ __   __| | |__ | |/' | __ _ _ __ __| |
 `--. \/ _ \| | | | '_ \ / _` | '_ \|  /| |/ _` | '__/ _` |
/\__/ / (_) | |_| | | | | (_| | |_) \ |_/ / (_| | | | (_| |
\____/ \___/ \__,_|_| |_|\__,_|_.__/ \___/ \__,_|_|  \__,_|

```


Soundb0ard is an interactive music making environment, with which you interact via a unix styled shell.

It uses Ableton Link to sync with other running apps on the same local network. Follow install instructions for Link first - https://github.com/Ableton/link

Other libraries needed are PortAudio, PortMidi, Exuberant Ctags, gperf, Cscope, PerlinNoise and libsndfile:

http://www.portaudio.com/
http://portmedia.sourceforge.net/portmidi/
http://www.mega-nerd.com/libsndfile/
http://ctags.sourceforge.net/
https://github.com/Reputeless/PerlinNoise

I still need to do some more work to make it portable. For the moment I've only been developing on OSX.
I've recently moved the project to CMake and using c++20. Edit the CMakeLists.txt and adjust any paths required.
To build:
`mkdir build`
`cd build`
`cmake ..`
`cmake --build .`

then run it from the top level directory (so it can find sample dir):
`build/Sbsh`


Demo here --

[![Alt text](https://img.youtube.com/vi/wNFlijArs2g/0.jpg)](https://www.youtube.com/watch?v=VRMtDkt9qRY)

