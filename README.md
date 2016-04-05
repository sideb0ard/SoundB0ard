# Soundb0ard Shell

## yup yup, git on them sine waves.

SBShell is an interactive music making environment, styled after a unix shell.  

To compile and run, you will need to install PortAudio and libsndfile:  
http://www.portaudio.com/  
http://www.mega-nerd.com/libsndfile/  

then, `git clone git@github.com:sideb0ard/SBShell.git`,  
run `make`
and if all is successful, you should have a new `sbsh` command in your directory.

Load it up:  
`./sbsh`

and typing `help` should give you something like::  

* `ps` - shows you status of Mixing desk - it's BPM, current tick, and list of Mixer channels
* `sine <FREQ>` - create a sine wave of FREQ Khz, e.g. `sine 440` for a middle A
* `fm <MODFREQ> <CARFREQ>` - create an Frequency Modulator.
* `ls` - show you a list of playable samples in the 'wav/' directory
* `play <FILE> <list of ticks to play on>` - play sample on ticks (tick are 0-15> e.g. `play kick2.wav 0 4 8 12`
* `loop <FREQ ... > <BARS>` - loop the list of FREQs, changing FREQ every num of BARS e.g. `loop 440 220 2`
* `sloop <FILE> <BARS>` - loop sample for BARS e.g. `sloop organ.wav 2`
* `floop <MODFREQ> <CARFREQS ...> <BARS>` - loop FMs, changing through list of CARFREQS every num of BARS



