/*
  Poly 6 virtual Synth and editor

  Includes code by:
    Dave Benn - Handling MUXs, a few other bits and original inspiration  https://www.notesandvolts.com/2019/01/teensy-synth-part-10-hardware.html

  Arduino IDE
  Tools Settings:
  Board: "Teensy3.6"
  USB Type: "Serial + MIDI"
  CPU Speed: "192" OVERCLOCK
  Optimize: "Fastest"

  Additional libraries:
    Agileware CircularBuffer available in Arduino libraries manager
    Replacement files are in the Modified Libraries folder and need to be placed in the teensy Audio folder.
*/

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include "MidiCC.h"
#include "Constants.h"
#include "Parameters.h"
#include "PatchMgr.h"
#include "HWControls.h"
#include "EepromMgr.h"



#define PARAMETER 0      //The main page for displaying the current patch and control (parameter) changes
#define RECALL 1         //Patches list
#define SAVE 2           //Save patch page
#define REINITIALISE 3   // Reinitialise message
#define PATCH 4          // Show current patch bypassing PARAMETER
#define PATCHNAMING 5    // Patch naming page
#define DELETE 6         //Delete patch page
#define DELETEMSG 7      //Delete patch message page
#define SETTINGS 8       //Settings page
#define SETTINGSVALUE 9  //Settings page

unsigned int state = PARAMETER;

#include "ST7735Display.h"

boolean cardStatus = false;

//MIDI 5 Pin DIN
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
unsigned long buttonDebounce = 0;

#include "Settings.h"
//#include "SettingsService.h"

int patchNo = 1;               //Current patch no
int voiceToReturn = -1;        //Initialise
long earliestTime = millis();  //For voice allocation - initialise to now

struct VoiceAndNote {
  int note;
  int velocity;
  long timeOn;
};

struct VoiceAndNote voices[NO_OF_VOICES] = {
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 }
};

boolean voiceOn[NO_OF_VOICES] = { false, false, false, false, false, false, false, false, false, false, false, false };
int prevNote = 0;  //Initialised to middle value
bool notes[88] = { 0 }, initial_loop = 1;
int8_t noteOrder[80] = { 0 }, orderIndx = { 0 };
int noteMsg;

void setup() {
  AudioMemory(470);
  SPI.begin();
  setupDisplay();
  delay(1000);
  setUpSettings();
  setupHardware();

  cardStatus = SD.begin(BUILTIN_SDCARD);
  if (cardStatus) {
    Serial.println("SD card is connected");
    //Get patch numbers and names from SD card
    loadPatches();
    if (patches.size() == 0) {
      //save an initialised patch to SD card
      savePatch("1", INITPATCH);
      loadPatches();
    }
  } else {
    Serial.println("SD card is not connected or unusable");

    reinitialiseToPanel();
    showPatchPage("No SD", "conn'd / usable");
  }


  //USB Client MIDI#
  usbMIDI.begin();
  usbMIDI.setHandleNoteOn(myNoteOn);
  usbMIDI.setHandleNoteOff(myNoteOff);
  usbMIDI.setHandlePitchChange(myPitchBend);
  usbMIDI.setHandleControlChange(myControlChange);
  usbMIDI.setHandleProgramChange(myProgramChange);
  usbMIDI.setHandleAfterTouchChannel(myAfterTouch);
  Serial.println("USB Client MIDI Listening");

  //MIDI 5 Pin DIN
  MIDI.begin();
  MIDI.setHandleNoteOn(myNoteOn);
  MIDI.setHandleNoteOff(myNoteOff);
  MIDI.setHandlePitchBend(myPitchBend);
  MIDI.setHandleControlChange(myControlChange);
  MIDI.setHandleProgramChange(myProgramChange);
  MIDI.setHandleAfterTouchChannel(myAfterTouch);
  MIDI.turnThruOn(midi::Thru::Mode::Off);
  Serial.println("MIDI In DIN Listening");

  encCW = getEncoderDir();
  midiChannel = getMIDIChannel();
  modWheelDepth = getModWheelDepth();
  pitchBendRange = getPitchBendRange();
  afterTouchDepth = getafterTouchDepth();
  NP = getNotePriority();
  uniNotes = getUnisonNotes();
  unidetune = getUnisonDetune();
  oldunidetune = unidetune;
  switch (unidetune) {
    case 0:
      detune = 1.000;
      break;
    case 1:
      detune = 1.002;
      break;
    case 2:
      detune = 1.004;
      break;
    case 3:
      detune = 1.006;
      break;
    case 4:
      detune = 1.008;
      break;
    case 5:
      detune = 1.010;
      break;
    case 6:
      detune = 1.012;
      break;
    case 7:
      detune = 1.014;
      break;
    case 8:
      detune = 1.016;
      break;
    case 9:
      detune = 1.018;
      break;
    case 10:
      detune = 1.020;
      break;
  }
  olddetune = detune;

  //vco setup
  for (int i = 0; i < 12; i++) {
    vcoA[i]->begin(vcoVol, 150.0f, WAVEFORM_SAWTOOTH);
    vcoB[i]->begin(vcoVol, 150.0f, WAVEFORM_SQUARE);
    vcoC[i]->begin(vcoVol * 1.5f, 150.0f, WAVEFORM_ARBITRARY);
    sub[i]->begin(vcoVol * 1.5f, 150.0f, WAVEFORM_TRIANGLE);
  }

  //filter setup
  for (int i = 0; i < 12; i++) {
    filters[i]->octaveControl(7);
    filterEnvs[i]->sustain(0);
  }

  //lfo A
  lfoA1.begin(WAVEFORM_SINE);

  //lfo B
  lfoB1.begin(0.5, 1, WAVEFORM_TRIANGLE);

  // AM Queue
  lfoAQueue.begin();

  //dly
  dlyFiltL.frequency(4000);
  dlyFiltR.frequency(3000);

  dlyMixL.gain(0, 0.75);
  dlyMixL.gain(0, 0.75);


  dlyL.disable(1);
  dlyL.disable(2);
  dlyL.disable(3);
  dlyL.disable(4);
  dlyL.disable(5);
  dlyL.disable(6);
  dlyL.disable(7);

  dlyR.disable(1);
  dlyR.disable(2);
  dlyR.disable(3);
  dlyR.disable(4);
  dlyR.disable(5);
  dlyR.disable(6);
  dlyR.disable(7);


  //reverb
  reverb.roomsize(0.9);
  reverb.damping(0.8);

  //LFO DESTINATION DISCONNECT

  patchCord44.disconnect();    //lfoAenv1, 0, modMix1
  patchCord45.disconnect();    //lfoAenv1, 0, vcoB1
  patchCord46.disconnect();    //lfoAenv1, 0, vcoC1
  patchCord47.disconnect();    //lfoAenv1, 0, sub1
  patchCord48.disconnect();    //lfoAenv1, 0, filterMix1
  patchCord19.disconnect();    //lfoAenv2, 0, modMix2
  patchCord20.disconnect();    //lfoAenv2, 0, vcoB2
  patchCord21.disconnect();    //lfoAenv2, 0, vcoC2
  patchCord22.disconnect();    //lfoAenv2, 0, sub2
  patchCord23.disconnect();    //lfoAenv2, 0, filterMix2
  patchCord24.disconnect();    //lfoAenv3, 0, modMix3
  patchCord25.disconnect();    //lfoAenv3, 0, vcoB3
  patchCord26.disconnect();    //lfoAenv3, 0, vcoC3
  patchCord27.disconnect();    //lfoAenv3, 0, sub3
  patchCord28.disconnect();    //lfoAenv3, 0, filterMix3
  patchCord29.disconnect();    //lfoAenv4, 0, modMix4
  patchCord30.disconnect();    //lfoAenv4, 0, vcoB4
  patchCord31.disconnect();    //lfoAenv4, 0, vcoC4
  patchCord32.disconnect();    //lfoAenv4, 0, sub4
  patchCord33.disconnect();    //lfoAenv4, 0, filterMix4
  patchCord34.disconnect();    //lfoAenv5, 0, modMix5
  patchCord35.disconnect();    //lfoAenv5, 0, vcoB5
  patchCord36.disconnect();    //lfoAenv5, 0, vcoC5
  patchCord37.disconnect();    //lfoAenv5, 0, sub5
  patchCord38.disconnect();    //lfoAenv5, 0, filterMix5
  patchCord39.disconnect();    //lfoAenv6, 0, modMix6
  patchCord40.disconnect();    //lfoAenv6, 0, vcoB6
  patchCord41.disconnect();    //lfoAenv6, 0, vcoC6
  patchCord42.disconnect();    //lfoAenv6, 0, sub6
  patchCord43.disconnect();    //lfoAenv6, 0, filterMix6
  patchCord1019.disconnect();  //lfoAenv7, 0, modMix7
  patchCord1020.disconnect();  //lfoAenv7, 0, vcoB7
  patchCord1021.disconnect();  //lfoAenv7, 0, vcoC7
  patchCord1022.disconnect();  //lfoAenv7, 0, sub7
  patchCord1023.disconnect();  //lfoAenv7, 0, filterMix7
  patchCord1024.disconnect();  //lfoAenv8, 0, modMix8
  patchCord1025.disconnect();  //lfoAenv8, 0, vcoB8
  patchCord1025.disconnect();  //lfoAenv8, 0, vcoC8
  patchCord1027.disconnect();  //lfoAenv8, 0, sub8
  patchCord1028.disconnect();  //lfoAenv8, 0, filterMix8
  patchCord1029.disconnect();  //lfoAenv9, 0, modMix9
  patchCord1030.disconnect();  //lfoAenv9, 0, vcoB9
  patchCord1031.disconnect();  //lfoAenv9, 0, vcoC9
  patchCord1032.disconnect();  //lfoAenv9, 0, sub9
  patchCord1033.disconnect();  //lfoAenv9, 0, filterMix9
  patchCord1034.disconnect();  //lfoAenv10, 0, modMix10
  patchCord1035.disconnect();  //lfoAenv10, 0, vcoB10
  patchCord1036.disconnect();  //lfoAenv10, 0, vcoC10
  patchCord1037.disconnect();  //lfoAenv10, 0, sub10
  patchCord1038.disconnect();  //lfoAenv10, 0, filterMix10
  patchCord1039.disconnect();  //lfoAenv11, 0, modMix11
  patchCord1040.disconnect();  //lfoAenv11, 0, vcoB11
  patchCord1041.disconnect();  //lfoAenv11, 0, vcoC11
  patchCord1042.disconnect();  //lfoAenv11, 0, sub11
  patchCord1043.disconnect();  //lfoAenv11, 0, filterMix11
  patchCord1044.disconnect();  //lfoAenv12, 0, modMix12
  patchCord1045.disconnect();  //lfoAenv12, 0, vcoB12
  patchCord1046.disconnect();  //lfoAenv12, 0, vcoC12
  patchCord1047.disconnect();  //lfoAenv12, 0, sub12
  patchCord1048.disconnect();  //lfoAenv12, 0, filterMix12

  patchCordLFOQueue.disconnect();
}

int getVoiceNo(int note) {
  voiceToReturn = -1;       //Initialise to 'null'
  earliestTime = millis();  //Initialise to now
  if (note == -1) {
    //NoteOn() - Get the oldest free voice (recent voices may be still on release stage)
    for (int i = 0; i < NO_OF_VOICES; i++) {
      if (voices[i].note == -1) {
        if (voices[i].timeOn < earliestTime) {
          earliestTime = voices[i].timeOn;
          voiceToReturn = i;
        }
      }
    }
    if (voiceToReturn == -1) {
      //No free voices, need to steal oldest sounding voice
      earliestTime = millis();  //Reinitialise
      for (int i = 0; i < NO_OF_VOICES; i++) {
        if (voices[i].timeOn < earliestTime) {
          earliestTime = voices[i].timeOn;
          voiceToReturn = i;
        }
      }
    }
    return voiceToReturn + 1;
  } else {
    //NoteOff() - Get voice number from note
    for (int i = 0; i < NO_OF_VOICES; i++) {
      if (voices[i].note == note) {
        return i + 1;
      }
    }
  }
  //Shouldn't get here, return voice 1
  return 1;
}

