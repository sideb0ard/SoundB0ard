# Soundb0ard Shell

## yup yup, git on them sine waves.

SBShell is an interactive music making environment, styled after a unix shell.  

To compile and run, you will need to install PortAudio, PortMidi and libsndfile:  
http://www.portaudio.com/  
http://portmedia.sourceforge.net/portmidi/  
http://www.mega-nerd.com/libsndfile/  

then, `git clone git@github.com:sideb0ard/SBShell.git`,  
run `make`
and if all is successful, you should have a new `sbsh` command in your directory.

Load it up:  
`./sbsh`

and typing `help` should give you something like::  
 
```
<pre>
#### SBShell - Interactive, scriptable, algorithmic music shell ####

[Global Cmds]
bpm <bpm> -- change bpm to <bpm>
vol <soundgen_num> <vol> -- change volume of Soundgenerator to <vol>
vol mixer <vol> -- change mixer volume to <vol>
ps -- show global status and process listing
ls -- show file listing of samples, loops, and file projects
help -- this message, duh

[Sample Looper Cmds]
loop <sample> <bars> e.g. "loop amen.wav 2"
loop <soundgen_num> add <sample> <bars>
loop <soundgen_num> change <parameter> <val>


[Step Sequencer Cmds]
seq <sample> <pattern> e.g. "seq kick2.wav 0 4 8 12"
seq <soundgen_num> add <pattern> e.g. seq 0 add 4 6 10 12
seq <soundgen_num> change <pattern>
seq <soundgen_num> euclid <num_hits> [true]
   -- generates equally spaced number of beats. Optional 'true' shifts
    them forward so first hit is on first tick of cycle.
seq <soundgen_num> life - generative changing pattern, based on game of life
seq <soundgen_num> swing <swing_setting>
    -- toggles swing on/off. Setting can be between 1..6, which represent
    50%, 54%, 58%, 62%, 66%, 71%

[Synthesizer Cmds]
syn nano -- start new Nano Synth
syn <soundgen_num> keys -- control synth via keyboard
syn <soundgen_num> midi -- control synth via midi controller
syn <soundgen_num> change <parameter> <val>
syn <soundgen_num> reset -- clear pattern data
syn <soundgen_num> sustain <val>

[FX Cmds]
decimate  <sound_gen_number>
env       <sound_gen_number> <loop_len> <type>
repeat    <sound_gen_number> <loop_len> -- beatrepeat, default settings
sidechain <sound_gen_number> <input_src> <mix> -- sidechain env, where input_src is a drum pattern
fx        <sound_gen_number> <fx_num> <parameter> <val>

[Programming Cmds]
var <var_name> = <value> -- store global variable
every loop; <cmd1> ; <cmd2>; ... -- run cmds once every loop

[Chaos Monkey Cmds]
chaos monkey -- oh oh, who brought the monkey?
chaos <soundgen_num> wakeup <time_seconds> -- how often the chaos monkey wakes up
chaos <soundgen_num> chance <percent> -- how likely the chaos monkey interrupts
chaos <soundgen_num> suggest <true|false> -- Toggle chaos monkey suggestion mode
chaos <soundgen_num> action <true|false> -- Toggle chaos monkey action mode
```
</pre>
