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

Other libraries needed are PortAudio, PortMidi, Exuberant Ctags, gperf, Cscope, liblo and libsndfile:
http://www.portaudio.com/
http://portmedia.sourceforge.net/portmidi/
http://www.mega-nerd.com/libsndfile/
https://github.com/radarsat1/liblo
http://ctags.sourceforge.net/

You'll need to edit the Makefile, there's some hardcoded pathnames with my username, and also my homebrew dir is under ~

Once all these are installed..

`git clone git@github.com:sideb0ard/SBShell.git`,
run `make`
and if all is successful, you should have a new `sbsh` command in your directory.

Load it up:
`./sbsh`