void myNoteOn(byte channel, byte note, byte velocity) {

  if (MONO_POLY_1 < 511 && MONO_POLY_2 < 511) {

    if (note < 0 || note > 127) return;
    switch (getVoiceNo(-1)) {
      case 1:
        voices[0].note = note;
        note1freq = note;
        env1.noteOn();
        filterEnv1.noteOn();
        lfoAenv1.noteOn();
        env1on = true;
        voiceOn[0] = true;
        break;

      case 2:
        voices[1].note = note;
        note2freq = note;
        env2.noteOn();
        filterEnv2.noteOn();
        lfoAenv2.noteOn();
        env2on = true;
        voiceOn[1] = true;
        break;

      case 3:
        voices[2].note = note;
        note3freq = note;
        env3.noteOn();
        filterEnv3.noteOn();
        lfoAenv3.noteOn();
        env3on = true;
        voiceOn[2] = true;
        break;

      case 4:
        voices[3].note = note;
        note4freq = note;
        env4.noteOn();
        filterEnv4.noteOn();
        lfoAenv4.noteOn();
        env4on = true;
        voiceOn[3] = true;
        break;

      case 5:
        voices[4].note = note;
        note5freq = note;
        env5.noteOn();
        filterEnv5.noteOn();
        lfoAenv5.noteOn();
        env5on = true;
        voiceOn[4] = true;
        break;

      case 6:
        voices[5].note = note;
        note6freq = note;
        env6.noteOn();
        filterEnv6.noteOn();
        lfoAenv6.noteOn();
        env6on = true;
        voiceOn[5] = true;
        break;

      case 7:
        voices[6].note = note;
        note7freq = note;
        env7.noteOn();
        filterEnv7.noteOn();
        lfoAenv7.noteOn();
        env7on = true;
        voiceOn[6] = true;
        break;

      case 8:
        voices[7].note = note;
        note8freq = note;
        env8.noteOn();
        filterEnv8.noteOn();
        lfoAenv8.noteOn();
        env8on = true;
        voiceOn[7] = true;
        break;

      case 9:
        voices[8].note = note;
        note9freq = note;
        env9.noteOn();
        filterEnv9.noteOn();
        lfoAenv9.noteOn();
        env9on = true;
        voiceOn[8] = true;
        break;

      case 10:
        voices[9].note = note;
        note10freq = note;
        env10.noteOn();
        filterEnv10.noteOn();
        lfoAenv10.noteOn();
        env10on = true;
        voiceOn[9] = true;
        break;

      case 11:
        voices[10].note = note;
        note11freq = note;
        env11.noteOn();
        filterEnv11.noteOn();
        lfoAenv11.noteOn();
        env11on = true;
        voiceOn[10] = true;
        break;

      case 12:
        voices[11].note = note;
        note12freq = note;
        env12.noteOn();
        filterEnv12.noteOn();
        lfoAenv12.noteOn();
        env12on = true;
        voiceOn[11] = true;
        break;
    }
  }

  if (MONO_POLY_1 > 511 && MONO_POLY_2 < 511) {  //UNISON mode
    detune = olddetune;
    noteMsg = note;

    if (velocity == 0) {
      notes[noteMsg] = false;
    } else {
      notes[noteMsg] = true;
    }

    switch (NP) {
      case 0:
        commandTopNoteUnison();
        break;

      case 1:
        commandBottomNoteUnison();
        break;

      case 2:
        if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
          orderIndx = (orderIndx + 1) % 40;
          noteOrder[orderIndx] = noteMsg;
        }
        commandLastNoteUnison();
        break;
    }
  }

  if (MONO_POLY_1 < 511 && MONO_POLY_2 > 511) {
    noteMsg = note;

    if (velocity == 0) {
      notes[noteMsg] = false;
    } else {
      notes[noteMsg] = true;
    }

    switch (NP) {
      case 0:
        commandTopNote();
        break;

      case 1:
        commandBottomNote();
        break;

      case 2:
        if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
          orderIndx = (orderIndx + 1) % 40;
          noteOrder[orderIndx] = noteMsg;
        }
        commandLastNote();
        break;
    }
  }
}

void myNoteOff(byte channel, byte note, byte velocity) {

  if (MONO_POLY_1 < 511 && MONO_POLY_2 < 511) {  //POLYPHONIC mode

    switch (getVoiceNo(note)) {
      case 1:
        env1.noteOff();
        filterEnv1.noteOff();
        lfoAenv1.noteOff();
        env1on = false;
        voices[0].note = -1;
        voiceOn[0] = false;
        break;

      case 2:
        env2.noteOff();
        filterEnv2.noteOff();
        lfoAenv2.noteOff();
        env2on = false;
        voices[1].note = -1;
        voiceOn[1] = false;
        break;

      case 3:
        env3.noteOff();
        filterEnv3.noteOff();
        lfoAenv3.noteOff();
        env3on = false;
        voices[2].note = -1;
        voiceOn[2] = false;
        break;

      case 4:
        env4.noteOff();
        filterEnv4.noteOff();
        lfoAenv4.noteOff();
        env4on = false;
        voices[3].note = -1;
        voiceOn[3] = false;
        break;

      case 5:
        env5.noteOff();
        filterEnv5.noteOff();
        lfoAenv5.noteOff();
        env5on = false;
        voices[4].note = -1;
        voiceOn[4] = false;
        break;

      case 6:
        env6.noteOff();
        filterEnv6.noteOff();
        lfoAenv6.noteOff();
        env6on = false;
        voices[5].note = -1;
        voiceOn[5] = false;
        break;

      case 7:
        env7.noteOff();
        filterEnv7.noteOff();
        lfoAenv7.noteOff();
        env7on = false;
        voices[6].note = -1;
        voiceOn[6] = false;
        break;

      case 8:
        env8.noteOff();
        filterEnv8.noteOff();
        lfoAenv8.noteOff();
        env8on = false;
        voices[7].note = -1;
        voiceOn[7] = false;
        break;

      case 9:
        env9.noteOff();
        filterEnv9.noteOff();
        lfoAenv9.noteOff();
        env9on = false;
        voices[8].note = -1;
        voiceOn[8] = false;
        break;

      case 10:
        env10.noteOff();
        filterEnv10.noteOff();
        lfoAenv10.noteOff();
        env10on = false;
        voices[9].note = -1;
        voiceOn[9] = false;
        break;

      case 11:
        env11.noteOff();
        filterEnv11.noteOff();
        lfoAenv11.noteOff();
        env11on = false;
        voices[10].note = -1;
        voiceOn[10] = false;
        break;

      case 12:
        env12.noteOff();
        filterEnv12.noteOff();
        lfoAenv12.noteOff();
        env12on = false;
        voices[11].note = -1;
        voiceOn[11] = false;
        break;
    }
  }

  if (MONO_POLY_1 > 511 && MONO_POLY_2 < 511) {  //UNISON
    detune = olddetune;
    noteMsg = note;
    notes[noteMsg] = false;

    switch (NP) {
      case 0:
        commandTopNoteUnison();
        break;

      case 1:
        commandBottomNoteUnison();
        break;

      case 2:
        if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
          orderIndx = (orderIndx + 1) % 40;
          noteOrder[orderIndx] = noteMsg;
        }
        commandLastNoteUnison();
        break;
    }
  }

  if (MONO_POLY_1 < 511 && MONO_POLY_2 > 511) {
    detune = 1.000;
    noteMsg = note;
    notes[noteMsg] = false;

    switch (NP) {
      case 0:
        commandTopNote();
        break;

      case 1:
        commandBottomNote();
        break;

      case 2:
        if (notes[noteMsg]) {  // If note is on and using last note priority, add to ordered list
          orderIndx = (orderIndx + 1) % 40;
          noteOrder[orderIndx] = noteMsg;
        }
        commandLastNote();
        break;
    }
  }
}

int mod(int a, int b) {
  int r = a % b;
  return r < 0 ? r + b : r;
}

void commandTopNote() {
  int topNote = 0;
  bool noteActive = false;

  for (int i = 0; i < 88; i++) {
    if (notes[i]) {
      topNote = i;
      noteActive = true;
    }
  }

  if (noteActive) {
    commandNote(topNote);
  } else {  // All notes are off, turn off gate
    env1.noteOff();
    filterEnv1.noteOff();
    lfoAenv1.noteOff();
    env1on = false;
  }
}

void commandBottomNote() {

  int bottomNote = 0;
  bool noteActive = false;

  for (int i = 87; i >= 0; i--) {
    if (notes[i]) {
      bottomNote = i;
      noteActive = true;
    }
  }

  if (noteActive) {
    commandNote(bottomNote);
  } else {  // All notes are off, turn off gate
    env1.noteOff();
    filterEnv1.noteOff();
    lfoAenv1.noteOff();
    env1on = false;
  }
}

void commandLastNote() {

  int8_t noteIndx;

  for (int i = 0; i < 40; i++) {
    noteIndx = noteOrder[mod(orderIndx - i, 40)];
    if (notes[noteIndx]) {
      commandNote(noteIndx);
      return;
    }
  }
  env1.noteOff();
  filterEnv1.noteOff();
  lfoAenv1.noteOff();
  env1on = false;
}

void commandNote(int note) {

  note1freq = note;
  env1.noteOn();
  filterEnv1.noteOn();
  lfoAenv1.noteOn();
  env1on = true;
}

void commandTopNoteUnison() {
  int topNote = 0;
  bool noteActive = false;

  for (int i = 0; i < 88; i++) {
    if (notes[i]) {
      topNote = i;
      noteActive = true;
    }
  }

  if (noteActive) {
    commandNoteUnison(topNote);
  } else {  // All notes are off, turn off gate
    for (int i = 0; i < uniNotes; i++) {
      voices[i].note = -1;  // clear note assignment
      switch (i) {
        case 0:
          env1.noteOff();
          filterEnv1.noteOff();
          lfoAenv1.noteOff();
          env1on = false;
          break;
        case 1:
          env2.noteOff();
          filterEnv2.noteOff();
          lfoAenv2.noteOff();
          env2on = false;
          break;
        case 2:
          env3.noteOff();
          filterEnv3.noteOff();
          lfoAenv3.noteOff();
          env3on = false;
          break;
        case 3:
          env4.noteOff();
          filterEnv4.noteOff();
          lfoAenv4.noteOff();
          env4on = false;
          break;
        case 4:
          env5.noteOff();
          filterEnv5.noteOff();
          lfoAenv5.noteOff();
          env5on = false;
          break;
        case 5:
          env6.noteOff();
          filterEnv6.noteOff();
          lfoAenv6.noteOff();
          env6on = false;
          break;
        case 6:
          env7.noteOff();
          filterEnv7.noteOff();
          lfoAenv7.noteOff();
          env7on = false;
          break;
        case 7:
          env8.noteOff();
          filterEnv8.noteOff();
          lfoAenv8.noteOff();
          env8on = false;
          break;
        case 8:
          env9.noteOff();
          filterEnv9.noteOff();
          lfoAenv9.noteOff();
          env9on = false;
          break;
        case 9:
          env10.noteOff();
          filterEnv10.noteOff();
          lfoAenv10.noteOff();
          env10on = false;
          break;
        case 10:
          env11.noteOff();
          filterEnv11.noteOff();
          lfoAenv11.noteOff();
          env11on = false;
          break;
        case 11:
          env12.noteOff();
          filterEnv12.noteOff();
          lfoAenv12.noteOff();
          env12on = false;
          break;
      }
    }
  }
}

void commandBottomNoteUnison() {

  int bottomNote = 0;
  bool noteActive = false;

  for (int i = 87; i >= 0; i--) {
    if (notes[i]) {
      bottomNote = i;
      noteActive = true;
    }
  }

  if (noteActive) {
    commandNoteUnison(bottomNote);
  } else {  // All notes are off, turn off gate
    for (int i = 0; i < uniNotes; i++) {
      voices[i].note = -1;  // clear note assignment
      switch (i) {
        case 0:
          env1.noteOff();
          filterEnv1.noteOff();
          lfoAenv1.noteOff();
          env1on = false;
          break;
        case 1:
          env2.noteOff();
          filterEnv2.noteOff();
          lfoAenv2.noteOff();
          env2on = false;
          break;
        case 2:
          env3.noteOff();
          filterEnv3.noteOff();
          lfoAenv3.noteOff();
          env3on = false;
          break;
        case 3:
          env4.noteOff();
          filterEnv4.noteOff();
          lfoAenv4.noteOff();
          env4on = false;
          break;
        case 4:
          env5.noteOff();
          filterEnv5.noteOff();
          lfoAenv5.noteOff();
          env5on = false;
          break;
        case 5:
          env6.noteOff();
          filterEnv6.noteOff();
          lfoAenv6.noteOff();
          env6on = false;
          break;
        case 6:
          env7.noteOff();
          filterEnv7.noteOff();
          lfoAenv7.noteOff();
          env7on = false;
          break;
        case 7:
          env8.noteOff();
          filterEnv8.noteOff();
          lfoAenv8.noteOff();
          env8on = false;
          break;
        case 8:
          env9.noteOff();
          filterEnv9.noteOff();
          lfoAenv9.noteOff();
          env9on = false;
          break;
        case 9:
          env10.noteOff();
          filterEnv10.noteOff();
          lfoAenv10.noteOff();
          env10on = false;
          break;
        case 10:
          env11.noteOff();
          filterEnv11.noteOff();
          lfoAenv11.noteOff();
          env11on = false;
          break;
        case 11:
          env12.noteOff();
          filterEnv12.noteOff();
          lfoAenv12.noteOff();
          env12on = false;
          break;
      }
    }
  }
}

