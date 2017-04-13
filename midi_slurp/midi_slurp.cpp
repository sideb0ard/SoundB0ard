#include <iostream>
#include <math.h>
#include "include/MidiFile.h"

// parser for midi files using http://midifile.sapp.org/ and spits them out to my minisynth format


const static int M_PPQN = 960;

bool isNoteMessage(MidiEvent& me)
{
    if (me.isNoteOn() || me.isNoteOff())
        return true;
    return false;
}

int main(int argc, char **argv) {

    if (argc != 2) {
       std::cout << "Need a filename to work with\n";
       return -1;
    }
   MidiFile midifile(argv[1]);

   double multi = M_PPQN / (double) midifile.getTicksPerQuarterNote();

   for (int i = 0; i < midifile.getNumTracks(); i++)
   {
       MidiEventList el = midifile[i];
       for (int j = 0; j < el.size(); j ++)
       {
           MidiEvent me = el[j];

           if (!isNoteMessage(me))
               continue;

           int midi_status;
           if (me.isNoteOn())
               midi_status = 144;
           else
               midi_status = 128;

           int tick = floor(multi * me.tick);
           int midi_num = me[1];
           int midi_vel = me[2];
           std::cout << tick << "::" << midi_status << "::" << midi_num << "::" << midi_vel << std::endl;

       }
   }

   return 0;
}
