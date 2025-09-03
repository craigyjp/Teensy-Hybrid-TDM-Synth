#include <EEPROM.h>

#define EEPROM_MIDI_CH 0
#define EEPROM_ENCODER_DIR 1
#define EEPROM_LAST_PATCH 2
#define EEPROM_MIDI_OUT_CH 3
#define EEPROM_LOAD_FACTORY 4
#define EEPROM_UPDATE_PARAMS 5
#define EEPROM_SAVE_CURRENT 6
#define EEPROM_SAVE_ALL 7
#define EEPROM_ROM_TYPE 8
#define EEPROM_LOAD_RAM 9
#define EEPROM_BANK_SELECT 10
#define EEPROM_ENCODER_ACCELERATE 11
#define EEPROM_AFTERTOUCH 12

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
  byte ea = EEPROM.read(EEPROM_ENCODER_ACCELERATE); 
  if (ea < 0 || ea > 1)return true; //If EEPROM has no encoder direction stored
  return ea == 1 ? true : false;
}

void storeEncoderAccelerate(byte encoderAccelerate)
{
  EEPROM.update(EEPROM_ENCODER_ACCELERATE, encoderAccelerate);
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

int getSetBank() {
  byte sb = EEPROM.read(EEPROM_BANK_SELECT);
  if (sb < 0 || sb > 4) sb = 0;//If EEPROM has no MIDI channel stored
  return sb;
}

void storeSetBank(byte sb){
  EEPROM.update(EEPROM_BANK_SELECT, sb);
}

boolean getLoadFactory() {
  byte lf = EEPROM.read(EEPROM_LOAD_FACTORY); 
  if (lf < 0 || lf > 1)return true;
  return lf ? true : false;
}

void storeLoadFactory(byte lfupdate)
{
  EEPROM.update(EEPROM_LOAD_FACTORY, lfupdate);
}

boolean getROMType() {
  byte rt = EEPROM.read(EEPROM_ROM_TYPE); 
  if (rt < 0 || rt > 1)return true;
  return rt ? true : false;
}

void storeROMType(byte rtupdate)
{
  EEPROM.update(EEPROM_ROM_TYPE, rtupdate);
}

boolean getLoadRAM() {
  byte lr = EEPROM.read(EEPROM_LOAD_RAM); 
  if (lr < 0 || lr > 1)return true;
  return lr ? true : false;
}

void storeLoadRAM(byte lrupdate)
{
  EEPROM.update(EEPROM_LOAD_RAM, lrupdate);
}

boolean getSaveCurrent() {
  byte sc = EEPROM.read(EEPROM_SAVE_CURRENT); 
  if (sc < 0 || sc > 1)return true;
  return sc ? true : false;
}

void storeSaveCurrent(byte scupdate)
{
  EEPROM.update(EEPROM_SAVE_CURRENT, scupdate);
}

boolean getAfterTouch() {
  byte at = EEPROM.read(EEPROM_AFTERTOUCH); 
  if (at < 0 || at > 1)return false;
  return at ? true : false;
}

void storeAfterTouch(byte atupdate)
{
  EEPROM.update(EEPROM_AFTERTOUCH, atupdate);
}

boolean getSaveAll() {
  byte sa = EEPROM.read(EEPROM_SAVE_ALL); 
  if (sa < 0 || sa > 1)return true;
  return sa ? true : false;
}

void storeSaveAll(byte saupdate)
{
  EEPROM.update(EEPROM_SAVE_ALL, saupdate);
}