void commandLastNoteUnison() {

  int8_t noteIndx;

  for (int i = 0; i < 40; i++) {
    noteIndx = noteOrder[mod(orderIndx - i, 40)];
    if (notes[noteIndx]) {
      commandNoteUnison(noteIndx);
      return;
    }
  }
  for (int i = 0; i < uniNotes; i++) {
    voices[i].note = -1;  // clear note assignment
    switch (i) {
      case 0:
        env1.noteOff();
        filterEnv1.noteOff();
        lfoAenv1.noteOff();
        env1on = false;
        break;
      case 1:
        env2.noteOff();
        filterEnv2.noteOff();
        lfoAenv2.noteOff();
        env2on = false;
        break;
      case 2:
        env3.noteOff();
        filterEnv3.noteOff();
        lfoAenv3.noteOff();
        env3on = false;
        break;
      case 3:
        env4.noteOff();
        filterEnv4.noteOff();
        lfoAenv4.noteOff();
        env4on = false;
        break;
      case 4:
        env5.noteOff();
        filterEnv5.noteOff();
        lfoAenv5.noteOff();
        env5on = false;
        break;
      case 5:
        env6.noteOff();
        filterEnv6.noteOff();
        lfoAenv6.noteOff();
        env6on = false;
        break;
      case 6:
        env7.noteOff();
        filterEnv7.noteOff();
        lfoAenv7.noteOff();
        env7on = false;
        break;
      case 7:
        env8.noteOff();
        filterEnv8.noteOff();
        lfoAenv8.noteOff();
        env8on = false;
        break;
      case 8:
        env9.noteOff();
        filterEnv9.noteOff();
        lfoAenv9.noteOff();
        env9on = false;
        break;
      case 9:
        env10.noteOff();
        filterEnv10.noteOff();
        lfoAenv10.noteOff();
        env10on = false;
        break;
      case 10:
        env11.noteOff();
        filterEnv11.noteOff();
        lfoAenv11.noteOff();
        env11on = false;
        break;
      case 11:
        env12.noteOff();
        filterEnv12.noteOff();
        lfoAenv12.noteOff();
        env12on = false;
        break;
    }
  }
}

void commandNoteUnison(int note) {
  // Limit to available voices
  if (uniNotes > NO_OF_VOICES) uniNotes = NO_OF_VOICES;
  if (uniNotes < 1) uniNotes = 1;

  // Set note frequency base
  for (int i = 0; i < uniNotes; i++) {
    voices[i].note = note;  // Optional bookkeeping
  }

  // Calculate detune spread
  float baseOffset = detune - 1.000;  // e.g. 0.02 for ±2%
  float spread = baseOffset;          // could later be scaled with uniNotes

  // Center index (for symmetry)
  int center = uniNotes / 2;  // integer division works for both even/odd

  // Reset all detunes first
  for (int i = 0; i < NO_OF_VOICES; i++) {
    voiceDetune[i] = 1.000;
  }

  // Assign detunes to active voices
  for (int i = 0; i < uniNotes; i++) {
    int distance = i - center;
    voiceDetune[i] = 1.000 + (distance * spread);
  }

  // Trigger only the voices used by unison
  for (int i = 0; i < uniNotes; i++) {
    switch (i) {
      case 0:
        note1freq = note;
        env1.noteOn();
        filterEnv1.noteOn();
        lfoAenv1.noteOn();
        env1on = true;
        break;
      case 1:
        note2freq = note;
        env2.noteOn();
        filterEnv2.noteOn();
        lfoAenv2.noteOn();
        env2on = true;
        break;
      case 2:
        note3freq = note;
        env3.noteOn();
        filterEnv3.noteOn();
        lfoAenv3.noteOn();
        env3on = true;
        break;
      case 3:
        note4freq = note;
        env4.noteOn();
        filterEnv4.noteOn();
        lfoAenv4.noteOn();
        env4on = true;
        break;
      case 4:
        note5freq = note;
        env5.noteOn();
        filterEnv5.noteOn();
        lfoAenv5.noteOn();
        env5on = true;
        break;
      case 5:
        note6freq = note;
        env6.noteOn();
        filterEnv6.noteOn();
        lfoAenv6.noteOn();
        env6on = true;
        break;
      case 6:
        note7freq = note;
        env7.noteOn();
        filterEnv7.noteOn();
        lfoAenv7.noteOn();
        env7on = true;
        break;
      case 7:
        note8freq = note;
        env8.noteOn();
        filterEnv8.noteOn();
        lfoAenv8.noteOn();
        env8on = true;
        break;
      case 8:
        note9freq = note;
        env9.noteOn();
        filterEnv9.noteOn();
        lfoAenv9.noteOn();
        env9on = true;
        break;
      case 9:
        note10freq = note;
        env10.noteOn();
        filterEnv10.noteOn();
        lfoAenv10.noteOn();
        env10on = true;
        break;
      case 10:
        note11freq = note;
        env11.noteOn();
        filterEnv11.noteOn();
        lfoAenv11.noteOn();
        env11on = true;
        break;
      case 11:
        note12freq = note;
        env12.noteOn();
        filterEnv12.noteOn();
        lfoAenv12.noteOn();
        env12on = true;
        break;
    }
  }
}

void allNotesOff() {

  voices[0].note = -1;
  voiceOn[0] = false;
  env1.noteOff();
  filterEnv1.noteOff();
  lfoAenv1.noteOff();
  env1on = false;

  voices[1].note = -1;
  voiceOn[1] = false;
  env2.noteOff();
  filterEnv2.noteOff();
  lfoAenv2.noteOff();
  env2on = false;

  voices[2].note = -1;
  voiceOn[2] = false;
  env3.noteOff();
  filterEnv3.noteOff();
  lfoAenv3.noteOff();
  env3on = false;

  voices[3].note = -1;
  voiceOn[3] = false;
  env4.noteOff();
  filterEnv4.noteOff();
  lfoAenv4.noteOff();
  env4on = false;

  voices[4].note = -1;
  voiceOn[4] = false;
  env5.noteOff();
  filterEnv5.noteOff();
  lfoAenv5.noteOff();
  env5on = false;

  voices[5].note = -1;
  voiceOn[5] = false;
  env6.noteOff();
  filterEnv6.noteOff();
  lfoAenv6.noteOff();
  env6on = false;

  voices[6].note = -1;
  voiceOn[6] = false;
  env7.noteOff();
  filterEnv7.noteOff();
  lfoAenv7.noteOff();
  env7on = false;

  voices[7].note = -1;
  voiceOn[7] = false;
  env8.noteOff();
  filterEnv8.noteOff();
  lfoAenv8.noteOff();
  env8on = false;

  voices[8].note = -1;
  voiceOn[8] = false;
  env9.noteOff();
  filterEnv9.noteOff();
  lfoAenv9.noteOff();
  env9on = false;

  voices[9].note = -1;
  voiceOn[9] = false;
  env10.noteOff();
  filterEnv10.noteOff();
  lfoAenv10.noteOff();
  env10on = false;

  voices[10].note = -1;
  voiceOn[10] = false;
  env11.noteOff();
  filterEnv11.noteOff();
  lfoAenv11.noteOff();
  env11on = false;

  voices[11].note = -1;
  voiceOn[11] = false;
  env12.noteOff();
  filterEnv12.noteOff();
  lfoAenv12.noteOff();
  env12on = false;
}

FLASHMEM void updateVolume() {
  //mainVol = (float)mux23 / 1024;
  //vco Mixer
  float gainA = vcoAvol * mainVol;
  float gainB = vcoBvol * mainVol;
  float gainC = vcoCvol * mainVol;
  float gainSub = Subvol * mainVol;

  // Apply to all 12 voice mixers
  for (int i = 0; i < 12; i++) {
    voiceMixers[i]->gain(0, gainA);
    voiceMixers[i]->gain(1, gainB);
    voiceMixers[i]->gain(2, gainC);
    voiceMixers[i]->gain(3, gainSub);
  }
  if (!announce) {
    showCurrentParameterPage("Volume", String(mux23 >> 3));
  }
  midiCCOut(CCvolumeControl, (mux23 >> 3), 1);
}

//main octave
FLASHMEM void updateMainOctave() {
  if (MAIN_OCT_1 > 511) {
    octave = 0.5;
    if (!announce) {
      showCurrentParameterPage("Main Octave", "-1");
    }
    midiCCOut(CCoctave_1, 0, 1);
  } else if (MAIN_OCT_1 < 511 && MAIN_OCT_2 < 511) {
    octave = 1;
    if (!announce) {
      showCurrentParameterPage("Main Octave", "0");
    }
    midiCCOut(CCoctave_1, 63, 1);
  } else if (MAIN_OCT_2 > 511) {
    octave = 2;
    if (!announce) {
      showCurrentParameterPage("Main Octave", "+1");
    }
    midiCCOut(CCoctave_1, 127, 1);
  }
}

///////////////  OCTAVES OCTAVES /////////////////////////////////////////////////////////////7////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////


//octave vco B
FLASHMEM void updateOctaveB() {
  if (B_OCTAVE_1 > 511) {
    octaveB = 0.5;
    if (!announce) {
      showCurrentParameterPage("OscB Octave", "-1");
    }
    midiCCOut(CCosc_b_oct_1, 0, 1);
  } else if (B_OCTAVE_1 < 511 && B_OCTAVE_2 < 511) {
    octaveB = 1;
    if (!announce) {
      showCurrentParameterPage("OscB Octave", "0");
    }
    midiCCOut(CCosc_b_oct_1, 63, 1);
  } else if (B_OCTAVE_2 > 511) {
    octaveB = 2;
    if (!announce) {
      showCurrentParameterPage("OscB Octave", "+1");
    }
    midiCCOut(CCosc_b_oct_1, 127, 1);
  }
}

FLASHMEM void updatemonoPoly() {

  if (MONO_POLY_1 < 511 && MONO_POLY_2 < 511) {
    showCurrentParameterPage("KeyMode", "Polyphonic");
    for (int i = 0; i < NO_OF_VOICES; i++) {
      voiceDetune[i] = 1.000;
    }
  }
  if (MONO_POLY_1 > 511 && MONO_POLY_2 < 511) {
    showCurrentParameterPage("KeyMode", "Unison");
  }
  if (MONO_POLY_1 < 511 && MONO_POLY_2 > 511) {
    showCurrentParameterPage("KeyMode", "Mono");
  }
}

FLASHMEM void updateOctaveC() {
  if (C_OCTAVE_1 > 511) {
    octaveC = 0.5;
    if (!announce) {
      showCurrentParameterPage("OscC Octave", "-1");
    }
    midiCCOut(CCosc_c_oct_1, 0, 1);
  } else if (C_OCTAVE_1 < 511 && C_OCTAVE_2 < 511) {
    octaveC = 1;
    if (!announce) {
      showCurrentParameterPage("OscC Octave", "0");
    }
    midiCCOut(CCosc_c_oct_1, 63, 1);
  } else if (C_OCTAVE_2 > 511) {
    octaveC = 2;
    if (!announce) {
      showCurrentParameterPage("OscB Octave", "+1");
    }
    midiCCOut(CCosc_c_oct_1, 127, 1);
  }
}

//Shape A
FLASHMEM void updateShapeA() {
  if (A_SHAPE_1 > 511) {
    shapeA = 0;
    if (!announce) {
      showCurrentParameterPage("OscA Wave", "Square");
    }
    midiCCOut(CCosc_a_shape_1, 0, 1);
  } else if (A_SHAPE_1 < 511 && A_SHAPE_2 < 511) {
    shapeA = 1;
    if (!announce) {
      showCurrentParameterPage("OscA Wave", "Sawtooth");
    }
    midiCCOut(CCosc_a_shape_1, 63, 1);
  } else if (A_SHAPE_2 > 511) {
    shapeA = 2;
    if (!announce) {
      showCurrentParameterPage("OscA Wave", "Triangle");
    }
    midiCCOut(CCosc_a_shape_1, 127, 1);
  }
  //vco A shape
  for (int i = 0; i < 12; i++) {
    switch (shapeA) {
      case 0:  // Square / Pulse
        vcoA[i]->begin(WAVEFORM_PULSE);
        vcoA[i]->amplitude(vcoVol);
        break;

      case 1:  // Sawtooth
        vcoA[i]->begin(WAVEFORM_SAWTOOTH);
        vcoA[i]->amplitude(vcoVol);
        break;

      case 2:  // Triangle
        vcoA[i]->begin(WAVEFORM_TRIANGLE_VARIABLE);
        vcoA[i]->amplitude(vcoVol * 1.5f);
        break;
    }
  }
}

//Shape B
FLASHMEM void updateShapeB() {
  if (B_SHAPE_1 > 511) {
    shapeB = 0;
    if (!announce) {
      showCurrentParameterPage("OscB Wave", "Square");
    }
    midiCCOut(CCosc_b_shape_1, 0, 1);
  } else if (B_SHAPE_1 < 511 && B_SHAPE_2 < 511) {
    shapeB = 1;
    if (!announce) {
      showCurrentParameterPage("OscB Wave", "Sawtooth");
    }
    midiCCOut(CCosc_b_shape_1, 63, 1);
  } else if (B_SHAPE_2 > 511) {
    shapeB = 2;
    if (!announce) {
      showCurrentParameterPage("OscB Wave", "Triangle");
    }
    midiCCOut(CCosc_b_shape_1, 127, 1);
  }
  //vco B shape
  for (int i = 0; i < 12; i++) {
    switch (shapeA) {
      case 0:  // Square / Pulse
        vcoB[i]->begin(WAVEFORM_PULSE);
        vcoB[i]->amplitude(vcoVol);
        break;

      case 1:  // Sawtooth
        vcoB[i]->begin(WAVEFORM_SAWTOOTH);
        vcoB[i]->amplitude(vcoVol);
        break;

      case 2:  // Triangle
        vcoB[i]->begin(WAVEFORM_TRIANGLE_VARIABLE);
        vcoB[i]->amplitude(vcoVol * 1.5f);
        break;
    }
  }
}

