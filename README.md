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
https://github.com/anthonix/ffts
http://ctags.sourceforge.net/
https://www.mlpack.org/
https://github.com/Reputeless/PerlinNoise

I still need to do some more work to make it portable. For the moment I've only been developing on OSX. In the Makefile there are a few lib paths and include dirs that you'll need to amend, but I believe it should compile cleanly for others with those changes. (Pull requests welcome!)


Once all these are installed and the Makefile updated ..

`git clone git@github.com:sideb0ard/SBShell.git`,
run `make`
and if all is successful, you should have a new `sbsh` command in your directory.

Load it up:
`./sbsh`

Demo here --

[![Alt text](https://img.youtube.com/vi/wNFlijArs2g/0.jpg)](https://www.youtube.com/watch?v=9h3SCJWIl4w)

