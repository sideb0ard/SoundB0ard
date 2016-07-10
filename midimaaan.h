void *midiman(void*);

void midinoteon(unsigned int midinote, int velocity);
void midinoteoff(unsigned int midinote, int velocity);
void midipitchbend(int data1, int data2);
void midicontrol(int data1, int data2);