//Vco C shape
FLASHMEM void updateShapeC() {
  shapeC = mux11;
  midiCCOut(CCosc_C_shape, (mux11 >> 3), 1);
  if (!announce) {
    showCurrentParameterPage("OscC Wave", String(map(mux11, 0, 1023, 1, 28)));
  }
  //Vco C shapes
  switch (shapeC) {
    case 1 ... 32:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave1, 2000);
      }
      break;
    case 37 ... 69:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave2, 2000);
      }
      break;
    case 73 ... 105:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave3, 2000);
      }
      break;
    case 109 ... 141:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave4, 2000);
      }
      break;
    case 145 ... 177:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave5, 2000);
      }
      break;
    case 181 ... 212:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave6, 2000);
      }
      break;
    case 217 ... 248:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave7, 2000);
      }
      break;
    case 253 ... 285:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave8, 2000);
      }
      break;
    case 289 ... 320:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave9, 2000);
      }
      break;
    case 325 ... 357:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave10, 2000);
      }
      break;
    case 361 ... 393:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave11, 2000);
      }
      break;
    case 397 ... 429:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave12, 2000);
      }
      break;
    case 433 ... 465:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave13, 2000);
      }
      break;
    case 469 ... 500:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave14, 2000);
      }
      break;
    case 505 ... 537:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave15, 2000);
      }
      break;
    case 541 ... 573:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave16, 2000);
      }
      break;
    case 577 ... 609:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave17, 2000);
      }
      break;
    case 613 ... 645:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave18, 2000);
      }
      break;
    case 649 ... 680:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave19, 2000);
      }
      break;
    case 685 ... 717:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave20, 2000);
      }
      break;
    case 721 ... 752:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave21, 2000);
      }
      break;
    case 757 ... 789:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave22, 2000);
      }
      break;
    case 793 ... 825:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave23, 2000);
      }
      break;
    case 829 ... 860:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave24, 2000);
      }
      break;
    case 865 ... 896:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave25, 2000);
      }
      break;
    case 901 ... 933:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave26, 2000);
      }
      break;
    case 937 ... 966:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave27, 2000);
      }
      break;
    case 970 ... 1024:
      for (int i = 0; i < 12; i++) {
        vcoC[i]->arbitraryWaveform(wave28, 2000);
      }
      break;
  }
}

//tuneB
FLASHMEM void updateTuneB() {
  // if (mux13 < 512) {
  //   tuneB = ((float)mux13 / 1023) + 0.5;
  // } else {
  //   tuneB = ((float)mux13 / 510);
  // }
  if (!announce) {
    showCurrentParameterPage("OscB Tune", String(mux13 >> 3));
  }
  midiCCOut(CCosc_B_freq, (mux13 >> 3), 1);
}

//tuneC
FLASHMEM void updateTuneC() {
  // if (mux12 < 512) {
  //   tuneC = ((float)mux12 / 1023) + 0.5;
  // } else {
  //   tuneC = ((float)mux12 / 510);
  // }
  if (!announce) {
    showCurrentParameterPage("OscC Tune", String(mux12 >> 3));
  }
  midiCCOut(CCosc_C_freq, (mux12 >> 3), 1);
}

//Cross mod
FLASHMEM void updateCrossMod() {
  //crossMod = (float)mux14 / 512;
  //cross mod
  for (int i = 0; i < 12; i++) {
    modMixers[i]->gain(0, crossMod);
  }
  if (!announce) {
    showCurrentParameterPage("Cross Mod", String(mux14 >> 3));
  }
  midiCCOut(CCcrossmod, (mux14 >> 3), 1);
}


FLASHMEM void updateVolA() {
  //vcoAvol = (float)mux10 / 1023;
  //vco Mixer
  float gainValue = vcoAvol * mainVol;
  for (int i = 0; i < 12; i++) {
    voiceMixers[i]->gain(0, gainValue);
  }
  if (!announce) {
    showCurrentParameterPage("OscA Volume", String(mux10 >> 3));
  }
  midiCCOut(CCosc_A_vol, (mux10 >> 3), 1);
}

FLASHMEM void updateVolB() {
  //vcoBvol = (float)mux9 / 1023;
  //vco Mixer
  float gainValue = vcoBvol * mainVol;
  for (int i = 0; i < 12; i++) {
    voiceMixers[i]->gain(1, gainValue);
  }
  if (!announce) {
    showCurrentParameterPage("OscB Volume", String(mux9 >> 3));
  }
  midiCCOut(CCosc_B_vol, (mux9 >> 3), 1);
}

FLASHMEM void updateVolC() {
  //vcoCvol = (float)mux8 / 1023;
  //vco Mixer
  float gainValue = vcoCvol * mainVol;
  for (int i = 0; i < 12; i++) {
    voiceMixers[i]->gain(2, gainValue);
  }
  if (!announce) {
    showCurrentParameterPage("OscC Volume", String(mux8 >> 3));
  }
  midiCCOut(CCosc_C_vol, (mux8 >> 3), 1);
}

FLASHMEM void updateSubVol() {
  //Subvol = (float)mux17 / 1023;
  //vco Mixer
  float gainValue = Subvol * mainVol;
  for (int i = 0; i < 12; i++) {
    voiceMixers[i]->gain(3, gainValue);
  }
  if (!announce) {
    showCurrentParameterPage("Sub Volume", String(mux17 >> 3));
  }
  midiCCOut(CCosc_Subvol, (mux17 >> 3), 1);
}

//Filter
FLASHMEM void updateCutoff() {
  //cut = 15000 * (float)mux25 / 1023 + 15;  /////cut
  //filter
  for (int i = 0; i < 12; i++) {
    filters[i]->frequency(cut);
  }
  if (!announce) {
    showCurrentParameterPage("Filter Cutoff", String(cutoffstr) + " Hz");
  }
  midiCCOut(CCvcf_frequency, (mux25 >> 3), 1);
}

FLASHMEM void updateRes() {
  //res = 4.5 * (float)mux24 / 1023 + 1.1;
  //filter

  for (int i = 0; i < 12; i++) {
    filters[i]->resonance(res);
  }
  if (!announce) {
    showCurrentParameterPage("Filter Res", String(mux24 >> 3));
  }
  midiCCOut(CCvcf_resonance, (mux24 >> 3), 1);
}

//Filter Env

FLASHMEM void updateFilterAttack() {
  //filtAtt = (3000 * (float)mux0 / 1023);
  for (int i = 0; i < 12; i++) {
    filterEnvs[i]->attack(filtAtt);
  }
  if (!announce) {
    showCurrentParameterPage("Filter Attack", String(filterAttackstr) + " mS");
  }
  midiCCOut(CCvcf_attack, (mux0 >> 3), 1);
}

FLASHMEM void updateFilterDecay() {
  //filtDec = (3000 * (float)mux1 / 1023);
  for (int i = 0; i < 12; i++) {
    filterEnvs[i]->decay(filtDec);
    filterEnvs[i]->release(filtDec);
  }
  if (!announce) {
    showCurrentParameterPage("Filter Decay", String(filterDecaystr) + " mS");
  }
  midiCCOut(CCvcf_decay, (mux1 >> 3), 1);
}

FLASHMEM void updateFilterAmount() {
  //filtAmt = (float)mux2 / 512 - 1;
  for (int i = 0; i < 12; i++) {
    dcArray[i]->amplitude(filtAmt);
  }
  if (!announce) {
    showCurrentParameterPage("Filter Env", String(mux2 >> 3));
  }
  midiCCOut(CCvcf_env_amount, (mux2 >> 3), 1);
}

FLASHMEM void updateFilterMode() {
  // Determine filter mode from FILTER_MODE value
  if (FILTER_MODE > 511) {
    filterMode = 1;  // Low Pass
    showCurrentParameterPage("Filter Mode", "Low Pass");
    midiCCOut(CCfiltermode, 127, 1);
  } else {
    filterMode = 0;  // Band Pass
    if (!announce) {
      showCurrentParameterPage("Filter Mode", "Band Pass");
    }
    midiCCOut(CCfiltermode, 0, 1);
  }

  // Apply the correct gain settings for each filterMode mixer
  for (int i = 0; i < 12; i++) {
    if (filterMode == 1) {  // Low Pass
      filterModes[i]->gain(0, 1);
      filterModes[i]->gain(1, 0);
    } else {  // Band Pass
      filterModes[i]->gain(0, 0);
      filterModes[i]->gain(1, 1);
    }
  }
}

FLASHMEM void updateAttack() {
  //envAtt = 3000 * (float)mux27 / 1023;
  //Main ENVELOPE
  for (int i = 0; i < 12; i++) {
    envs[i]->attack(envAtt);
  }
  if (!announce) {
    showCurrentParameterPage("Amp Attack", String(ampAttackstr) + " mS");
  }
  midiCCOut(CCvca_attack, (mux27 >> 3), 1);
}

FLASHMEM void updateDecay() {
  //envDec = 5000 * (float)mux26 / 1023;
  //envRel = 5000 * (float)mux26 / 1023;
  for (int i = 0; i < 12; i++) {
    envs[i]->decay(envDec);
    envs[i]->release(envRel);
  }
  if (!announce) {
    showCurrentParameterPage("Amp Decay", String(ampDecaystr) + " mS");
  }
  midiCCOut(CCvca_decay, (mux26 >> 3), 1);
}

FLASHMEM void updateSustain() {
  //envSus = (float)mux22 / 100;
  for (int i = 0; i < 12; i++) {
    envs[i]->sustain(envSus);
  }
  if (!announce) {
    showCurrentParameterPage("Amp Sustain", String(map(mux22, 0, 1023, 0, 100)));
  }
  midiCCOut(CCvca_sustain, (mux22 >> 3), 1);
}

FLASHMEM void updateLFOAmount() {

  lfoA1.amplitude(lfoAamp);

  if (!announce) {
    showCurrentParameterPage("LFO Amount", String(mux3 >> 3));
  }
  midiCCOut(CCmodulation, (mux3 >> 3), 1);
}

FLASHMEM void updateATAmount() {

  lfoA1.amplitude(lfoAamp);

  if (!announce) {
    showCurrentParameterPage("AT Depth", String(mux3 >> 3));
  }
  midiCCOut(CCmodulation, (mux3 >> 3), 1);
}

FLASHMEM void updateMWAmount() {

  lfoA1.amplitude(lfoAamp);

  if (!announce) {
    showCurrentParameterPage("MW Depth", String(mux3 >> 3));
  }
  midiCCOut(CCmodulation, (mux3 >> 3), 1);
}

FLASHMEM void updateLFOFreq() {
  //lfoAfreq = 20 * (float)mux4 / 1024 + 0.1;
  lfoA1.frequency(lfoAfreq);
  if (!announce) {
    showCurrentParameterPage("LFO Rate", String(LFOFreqstr) + " Hz");
  }
  midiCCOut(CClfo_frequency, (mux4 >> 3), 1);
}

FLASHMEM void updateLFOAttack() {
  //lfoAdel = 2000 * (float)mux5 / 1024;
  //lfoAatt = 3000 * (float)mux5 / 1024;
  for (int i = 0; i < 12; i++) {
    lfoAEnvs[i]->delay(lfoAdel);
    lfoAEnvs[i]->attack(lfoAatt);
  }
  if (!announce) {
    showCurrentParameterPage("LFO Attack", String(lfoAttackstr) + " mS");
  }
  midiCCOut(CClfo_attack, (mux5 >> 3), 1);
}

FLASHMEM void updateLFODecay() {
  //lfoAdec = 4000 * (float)mux6 / 1024;
  //lfoArel = 4000 * (float)mux6 / 1024;
  for (int i = 0; i < 12; i++) {
    lfoAEnvs[i]->decay(lfoAdec);
    lfoAEnvs[i]->release(lfoArel);
  }
  if (!announce) {
    showCurrentParameterPage("LFO Decay", String(lfoDecaystr) + " mS");
  }
  midiCCOut(CClfo_decay, (mux6 >> 3), 1);
}

FLASHMEM void updateLFOSustain() {
  //lfoAsus = (float)mux7 / 1024;
  for (int i = 0; i < 12; i++) {
    lfoAEnvs[i]->sustain(lfoAsus);
  }
  if (!announce) {
    showCurrentParameterPage("LFO Sustain", String(map(mux7, 0, 1023, 0, 100)));
  }
  midiCCOut(CClfo_sustain, (mux7 >> 3), 1);
}

