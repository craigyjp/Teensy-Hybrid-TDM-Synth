#include <EEPROM.h>

#define EEPROM_MIDI_CH 0
#define EEPROM_ENCODER_DIR 1
#define EEPROM_LAST_PATCH 2
#define EEPROM_MIDI_OUT_CH 3
#define EEPROM_UPDATE_PARAMS 5
#define EEPROM_UNISON_DETUNE 8
#define EEPROM_UNISON_NOTES 9
#define EEPROM_ENCODER_ACCELERATE 11


int getMIDIChannel() {
  byte midiChannel = EEPROM.read(EEPROM_MIDI_CH);
  if (midiChannel < 0 || midiChannel > 16) midiChannel = MIDI_CHANNEL_OMNI;//If EEPROM has no MIDI channel stored
  return midiChannel;
}

void storeMidiChannel(byte channel)
{
  EEPROM.update(EEPROM_MIDI_CH, channel);
}

boolean getEncoderDir() {
  byte ed = EEPROM.read(EEPROM_ENCODER_DIR); 
  if (ed < 0 || ed > 1)return true; //If EEPROM has no encoder direction stored
  return ed == 1 ? true : false;
}

void storeEncoderDir(byte encoderDir)
{
  EEPROM.update(EEPROM_ENCODER_DIR, encoderDir);
}

boolean getEncoderAccelerate() {
  accelerate = EEPROM.read(EEPROM_ENCODER_ACCELERATE);
  if (accelerate < 0 || accelerate > 1) return true; // default = true
  return accelerate == 1;
}

void storeEncoderAccelerate(byte accelerate)
{
  EEPROM.write(EEPROM_ENCODER_ACCELERATE, accelerate);
}

boolean getUpdateParams() {
  byte params = EEPROM.read(EEPROM_UPDATE_PARAMS); 
  if (params < 0 || params > 1)return true; //If EEPROM has no encoder direction stored
  return params == 1 ? true : false;
}

void storeUpdateParams(byte updateParameters)
{
  EEPROM.update(EEPROM_UPDATE_PARAMS, updateParameters);
}

int getLastPatch() {
  int lastPatchNumber = EEPROM.read(EEPROM_LAST_PATCH);
  if (lastPatchNumber < 1 || lastPatchNumber > 999) lastPatchNumber = 1;
  return lastPatchNumber;
}

void storeLastPatch(int lastPatchNumber)
{
  EEPROM.update(EEPROM_LAST_PATCH, lastPatchNumber);
}

int getMIDIOutCh() {
  byte mc = EEPROM.read(EEPROM_MIDI_OUT_CH);
  if (mc < 0 || midiOutCh > 16) mc = 0;//If EEPROM has no MIDI channel stored
  return mc;
}

void storeMidiOutCh(byte midiOutCh){
  EEPROM.update(EEPROM_MIDI_OUT_CH, midiOutCh);
}

int getUnisonNotes() {
  byte un = EEPROM.read(EEPROM_UNISON_NOTES);
  if (un < 2 || un > 8) un = 2;
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



