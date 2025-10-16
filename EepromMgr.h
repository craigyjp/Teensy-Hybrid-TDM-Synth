#include <EEPROM.h>

#define EEPROM_MIDI_CH 0
#define EEPROM_PITCHBEND 1
#define EEPROM_MODWHEEL_DEPTH 2
#define EEPROM_ENCODER_DIR 3
#define EEPROM_UNISON_NOTES 15
#define EEPROM_AFTERTOUCH_DEPTH 5
#define EEPROM_NOTE_PRIORITY 6
#define EEPROM_MIDI_OUT_CH 7
#define EEPROM_UNISON_DETUNE 8
#define EEPROM_MIDI_THRU 9


int getMIDIChannel() {
  byte midiChannel = EEPROM.read(EEPROM_MIDI_CH);
  if (midiChannel < 0 || midiChannel > 16) midiChannel = MIDI_CHANNEL_OMNI;  //If EEPROM has no MIDI channel stored
  return midiChannel;
}

void storeMidiChannel(byte channel) {
  EEPROM.update(EEPROM_MIDI_CH, channel);
}

boolean getEncoderDir() {
  byte ed = EEPROM.read(EEPROM_ENCODER_DIR);
  if (ed < 0 || ed > 1) return true;  //If EEPROM has no encoder direction stored
  return ed == 1 ? true : false;
}

void storeEncoderDir(byte encoderDir) {
  EEPROM.update(EEPROM_ENCODER_DIR, encoderDir);
}

int getPitchBendRange() {
  byte pitchbend = EEPROM.read(EEPROM_PITCHBEND);
  if (pitchbend < 0 || pitchbend > 2) return pitchBendRange;  //If EEPROM has no pitchbend stored
  return pitchbend;
}

void storePitchBendRange(byte pitchbend) {
  EEPROM.update(EEPROM_PITCHBEND, pitchbend);
}

float getModWheelDepth() {
  byte mw = EEPROM.read(EEPROM_MODWHEEL_DEPTH);
  if (mw < 0 || mw > 10) return modWheelDepth;  //If EEPROM has no mod wheel depth stored
  return mw;
}

void storeModWheelDepth(byte mw) {
  EEPROM.update(EEPROM_MODWHEEL_DEPTH, mw);
}

float getafterTouchDepth() {
  byte atdepth = EEPROM.read(EEPROM_AFTERTOUCH_DEPTH);
  if (atdepth < 0 || atdepth > 10) return afterTouchDepth;  //If EEPROM has no mod wheel depth stored
  return atdepth;
}

void storeafterTouchDepth(byte atdepth) {
  EEPROM.update(EEPROM_AFTERTOUCH_DEPTH, atdepth);
}

int getNotePriority() {
  byte np = EEPROM.read(EEPROM_NOTE_PRIORITY);
  if (np < 0 || np > 2) np = 0;  //If EEPROM has no MIDI channel stored
  return np;
}

void storeNotePriority(byte np) {
  EEPROM.update(EEPROM_NOTE_PRIORITY, np);
}

int getUnisonNotes() {
  byte un = EEPROM.read(EEPROM_UNISON_NOTES);
  if (un < 2 || un > 12) un = 2;
  return un;
}

void storeUnisonNotes(byte un) {
  EEPROM.update(EEPROM_UNISON_NOTES, un);
}

int getUnisonDetune() {
  byte det = EEPROM.read(EEPROM_UNISON_DETUNE);
  if (det < 0 || det > 10) det = 0;
  return det;
}

void storeUnisonDetune(byte det) {
  EEPROM.update(EEPROM_UNISON_DETUNE, det);
}