FLASHMEM void updateLFODestination() {
  if (LFOA_DEST_1 > 511) {  //lfo - pitch
    lfoAdest = 0;
    if (!announce) {
      showCurrentParameterPage("LFO Dest", "Pitch");
    }
    midiCCOut(CClfo_dest_1, 0, 1);
  } else if (LFOA_DEST_1 < 511 && LFOA_DEST_2 < 511) {  //lfo - filter
    lfoAdest = 1;
    if (!announce) {
      showCurrentParameterPage("LFO Dest", "Filter");
    }
    midiCCOut(CClfo_dest_1, 63, 1);
  } else if (LFOA_DEST_2 > 511) {  //lfo - amp
    lfoAdest = 2;
    if (!announce) {
      showCurrentParameterPage("LFO Dest", "Amplifier");
    }
    midiCCOut(CClfo_dest_1, 127, 1);
  }
  //LFO A DESTINATION
  if (lfoAdest == 0) {  //lfo - pitch
    
    patchCord44.connect();     //lfoAenv1, 0, modMix1
    patchCord45.connect();     //lfoAenv1, 0, vcoB1
    patchCord46.connect();     //lfoAenv1, 0, vcoC1
    patchCord47.connect();     //lfoAenv1, 0, sub1
    patchCord48.disconnect();  //lfoAenv1, 0, filterMix1

    patchCord19.connect();     //lfoAenv2, 0, modMix2
    patchCord20.connect();     //lfoAenv2, 0, vcoB2
    patchCord21.connect();     //lfoAenv2, 0, vcoC2
    patchCord22.connect();     //lfoAenv2, 0, sub2
    patchCord23.disconnect();  //lfoAenv2, 0, filterMix2

    patchCord24.connect();     //lfoAenv3, 0, modMix3
    patchCord25.connect();     //lfoAenv3, 0, vcoB3
    patchCord26.connect();     //lfoAenv3, 0, vcoC3
    patchCord27.connect();     //lfoAenv3, 0, sub3
    patchCord28.disconnect();  //lfoAenv3, 0, filterMix3

    patchCord29.connect();     //lfoAenv4, 0, modMix4
    patchCord30.connect();     //lfoAenv4, 0, vcoB4
    patchCord31.connect();     //lfoAenv4, 0, vcoC4
    patchCord32.connect();     //lfoAenv4, 0, sub4
    patchCord33.disconnect();  //lfoAenv4, 0, filterMix4

    patchCord34.connect();     //lfoAenv5, 0, modMix5
    patchCord35.connect();     //lfoAenv5, 0, vcoB5
    patchCord36.connect();     //lfoAenv5, 0, vcoC5
    patchCord37.connect();     //lfoAenv5, 0, sub5
    patchCord38.disconnect();  //lfoAenv5, 0, filterMix5

    patchCord39.connect();     //lfoAenv6, 0, modMix6
    patchCord40.connect();     //lfoAenv6, 0, vcoB6
    patchCord41.connect();     //lfoAenv6, 0, vcoC6
    patchCord42.connect();     //lfoAenv6, 0, sub6
    patchCord43.disconnect();  //lfoAenv6, 0, filterMix6

    patchCord1019.connect();     //lfoAenv7, 0, modMix7
    patchCord1020.connect();     //lfoAenv7, 0, vcoB7
    patchCord1021.connect();     //lfoAenv7, 0, vcoC7
    patchCord1022.connect();     //lfoAenv7, 0, sub7
    patchCord1023.disconnect();  //lfoAenv7, 0, filterMix7

    patchCord1024.connect();     //lfoAenv8, 0, modMix8
    patchCord1025.connect();     //lfoAenv8, 0, vcoB8
    patchCord1025.connect();     //lfoAenv8, 0, vcoC8
    patchCord1027.connect();     //lfoAenv8, 0, sub8
    patchCord1028.disconnect();  //lfoAenv8, 0, filterMix8

    patchCord1029.connect();     //lfoAenv9, 0, modMix9
    patchCord1030.connect();     //lfoAenv9, 0, vcoB9
    patchCord1031.connect();     //lfoAenv9, 0, vcoC9
    patchCord1032.connect();     //lfoAenv9, 0, sub9
    patchCord1033.disconnect();  //lfoAenv9, 0, filterMix9

    patchCord1034.connect();     //lfoAenv10, 0, modMix10
    patchCord1035.connect();     //lfoAenv10, 0, vcoB10
    patchCord1036.connect();     //lfoAenv10, 0, vcoC10
    patchCord1037.connect();     //lfoAenv10, 0, sub10
    patchCord1038.disconnect();  //lfoAenv10, 0, filterMix10

    patchCord1039.connect();     //lfoAenv11, 0, modMix11
    patchCord1040.connect();     //lfoAenv11, 0, vcoB11
    patchCord1041.connect();     //lfoAenv11, 0, vcoC11
    patchCord1042.connect();     //lfoAenv11, 0, sub11
    patchCord1043.disconnect();  //lfoAenv11, 0, filterMix11

    patchCord1044.connect();     //lfoAenv12, 0, modMix12
    patchCord1045.connect();     //lfoAenv12, 0, vcoB12
    patchCord1046.connect();     //lfoAenv12, 0, vcoC12
    patchCord1047.connect();     //lfoAenv12, 0, sub12
    patchCord1048.disconnect();  //lfoAenv12, 0, filterMix12

    patchCordLFOQueue.disconnect();
  }
  if (lfoAdest == 1) {  //lfo - filter
    patchCord44.disconnect();  //lfoAenv1, 0, modMix1
    patchCord45.disconnect();  //lfoAenv1, 0, vcoB1
    patchCord46.disconnect();  //lfoAenv1, 0, vcoC1
    patchCord47.disconnect();  //lfoAenv1, 0, sub1
    patchCord48.connect();     //lfoAenv1, 0, filterMix1

    patchCord19.disconnect();  //lfoAenv2, 0, modMix2
    patchCord20.disconnect();  //lfoAenv2, 0, vcoB2
    patchCord21.disconnect();  //lfoAenv2, 0, vcoC2
    patchCord22.disconnect();  //lfoAenv2, 0, sub2
    patchCord23.connect();     //lfoAenv2, 0, filterMix2

    patchCord24.disconnect();  //lfoAenv3, 0, modMix3
    patchCord25.disconnect();  //lfoAenv3, 0, vcoB3
    patchCord26.disconnect();  //lfoAenv3, 0, vcoC3
    patchCord27.disconnect();  //lfoAenv3, 0, sub3
    patchCord28.connect();     //lfoAenv3, 0, filterMix3

    patchCord29.disconnect();  //lfoAenv4, 0, modMix4
    patchCord30.disconnect();  //lfoAenv4, 0, vcoB4
    patchCord31.disconnect();  //lfoAenv4, 0, vcoC4
    patchCord32.disconnect();  //lfoAenv4, 0, sub4
    patchCord33.connect();     //lfoAenv4, 0, filterMix4

    patchCord34.disconnect();  //lfoAenv5, 0, modMix5
    patchCord35.disconnect();  //lfoAenv5, 0, vcoB5
    patchCord36.disconnect();  //lfoAenv5, 0, vcoC5
    patchCord37.disconnect();  //lfoAenv5, 0, sub5
    patchCord38.connect();     //lfoAenv5, 0, filterMix5

    patchCord39.disconnect();  //lfoAenv6, 0, modMix6
    patchCord40.disconnect();  //lfoAenv6, 0, vcoB6
    patchCord41.disconnect();  //lfoAenv6, 0, vcoC6
    patchCord42.disconnect();  //lfoAenv6, 0, sub6
    patchCord43.connect();     //lfoAenv6, 0, filterMix6

    patchCord1019.disconnect();  //lfoAenv7, 0, modMix7
    patchCord1020.disconnect();  //lfoAenv7, 0, vcoB7
    patchCord1021.disconnect();  //lfoAenv7, 0, vcoC7
    patchCord1022.disconnect();  //lfoAenv7, 0, sub7
    patchCord1023.connect();     //lfoAenv7, 0, filterMix7

    patchCord1024.disconnect();  //lfoAenv8, 0, modMix8
    patchCord1025.disconnect();  //lfoAenv8, 0, vcoB8
    patchCord1025.disconnect();  //lfoAenv8, 0, vcoC8
    patchCord1027.disconnect();  //lfoAenv8, 0, sub8
    patchCord1028.connect();     //lfoAenv8, 0, filterMix8

    patchCord1029.disconnect();  //lfoAenv9, 0, modMix9
    patchCord1030.disconnect();  //lfoAenv9, 0, vcoB9
    patchCord1031.disconnect();  //lfoAenv9, 0, vcoC9
    patchCord1032.disconnect();  //lfoAenv9, 0, sub9
    patchCord1033.connect();     //lfoAenv9, 0, filterMix9

    patchCord1034.disconnect();  //lfoAenv10, 0, modMix10
    patchCord1035.disconnect();  //lfoAenv10, 0, vcoB10
    patchCord1036.disconnect();  //lfoAenv10, 0, vcoC10
    patchCord1037.disconnect();  //lfoAenv10, 0, sub10
    patchCord1038.connect();     //lfoAenv10, 0, filterMix10

    patchCord1039.disconnect();  //lfoAenv11, 0, modMix11
    patchCord1040.disconnect();  //lfoAenv11, 0, vcoB11
    patchCord1041.disconnect();  //lfoAenv11, 0, vcoC11
    patchCord1042.disconnect();  //lfoAenv11, 0, sub11
    patchCord1043.connect();     //lfoAenv11, 0, filterMix11

    patchCord1044.disconnect();  //lfoAenv12, 0, modMix12
    patchCord1045.disconnect();  //lfoAenv12, 0, vcoB12
    patchCord1046.disconnect();  //lfoAenv12, 0, vcoC12
    patchCord1047.disconnect();  //lfoAenv12, 0, sub12
    patchCord1048.connect();     //lfoAenv12, 0, filterMix12

    patchCordLFOQueue.disconnect();
  }
  if (lfoAdest == 2) {  //lfo - amp
    patchCord44.disconnect();  //lfoAenv1, 0, modMix1
    patchCord45.disconnect();  //lfoAenv1, 0, vcoB1
    patchCord46.disconnect();  //lfoAenv1, 0, vcoC1
    patchCord47.disconnect();  //lfoAenv1, 0, sub1
    patchCord48.disconnect();  //lfoAenv1, 0, filterMix1

    patchCord19.disconnect();  //lfoAenv2, 0, modMix2
    patchCord20.disconnect();  //lfoAenv2, 0, vcoB2
    patchCord21.disconnect();  //lfoAenv2, 0, vcoC2
    patchCord22.disconnect();  //lfoAenv2, 0, sub2
    patchCord23.disconnect();  //lfoAenv2, 0, filterMix2

    patchCord24.disconnect();  //lfoAenv3, 0, modMix3
    patchCord25.disconnect();  //lfoAenv3, 0, vcoB3
    patchCord26.disconnect();  //lfoAenv3, 0, vcoC3
    patchCord27.disconnect();  //lfoAenv3, 0, sub3
    patchCord28.disconnect();  //lfoAenv3, 0, filterMix3

    patchCord29.disconnect();  //lfoAenv4, 0, modMix4
    patchCord30.disconnect();  //lfoAenv4, 0, vcoB4
    patchCord31.disconnect();  //lfoAenv4, 0, vcoC4
    patchCord32.disconnect();  //lfoAenv4, 0, sub4
    patchCord33.disconnect();  //lfoAenv4, 0, filterMix4

    patchCord34.disconnect();  //lfoAenv5, 0, modMix5
    patchCord35.disconnect();  //lfoAenv5, 0, vcoB5
    patchCord36.disconnect();  //lfoAenv5, 0, vcoC5
    patchCord37.disconnect();  //lfoAenv5, 0, sub5
    patchCord38.disconnect();  //lfoAenv5, 0, filterMix5

    patchCord39.disconnect();  //lfoAenv6, 0, modMix6
    patchCord40.disconnect();  //lfoAenv6, 0, vcoB6
    patchCord41.disconnect();  //lfoAenv6, 0, vcoC6
    patchCord42.disconnect();  //lfoAenv6, 0, sub6
    patchCord43.disconnect();  //lfoAenv6, 0, filterMix6

    patchCord1019.disconnect();  //lfoAenv7, 0, modMix7
    patchCord1020.disconnect();  //lfoAenv7, 0, vcoB7
    patchCord1021.disconnect();  //lfoAenv7, 0, vcoC7
    patchCord1022.disconnect();  //lfoAenv7, 0, sub7
    patchCord1023.disconnect();  //lfoAenv7, 0, filterMix7

    patchCord1024.disconnect();  //lfoAenv8, 0, modMix8
    patchCord1025.disconnect();  //lfoAenv8, 0, vcoB8
    patchCord1025.disconnect();  //lfoAenv8, 0, vcoC8
    patchCord1027.disconnect();  //lfoAenv8, 0, sub8
    patchCord1028.disconnect();  //lfoAenv8, 0, filterMix8

    patchCord1029.disconnect();  //lfoAenv9, 0, modMix9
    patchCord1030.disconnect();  //lfoAenv9, 0, vcoB9
    patchCord1031.disconnect();  //lfoAenv9, 0, vcoC9
    patchCord1032.disconnect();  //lfoAenv9, 0, sub9
    patchCord1033.disconnect();  //lfoAenv9, 0, filterMix9

    patchCord1034.disconnect();  //lfoAenv10, 0, modMix10
    patchCord1035.disconnect();  //lfoAenv10, 0, vcoB10
    patchCord1036.disconnect();  //lfoAenv10, 0, vcoC10
    patchCord1037.disconnect();  //lfoAenv10, 0, sub10
    patchCord1038.disconnect();  //lfoAenv10, 0, filterMix10

    patchCord1039.disconnect();  //lfoAenv11, 0, modMix11
    patchCord1040.disconnect();  //lfoAenv11, 0, vcoB11
    patchCord1041.disconnect();  //lfoAenv11, 0, vcoC11
    patchCord1042.disconnect();  //lfoAenv11, 0, sub11
    patchCord1043.disconnect();  //lfoAenv11, 0, filterMix11

    patchCord1044.disconnect();  //lfoAenv12, 0, modMix12
    patchCord1045.disconnect();  //lfoAenv12, 0, vcoB12
    patchCord1046.disconnect();  //lfoAenv12, 0, vcoC12
    patchCord1047.disconnect();  //lfoAenv12, 0, sub12
    patchCord1048.disconnect();  //lfoAenv12, 0, filterMix12

    patchCordLFOQueue.connect();
  }
}

