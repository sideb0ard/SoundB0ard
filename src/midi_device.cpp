#include <midi_device.h>
#include <portmidi.h>

#include <iostream>

void MidiInit(Mixer *mixr) {
  std::cout << "Initializing MIDI...\n";
  PmError retval = Pm_Initialize();
  if (retval != pmNoError)
    std::cerr << "Err running Pm_Initialize: " << Pm_GetErrorText(retval)
              << std::endl;

  int cnt;
  const PmDeviceInfo *info;

  int dev = 0;

  if ((cnt = Pm_CountDevices())) {
    for (int i = 0; i < cnt; i++) {
      info = Pm_GetDeviceInfo(i);
      if (info->input && (strncmp("MPKmini2", info->name, 8) == 0)) {
        dev = i;
        break;
      }
    }
  } else {
    Pm_Terminate();
    return;
  }

  retval = Pm_OpenInput(&mixr->midi_stream, dev, NULL, 512L, NULL, NULL);
  if (retval != pmNoError) {
    std::cerr << "Err opening input for MPKmini2: " << Pm_GetErrorText(retval)
              << std::endl;
    Pm_Terminate();
    return;
  }
  mixr->midi_controller_name = "MPKmini2";
  mixr->have_midi_controller = true;
  std::cout << "Successfully opened " << mixr->midi_controller_name
            << std::endl;
}