//lfoA shape
FLASHMEM void updateLFOShape() {
  if (LFOA_SHAPE_1 > 511) {
    lfoAshape = 0;
    if (!announce) {
      showCurrentParameterPage("LFO Wave", "Triangle");
    }
    midiCCOut(CClfo_wave_1, 0, 1);
  } else if (LFOA_SHAPE_1 < 511 && LFOA_SHAPE_2 < 511) {
    lfoAshape = 1;
    if (!announce) {
      showCurrentParameterPage("LFO Wave", "Saw");
    }
    midiCCOut(CClfo_wave_1, 63, 1);
  } else if (LFOA_SHAPE_2 > 511) {
    lfoAshape = 2;
    if (!announce) {
      showCurrentParameterPage("LFO Wave", "S & H");
    }
    midiCCOut(CClfo_wave_1, 127, 1);
  }
  //lfo shape switch
  if (lfoAshape == 0) {
    lfoA1.begin(WAVEFORM_SINE);
  } else if (lfoAshape == 1) {
    lfoA1.begin(WAVEFORM_SAWTOOTH_REVERSE);

  } else if (lfoAshape == 2) {
    lfoA1.begin(WAVEFORM_SAMPLE_HOLD);
  }
}

FLASHMEM void updatePWAmount() {
  //lfoBamp = (float)mux15 / 1023;
  lfoB1.amplitude(lfoBamp);
  if (!announce) {
    showCurrentParameterPage("PWM Depth", String(mux15 >> 3));
  }
  midiCCOut(CCoscpwm, (mux15 >> 3), 1);
}

FLASHMEM void updatePWFreq() {
  //lfoBfreq = 5 * (float)mux16 / 1023 + 0.1;
  lfoB1.frequency(lfoBfreq);
  if (!announce) {
    showCurrentParameterPage("PWM Rate", String(PWMFreqstr) + " Hz");
  }
  midiCCOut(CCoscpwmrate, (mux16 >> 3), 1);
}

//Delay
FLASHMEM void updateDelayMix() {
  // dlyAmt = (float)mux21 / 1100 - 0.1;
  // if (dlyAmt < 0) {
  //   dlyAmt = 0;
  // }
  //delay
  dlyMixL.gain(1, dlyAmt * 0.9);
  dlyMixR.gain(1, (dlyAmt / 1.4) * 0.9);
  if (!announce) {
    showCurrentParameterPage("Delay Amount", String(mux21 >> 3));
  }
  midiCCOut(CCdly_amt, (mux21 >> 3), 1);
}

FLASHMEM void updateDelayTime() {
  //dlyTimeL = mux20 / 2.5;
  //dlyTimeR = mux20 / 1.25;
  dlyL.delay(0, dlyTimeL);
  dlyR.delay(0, dlyTimeR);
  if (!announce) {
    showCurrentParameterPage("Delay Time", String(mux20 >> 3));
  }
  midiCCOut(CCdly_size, (mux20 >> 3), 1);
}

//Reverb
FLASHMEM void updateReverbMix() {
  //revMix = ((float)mux18 / 1024 / 1.2);
  //reverb
  fxL.gain(1, revMix);
  fxR.gain(1, revMix);
  //output gain reduction
  fxL.gain(0, outGain - revMix / 1.6);
  fxL.gain(2, outGain - revMix / 1.6);

  fxR.gain(0, outGain - revMix / 1.6);
  fxR.gain(2, outGain - revMix / 1.6);
  if (!announce) {
    showCurrentParameterPage("Reverb Mix", String(mux18 >> 3));
  }
  midiCCOut(CCrev_amt, (mux18 >> 3), 1);
}

FLASHMEM void updateReverbSize() {
  //revSize = ((float)mux19 / 1024 - 0.01);
  reverb.roomsize(revSize);
  if (!announce) {
    showCurrentParameterPage("Reverb Size", String(mux19 >> 3));
  }
  midiCCOut(CCrev_size, (mux19 >> 3), 1);
}

void updatePatchname() {
  showPatchPage(String(patchNo), patchName);
}


void myControlChange(byte channel, byte control, byte value) {
  switch (control) {

    case CCmodwheel:
      {
        switch (modWheelDepth) {
          case 0: mwScale = 0.0f; break;
          case 1: mwScale = 1.0f / 5.0f; break;
          case 2: mwScale = 1.0f / 4.0f; break;
          case 3: mwScale = 1.0f / 3.5f; break;
          case 4: mwScale = 1.0f / 3.0f; break;
          case 5: mwScale = 1.0f / 2.5f; break;
          case 6: mwScale = 1.0f / 2.0f; break;
          case 7: mwScale = 1.0f / 1.75f; break;
          case 8: mwScale = 1.0f / 1.5f; break;
          case 9: mwScale = 1.0f / 1.25f; break;
          case 10: mwScale = 1.0f; break;
          default: mwScale = 0.0f; break;
        }

        // Compute modulation
        float midiMod = (value << 3) * mwScale;

        // Apply modulation
        int oldmux3 = mux3;
        mux3 = mux3 + midiMod;
        if (mux3 > 1023) mux3 = 1023;

        if (lfoAdest == 0 && lfoAshape != 2) {
          lfoAamp = ((float)mux3) / 1024 / 10;
        } else if (lfoAdest == 1 && lfoAshape != 2) {
          lfoAamp = ((float)mux3) / 1024 / 3;
        } else {
          lfoAamp = ((float)mux3) / 1024;
        }
        mux3 = oldmux3;
        break;
      }
      // restore base control value so modulation is temporary
      mux3 = oldmux3;
      break;

    case CCvolumeControl:
      mux23 = (value << 3);
      updateVolume();
      break;

    case CCvcf_frequency:
      mux25 = (value << 3);
      cutoffstr = FILTERFREQS[value];
      cut = 15000 * (float)mux25 / 1023 + 15;  /////cut
      updateCutoff();
      break;

    case CCvcf_resonance:
      mux24 = (value << 3);
      res = 4.5 * (float)mux24 / 1023 + 1.1;
      updateRes();
      break;

    case CCvcf_attack:
      mux0 = (value << 3);
      filterAttackstr = ENVTIMES[value];
      updateFilterAttack();
      break;

    case CCvcf_decay:
      mux1 = (value << 3);
      filterDecaystr = ENVTIMES[value];
      updateFilterDecay();
      break;

    case CCvcf_env_amount:
      mux2 = (value << 3);
      updateFilterAmount();
      break;

    case CClfo_frequency:
      mux4 = (value << 3);
      LFOFreqstr = LFOTEMPO[value];
      updateLFOFreq();
      break;

    case CCmodulation:
      mux3 = (value << 3);
      updateLFOAmount();
      break;

    case CClfo_attack:
      mux5 = (value << 3);
      lfoAttackstr = ENVTIMES[value];
      updateLFOAttack();
      break;

    case CClfo_decay:
      mux6 = (value << 3);
      lfoDecaystr = ENVTIMES[value];
      updateLFODecay();
      break;

    case CClfo_sustain:
      mux7 = (value << 3);
      updateLFOSustain();
      break;

    case CCvca_attack:
      mux27 = (value << 3);
      ampAttackstr = ENVTIMES[value];
      updateAttack();
      break;

    case CCvca_decay:
      mux26 = (value << 3);
      ampDecaystr = ENVTIMES[value];
      updateDecay();
      break;

    case CCvca_sustain:
      mux22 = (value << 3);
      updateSustain();
      break;

    case CCoscpwmrate:
      mux16 = (value << 3);
      PWMFreqstr = LFOTEMPO[value];
      updatePWFreq();
      break;

    case CCoscpwm:
      mux15 = (value << 3);
      updatePWAmount();
      break;

    case CCosc_B_freq:
      mux13 = (value << 3);
      updateTuneB();
      break;

    case CCosc_C_freq:
      mux12 = (value << 3);
      updateTuneC();
      break;

    case CCosc_A_vol:
      mux10 = (value << 3);
      updateVolA();
      break;

    case CCosc_B_vol:
      mux9 = (value << 3);
      updateVolB();
      break;

    case CCosc_C_vol:
      mux8 = (value << 3);
      updateVolC();
      break;

    case CCosc_Subvol:
      mux17 = (value << 3);
      updateSubVol();
      break;

    case CCosc_C_shape:
      mux11 = (value << 3);
      updateShapeC();
      break;

    case CCcrossmod:
      mux14 = (value << 3);
      updateCrossMod();
      break;

    case CCrev_size:
      mux19 = (value << 3);
      updateReverbSize();
      break;

    case CCrev_amt:
      mux18 = (value << 3);
      updateReverbMix();
      break;

    case CCdly_size:
      mux21 = (value << 3);
      updateDelayTime();
      break;

    case CCdly_amt:
      mux20 = (value << 3);
      updateDelayMix();
  }
}

void myAfterTouch(byte channel, byte value) {

  switch (afterTouchDepth) {
    case 0: atScale = 0.0f; break;
    case 1: atScale = 1.0f / 5.0f; break;
    case 2: atScale = 1.0f / 4.0f; break;
    case 3: atScale = 1.0f / 3.5f; break;
    case 4: atScale = 1.0f / 3.0f; break;
    case 5: atScale = 1.0f / 2.5f; break;
    case 6: atScale = 1.0f / 2.0f; break;
    case 7: atScale = 1.0f / 1.75f; break;
    case 8: atScale = 1.0f / 1.5f; break;
    case 9: atScale = 1.0f / 1.25f; break;
    case 10: atScale = 1.0f; break;
  }

  // Scale the aftertouch value (0–127) to modulation offset
  midiMod = (value << 3) * atScale;  // ≈ 0–1024 range

  // Combine base mux3 (from knob) + aftertouch offset
  int combined = constrain((int)mux3 + midiMod, 0, 1023);

  // Compute the *effective modulation amount* based on LFO dest
  float effectiveLFOAmp;
  if (lfoAdest == 0 && lfoAshape != 2) {
    effectiveLFOAmp = (float)combined / 1024.0f / 10.0f;
  } else if (lfoAdest == 1 && lfoAshape != 2) {
    effectiveLFOAmp = (float)combined / 1024.0f / 3.0f;
  } else {
    effectiveLFOAmp = (float)combined / 1024.0f;
  }

  lfoAamp = effectiveLFOAmp;
}

void myPitchBend(byte channel, int pitch) {
  switch (pitchBendRange) {
    case 0:
      break;

    case 1:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1, 1.06);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.06) - 0.06);
      }
      break;

    case 2:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1, 1.12);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.11) - 0.11);
      }
      break;

    case 3:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 1.19);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.16) - 0.16);
      }
      break;

    case 4:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 1.26);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.21) - 0.21);
      }
      break;

    case 5:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 1.33);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.25) - 0.25);
      }
      break;

    case 6:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 1.42);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.29) - 0.29);
      }
      break;

    case 7:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 1.50);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.33) - 0.33);
      }
      break;

    case 8:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 1.58);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.37) - 0.37);
      }
      break;

    case 9:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 1.68);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.000, 1.405) - 0.405);
      }

      break;

    case 10:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 1.79);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.44) - 0.44);
      }
      break;

    case 11:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 1.885);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.47) - 0.47);
      }
      break;

    case 12:
      newpitchbend = (pitch + 8192);
      if (newpitchbend > 8192) {
        bend = map(newpitchbend, 8193, 16383, 1.00, 2.00);
      }
      if (newpitchbend < 8193) {
        bend = (map(newpitchbend, 0, 8192, 1.00, 1.50) - 0.50);
      }
      break;
  }
}

void myProgramChange(byte channel, byte program) {
  state = PATCH;
  patchNo = program + 1;
  recallPatch(patchNo);
  Serial.print("MIDI Pgm Change:");
  Serial.println(patchNo);
  state = PARAMETER;
}

void recallPatch(int patchNo) {
  allNotesOff();
  announce = true;
  File patchFile = SD.open(String(patchNo).c_str());
  if (!patchFile) {
    Serial.println("File not found");
  } else {
    String data[NO_OF_PARAMS];  //Array of data read in
    recallPatchData(patchFile, data);
    setCurrentPatchData(data);
    patchFile.close();
  }
  announce = false;
}

FLASHMEM void setCurrentPatchData(String data[]) {
  patchName = data[0];
  octave = data[1].toFloat();
  octaveB = data[2].toFloat();
  octaveC = data[3].toFloat();
  shapeA = data[4].toInt();
  shapeB = data[5].toInt();
  shapeC = data[6].toInt();
  tuneB = data[7].toFloat();
  tuneC = data[8].toFloat();
  crossMod = data[9].toFloat();
  vcoAvol = data[10].toFloat();
  vcoBvol = data[11].toFloat();
  vcoCvol = data[12].toFloat();
  Subvol = data[13].toFloat();
  cut = data[14].toInt();
  res = data[15].toFloat();
  filtAtt = data[16].toInt();
  filtDec = data[17].toInt();
  filtAmt = data[18].toFloat();
  FILTER_MODE = data[19].toInt();
  envAtt = data[20].toInt();
  envDec = data[21].toInt();
  envRel = data[22].toInt();
  envSus = data[23].toFloat();
  lfoAamp = data[24].toFloat();
  lfoAfreq = data[25].toFloat();
  lfoAdel = data[26].toInt();
  lfoAatt = data[27].toInt();
  lfoAdec = data[28].toInt();
  lfoArel = data[29].toInt();
  lfoAsus = data[30].toFloat();
  lfoBamp = data[31].toFloat();
  lfoBfreq = data[32].toFloat();
  dlyAmt = data[33].toFloat();
  dlyTimeL = data[34].toFloat();
  dlyTimeR = data[35].toFloat();
  revMix = data[36].toFloat();
  revSize = data[37].toFloat();
  LFOA_DEST_1 = data[38].toInt();
  LFOA_SHAPE_1 = data[39].toInt();
  modWheelDepth = data[40].toInt();
  pitchBendRange = data[41].toInt();
  afterTouchDepth = data[42].toInt();
  NP = data[43].toInt();
  unidetune = data[44].toInt();
  MONO_POLY_1 = data[45].toInt();
  MONO_POLY_2 = data[46].toInt();
  LFOA_DEST_2 = data[47].toInt();
  LFOA_SHAPE_2 = data[48].toInt();

  updateVolume();
  updateCrossMod();
  updateShapeA();
  updateShapeB();
  updateShapeC();
  updateVolA();
  updateVolB();
  updateVolC();
  updateTuneB();
  updateTuneC();
  updateSubVol();
  updateFilterMode();
  updateCutoff();
  updateRes();
  updateFilterAttack();
  updateFilterDecay();
  updateFilterAmount();
  updateLFOFreq();
  updateLFOAttack();
  updateLFODecay();
  updateLFOSustain();
  updateLFOShape();
  updateLFOAmount();
  updateLFODestination();
  updatePWFreq();
  updatePWAmount();
  updateAttack();
  updateDecay();
  updateSustain();
  updateDelayTime();
  updateDelayMix();
  updateReverbMix();
  updateReverbSize();

  //Patchname
  updatePatchname();


  Serial.print("Set Patch: ");
  Serial.println(patchName);
}

FLASHMEM String getCurrentPatchData() {
  return patchName + "," + String(octave) + "," + String(octaveB) + "," + String(octaveC) + "," + String(shapeA) + "," + String(shapeB) + "," + String(shapeC) + "," + String(tuneB, 4) + "," + String(tuneC, 4)
         + "," + String(crossMod) + "," + String(vcoAvol) + "," + String(vcoBvol) + "," + String(vcoCvol) + "," + String(Subvol) + "," + String(cut) + "," + String(res) + "," + String(filtAtt) + "," + String(filtDec)
         + "," + String(filtAmt) + "," + String(FILTER_MODE) + "," + String(envAtt) + "," + String(envDec) + "," + String(envRel) + "," + String(envSus) + "," + String(lfoAamp) + "," + String(lfoAfreq)
         + "," + String(lfoAdel) + "," + String(lfoAatt) + "," + String(lfoAdec) + "," + String(lfoArel) + "," + String(lfoAsus) + "," + String(lfoBamp) + "," + String(lfoBfreq) + "," + String(dlyAmt)
         + "," + String(dlyTimeL) + "," + String(dlyTimeR) + "," + String(revMix) + "," + String(revSize) + "," + String(LFOA_DEST_1) + "," + String(LFOA_SHAPE_1) + "," + String(modWheelDepth) + "," + String(pitchBendRange)
         + "," + String(afterTouchDepth) + "," + String(NP) + "," + String(unidetune) + "," + String(MONO_POLY_1) + "," + String(MONO_POLY_2) + "," + String(LFOA_DEST_2) + "," + String(LFOA_SHAPE_2);
}

FLASHMEM void checkMux() {

  mux1Read = adc->adc1->analogRead(muxPots1);
  mux2Read = adc->adc1->analogRead(muxPots2);
  mux3Read = adc->adc1->analogRead(muxPots3);
  mux4Read = adc->adc1->analogRead(muxPots4);
  mux5Read = adc->adc1->analogRead(muxPots5);
  mux6Read = adc->adc1->analogRead(muxPots6);

  if (mux1Read > (mux1ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux1Read < (mux1ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux1ValuesPrev[muxInput] = mux1Read;

    switch (muxInput) {
      case 0:
        mux0 = mux1Read;
        filterAttackstr = ENVTIMES[mux0 / 8];
        filtAtt = (3000 * (float)mux0 / 1023);
        updateFilterAttack();
        break;
      case 1:
        mux1 = mux1Read;
        filtDec = (3000 * (float)mux1 / 1023);
        filterDecaystr = ENVTIMES[mux1 / 8];
        updateFilterDecay();
        break;
      case 2:
        mux2 = mux1Read;
        filtAmt = (float)mux2 / 512 - 1;
        updateFilterAmount();
        break;
      case 3:
        mux3 = mux1Read;
        if (lfoAdest == 0 && lfoAshape != 2) {
          lfoAamp = ((float)mux3) / 1024 / 10;
        } else if (lfoAdest == 1 && lfoAshape != 2) {
          lfoAamp = ((float)mux3) / 1024 / 3;
        } else {
          lfoAamp = ((float)mux3) / 1024;
        }
        updateLFOAmount();
        break;
      case 4:
        mux4 = mux1Read;
        lfoAfreq = 20 * (float)mux4 / 1024 + 0.1;
        LFOFreqstr = LFOTEMPO[mux4 / 8];
        updateLFOFreq();
        break;
      case 5:
        mux5 = mux1Read;
        lfoAdel = 2000 * (float)mux5 / 1024;
        lfoAatt = 3000 * (float)mux5 / 1024;
        lfoAttackstr = ENVTIMES[mux5 / 8];
        updateLFOAttack();
        break;
      case 6:
        mux6 = mux1Read;
        lfoAdec = 4000 * (float)mux6 / 1024;
        lfoArel = 4000 * (float)mux6 / 1024;
        lfoDecaystr = ENVTIMES[mux6 / 8];
        updateLFODecay();
        break;
      case 7:
        mux7 = mux1Read;
        lfoAsus = (float)mux7 / 1024;
        updateLFOSustain();
        break;
    }
  }

  if (mux2Read > (mux2ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux2Read < (mux2ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux2ValuesPrev[muxInput] = mux2Read;

    switch (muxInput) {
      case 0:
        mux8 = mux2Read;
        vcoCvol = (float)mux8 / 1023;
        updateVolC();
        break;
      case 1:
        mux9 = mux2Read;
        vcoBvol = (float)mux9 / 1023;
        updateVolB();
        break;
      case 2:
        mux10 = mux2Read;
        vcoAvol = (float)mux10 / 1023;
        updateVolA();
        break;
      case 3:
        mux11 = mux2Read;
        updateShapeC();
        break;
      case 4:
        mux12 = mux2Read;
        if (mux12 < 512) {
          tuneC = ((float)mux12 / 1023) + 0.5;
        } else {
          tuneC = ((float)mux12 / 510);
        }
        tuneC = roundf(tuneC * 10000.0f) / 10000.0f;
        updateTuneC();
        break;
      case 5:
        mux13 = mux2Read;
        if (mux13 < 512) {
          tuneB = ((float)mux13 / 1023) + 0.5;
        } else {
          tuneB = ((float)mux13 / 510);
        }
        tuneB = roundf(tuneB * 10000.0f) / 10000.0f;
        updateTuneB();
        break;
      case 6:
        mux14 = mux2Read;
        crossMod = (float)mux14 / 512;
        updateCrossMod();
        break;
      case 7:
        mux15 = mux2Read;
        lfoBamp = (float)mux15 / 1023;
        updatePWAmount();
        break;
    }
  }

  if (mux3Read > (mux3ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux3Read < (mux3ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux3ValuesPrev[muxInput] = mux3Read;

    switch (muxInput) {
      case 0:
        mux16 = mux3Read;
        lfoBfreq = 5 * (float)mux16 / 1023 + 0.1;
        PWMFreqstr = LFOTEMPO[mux16 / 8];
        updatePWFreq();
        break;
      case 1:
        mux17 = mux3Read;
        Subvol = (float)mux17 / 1023;
        updateSubVol();
        break;
      case 2:
        mux18 = mux3Read;
        revMix = ((float)mux18 / 1024 / 1.2);
        updateReverbMix();
        break;
      case 3:
        mux19 = mux3Read;
        revSize = ((float)mux19 / 1024 - 0.01);
        updateReverbSize();
        break;
      case 4:
        mux20 = mux3Read;
        dlyTimeL = mux20 / 2.5;
        dlyTimeR = mux20 / 1.25;
        updateDelayTime();
        break;
      case 5:
        mux21 = mux3Read;
        dlyAmt = (float)mux21 / 1100 - 0.1;
        if (dlyAmt < 0) {
          dlyAmt = 0;
        }
        updateDelayMix();
        break;
      case 6:
        mux22 = mux3Read;
        envSus = (float)mux22 / 100;
        updateSustain();
        break;
      case 7:
        mux23 = mux3Read;
        mainVol = (float)mux23 / 1024;
        updateVolume();
        break;
    }
  }

  if (mux4Read > (mux4ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux4Read < (mux4ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux4ValuesPrev[muxInput] = mux4Read;

    switch (muxInput) {
      case 0:
        mux24 = mux4Read;
        res = 4.5 * (float)mux24 / 1023 + 1.1;
        updateRes();
        break;
      case 1:
        mux25 = mux4Read;
        cutoffstr = FILTERFREQS[mux25 / 8];
        cut = 15000 * (float)mux25 / 1023 + 15;  /////cut
        updateCutoff();
        break;
      case 2:
        mux26 = mux4Read;
        envDec = 5000 * (float)mux26 / 1023;
        envRel = 5000 * (float)mux26 / 1023;
        ampDecaystr = ENVTIMES[mux26 / 8];
        updateDecay();
        break;
      case 3:
        mux27 = mux4Read;
        envAtt = 3000 * (float)mux27 / 1023;
        ampAttackstr = ENVTIMES[mux27 / 8];
        updateAttack();
        break;
      case 4:
        mux28 = mux4Read;
        break;
      case 5:
        mux29 = mux4Read;
        break;
      case 6:
        mux30 = mux4Read;
        break;
      case 7:
        FILTER_MODE = mux4Read;
        updateFilterMode();
        break;
    }
  }

  if (mux5Read > (mux5ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux5Read < (mux5ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux5ValuesPrev[muxInput] = mux5Read;

    switch (muxInput) {
      case 0:
        MONO_POLY_1 = mux5Read;
        updatemonoPoly();
        break;
      case 1:
        A_SHAPE_1 = mux5Read;
        updateShapeA();
        break;
      case 2:
        A_SHAPE_2 = mux5Read;
        updateShapeA();
        break;
      case 3:
        B_SHAPE_1 = mux5Read;
        updateShapeB();
        break;
      case 4:
        B_SHAPE_2 = mux5Read;
        updateShapeB();
        break;
      case 5:
        MAIN_OCT_1 = mux5Read;
        updateMainOctave();
        break;
      case 6:
        MAIN_OCT_2 = mux5Read;
        updateMainOctave();
        break;
      case 7:
        B_OCTAVE_1 = mux5Read;
        updateOctaveB();
        break;
    }
  }

  if (mux6Read > (mux6ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux6Read < (mux6ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux6ValuesPrev[muxInput] = mux6Read;

    switch (muxInput) {
      case 0:
        C_OCTAVE_1 = mux6Read;
        updateOctaveC();
        break;
      case 1:
        C_OCTAVE_2 = mux6Read;
        updateOctaveC();
        break;
      case 2:
        B_OCTAVE_2 = mux6Read;
        updateOctaveB();
        break;
      case 3:
        LFOA_SHAPE_1 = mux6Read;
        updateLFOShape();
        break;
      case 4:
        LFOA_SHAPE_2 = mux6Read;
        updateLFOShape();
        break;
      case 5:
        LFOA_DEST_1 = mux6Read;
        updateLFODestination();
        break;
      case 6:
        LFOA_DEST_2 = mux6Read;
        updateLFODestination();
        break;
      case 7:
        MONO_POLY_2 = mux6Read;
        updatemonoPoly();
        break;
    }
  }

  muxInput++;
  if (muxInput >= MUXCHANNELS) {
    muxInput = 0;
    if (!firstPatchLoaded) {
      recallPatch(patchNo);  // Load first patch after all controls read
      firstPatchLoaded = true;
    }
  }

  digitalWriteFast(MUX1, muxInput & B0001);
  digitalWriteFast(MUX2, muxInput & B0010);
  digitalWriteFast(MUX3, muxInput & B0100);
}

void midiCCOut(byte cc, byte value, byte ccChannel) {
  MIDI.sendControlChange(cc, value, ccChannel);     //MIDI DIN is set to Out
  usbMIDI.sendControlChange(cc, value, ccChannel);  //MIDI DIN is set to Out
}

void midiProgOut(byte chg, byte channel) {
  if (Program == true) {
    if (chg < 113) {
      MIDI.sendProgramChange(chg - 1, channel);     //MIDI DIN is set to Out
      usbMIDI.sendProgramChange(chg - 1, channel);  //MIDI DIN is set to Out
    }
  }
}

void showSettingsPage() {
  showSettingsPage(settings::current_setting(), settings::current_setting_value(), state);
}

void updateEEPromSettings() {

  if (oldunidetune != unidetune) {
    switch (unidetune) {
      case 0:
        detune = 1.000;
        break;
      case 1:
        detune = 1.002;
        break;
      case 2:
        detune = 1.004;
        break;
      case 3:
        detune = 1.006;
        break;
      case 4:
        detune = 1.008;
        break;
      case 5:
        detune = 1.010;
        break;
      case 6:
        detune = 1.012;
        break;
      case 7:
        detune = 1.014;
        break;
      case 8:
        detune = 1.016;
        break;
      case 9:
        detune = 1.018;
        break;
      case 10:
        detune = 1.020;
        break;
    }
    oldunidetune = unidetune;
    olddetune = detune;
  }
}

void checkSwitches() {

  saveButton.update();
  if (saveButton.held()) {
    switch (state) {
      case PARAMETER:
      case PATCH:
        state = DELETE;
        break;
    }
  } else if (saveButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        if (patches.size() < PATCHES_LIMIT) {
          resetPatchesOrdering();  //Reset order of patches from first patch
          patches.push({ patches.size() + 1, INITPATCHNAME });
          state = SAVE;
        }
        break;
      case SAVE:
        //Save as new patch with INITIALPATCH name or overwrite existing keeping name - bypassing patch renaming
        patchName = patches.last().patchName;
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patches.last().patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        break;
      case PATCHNAMING:
        if (renamedPatch.length() > 0) patchName = renamedPatch;  //Prevent empty strings
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        break;
    }
  }

  settingsButton.update();
  if (settingsButton.held()) {
    //If recall held, set current patch to match current hardware state
    //Reinitialise all hardware values to force them to be re-read if different
    state = REINITIALISE;
    reinitialiseToPanel();
  } else if (settingsButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = SETTINGS;
        showSettingsPage();
        break;
      case SETTINGS:
        showSettingsPage();
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }

  backButton.update();
  if (backButton.held()) {
    //If Back button held, Panic - all notes off
    allNotesOff();
  } else if (backButton.numClicks() == 1) {
    switch (state) {
      case RECALL:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SAVE:
        renamedPatch = "";
        state = PARAMETER;
        loadPatches();  //Remove patch that was to be saved
        setPatchesOrdering(patchNo);
        break;
      case PATCHNAMING:
        charIndex = 0;
        renamedPatch = "";
        state = SAVE;
        break;
      case DELETE:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SETTINGS:
        state = PARAMETER;
        break;
      case SETTINGSVALUE:
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }

  //Encoder switch
  recallButton.update();
  if (recallButton.held()) {
    //If Recall button held, return to current patch setting
    //which clears any changes made
    state = PATCH;
    //Recall the current patch
    patchNo = patches.first().patchNo;
    recallPatch(patchNo);
    state = PARAMETER;
  } else if (recallButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = RECALL;  //show patch list
        break;
      case RECALL:
        state = PATCH;
        //Recall the current patch
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case SAVE:
        showRenamingPage(patches.last().patchName);
        patchName = patches.last().patchName;
        state = PATCHNAMING;
        break;
      case PATCHNAMING:
        if (renamedPatch.length() < 13) {
          renamedPatch.concat(String(currentCharacter));
          charIndex = 0;
          currentCharacter = CHARACTERS[charIndex];
          showRenamingPage(renamedPatch);
        }
        break;
      case DELETE:
        //Don't delete final patch
        if (patches.size() > 1) {
          state = DELETEMSG;
          patchNo = patches.first().patchNo;     //PatchNo to delete from SD card
          patches.shift();                       //Remove patch from circular buffer
          deletePatch(String(patchNo).c_str());  //Delete from SD card
          loadPatches();                         //Repopulate circular buffer to start from lowest Patch No
          renumberPatchesOnSD();
          loadPatches();                      //Repopulate circular buffer again after delete
          patchNo = patches.first().patchNo;  //Go back to 1
          recallPatch(patchNo);               //Load first patch
        }
        state = PARAMETER;
        break;
      case SETTINGS:
        state = SETTINGSVALUE;
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }
}

void reinitialiseToPanel() {
  //This sets the current patch to be the same as the current hardware panel state - all the pots
  //The four button controls stay the same state
  //This reinialises the previous hardware values to force a re-read
  muxInput = 0;
  for (int i = 0; i < MUXCHANNELS; i++) {
    mux1ValuesPrev[i] = RE_READ;
    mux2ValuesPrev[i] = RE_READ;
    mux3ValuesPrev[i] = RE_READ;
    mux4ValuesPrev[i] = RE_READ;
    mux5ValuesPrev[i] = RE_READ;
    mux6ValuesPrev[i] = RE_READ;
  }
  patchName = INITPATCHNAME;
  showPatchPage("Initial", "Panel Settings");
}

void checkEncoder() {
  //Encoder works with relative inc and dec values
  //Detent encoder goes up in 4 steps, hence +/-3

  long encRead = encoder.read();
  if ((encCW && encRead > encPrevious + 3) || (!encCW && encRead < encPrevious - 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.push(patches.shift());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case RECALL:
        patches.push(patches.shift());
        break;
      case SAVE:
        patches.push(patches.shift());
        break;
      case PATCHNAMING:
        if (charIndex == TOTALCHARS) charIndex = 0;  //Wrap around
        currentCharacter = CHARACTERS[charIndex++];
        showRenamingPage(renamedPatch + currentCharacter);
        break;
      case DELETE:
        patches.push(patches.shift());
        break;
      case SETTINGS:
        settings::increment_setting();
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::increment_setting_value();
        showSettingsPage();
        break;
    }
    encPrevious = encRead;
  } else if ((encCW && encRead < encPrevious - 3) || (!encCW && encRead > encPrevious + 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.unshift(patches.pop());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case RECALL:
        patches.unshift(patches.pop());
        break;
      case SAVE:
        patches.unshift(patches.pop());
        break;
      case PATCHNAMING:
        if (charIndex == -1)
          charIndex = TOTALCHARS - 1;
        currentCharacter = CHARACTERS[charIndex--];
        showRenamingPage(renamedPatch + currentCharacter);
        break;
      case DELETE:
        patches.unshift(patches.pop());
        break;
      case SETTINGS:
        settings::decrement_setting();
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::decrement_setting_value();
        showSettingsPage();
        break;
    }
    encPrevious = encRead;
  }
}

void loop() {
  checkMux();
  checkSwitches();
  updateEEPromSettings();
  checkEncoder();
  MIDI.read(midiChannel);
  usbMIDI.read(midiChannel);

  //voice 1 frequencies
  vcoA1.frequency(noteFreqs[note1freq] * octave * bend * voiceDetune[0]);
  vcoB1.frequency(noteFreqs[note1freq] * octave * octaveB * tuneB * bend * voiceDetune[0]);
  vcoC1.frequency(noteFreqs[note1freq] * octave * octaveC * tuneC * bend * voiceDetune[0]);
  sub1.frequency(noteFreqs[note1freq] / 2 * octave * bend * voiceDetune[0]);

  vcoA2.frequency(noteFreqs[note2freq] * octave * bend * voiceDetune[1]);
  vcoB2.frequency(noteFreqs[note2freq] * octave * octaveB * tuneB * bend * voiceDetune[1]);
  vcoC2.frequency(noteFreqs[note2freq] * octave * octaveC * tuneC * bend * voiceDetune[1]);
  sub2.frequency(noteFreqs[note2freq] / 2 * octave * bend * voiceDetune[1]);

  vcoA3.frequency(noteFreqs[note3freq] * octave * bend * voiceDetune[2]);
  vcoB3.frequency(noteFreqs[note3freq] * octave * octaveB * tuneB * bend * voiceDetune[2]);
  vcoC3.frequency(noteFreqs[note3freq] * octave * octaveC * tuneC * bend * voiceDetune[2]);
  sub3.frequency(noteFreqs[note3freq] / 2 * octave * bend * voiceDetune[2]);

  vcoA4.frequency(noteFreqs[note4freq] * octave * bend * voiceDetune[3]);
  vcoB4.frequency(noteFreqs[note4freq] * octave * octaveB * tuneB * bend * voiceDetune[3]);
  vcoC4.frequency(noteFreqs[note4freq] * octave * octaveC * tuneC * bend * voiceDetune[3]);
  sub4.frequency(noteFreqs[note4freq] / 2 * octave * bend * voiceDetune[3]);

  vcoA5.frequency(noteFreqs[note5freq] * octave * bend * voiceDetune[4]);
  vcoB5.frequency(noteFreqs[note5freq] * octave * octaveB * tuneB * bend * voiceDetune[4]);
  vcoC5.frequency(noteFreqs[note5freq] * octave * octaveC * tuneC * bend * voiceDetune[4]);
  sub5.frequency(noteFreqs[note5freq] / 2 * octave * bend * voiceDetune[4]);

  vcoA6.frequency(noteFreqs[note6freq] * octave * bend * voiceDetune[5]);
  vcoB6.frequency(noteFreqs[note6freq] * octave * octaveB * tuneB * bend * voiceDetune[5]);
  vcoC6.frequency(noteFreqs[note6freq] * octave * octaveC * tuneC * bend * voiceDetune[5]);
  sub6.frequency(noteFreqs[note6freq] / 2 * octave * bend * voiceDetune[5]);

  vcoA7.frequency(noteFreqs[note7freq] * octave * bend * voiceDetune[6]);
  vcoB7.frequency(noteFreqs[note7freq] * octave * octaveB * tuneB * bend * voiceDetune[6]);
  vcoC7.frequency(noteFreqs[note7freq] * octave * octaveC * tuneC * bend * voiceDetune[6]);
  sub7.frequency(noteFreqs[note7freq] / 2 * octave * bend * voiceDetune[6]);

  vcoA8.frequency(noteFreqs[note8freq] * octave * bend * voiceDetune[7]);
  vcoB8.frequency(noteFreqs[note8freq] * octave * octaveB * tuneB * bend * voiceDetune[7]);
  vcoC8.frequency(noteFreqs[note8freq] * octave * octaveC * tuneC * bend * voiceDetune[7]);
  sub8.frequency(noteFreqs[note8freq] / 2 * octave * bend * voiceDetune[7]);

  vcoA9.frequency(noteFreqs[note9freq] * octave * bend * voiceDetune[8]);
  vcoB9.frequency(noteFreqs[note9freq] * octave * octaveB * tuneB * bend * voiceDetune[8]);
  vcoC9.frequency(noteFreqs[note9freq] * octave * octaveC * tuneC * bend * voiceDetune[8]);
  sub9.frequency(noteFreqs[note9freq] / 2 * octave * bend * voiceDetune[8]);

  vcoA10.frequency(noteFreqs[note10freq] * octave * bend * voiceDetune[9]);
  vcoB10.frequency(noteFreqs[note10freq] * octave * octaveB * tuneB * bend * voiceDetune[9]);
  vcoC10.frequency(noteFreqs[note10freq] * octave * octaveC * tuneC * bend * voiceDetune[9]);
  sub10.frequency(noteFreqs[note10freq] / 2 * octave * bend * voiceDetune[9]);

  vcoA11.frequency(noteFreqs[note11freq] * octave * bend * voiceDetune[10]);
  vcoB11.frequency(noteFreqs[note11freq] * octave * octaveB * tuneB * bend * voiceDetune[10]);
  vcoC11.frequency(noteFreqs[note11freq] * octave * octaveC * tuneC * bend * voiceDetune[10]);
  sub11.frequency(noteFreqs[note11freq] / 2 * octave * bend * voiceDetune[10]);

  vcoA12.frequency(noteFreqs[note12freq] * octave * bend * voiceDetune[11]);
  vcoB12.frequency(noteFreqs[note12freq] * octave * octaveB * tuneB * bend * voiceDetune[11]);
  vcoC12.frequency(noteFreqs[note12freq] * octave * octaveC * tuneC * bend * voiceDetune[11]);
  sub12.frequency(noteFreqs[note12freq] / 2 * octave * bend * voiceDetune[11]);

  if (lfoAQueue.available() > 0) {
    int16_t *buffer = lfoAQueue.readBuffer();

    // Read the first sample (or average a few)
    float sample = (float)buffer[0] / 32767.0f;  // convert to -1..+1 range

    // Unipolarize for amplitude modulation (0..1)
    float ampMod = (sample * 0.5f) + 0.5f;

    // Apply modulation depth
    float depth = lfoAamp;  // your current LFO depth (0..1)
    float modValue = 1.0f - (ampMod * depth);

    // Apply to final mix gains
    finalMix.gain(0, modValue);
    finalMix.gain(1, modValue);
    finalMix.gain(2, modValue);
    finalMix.gain(3, modValue);

    lfoAQueue.freeBuffer();
  }
}
