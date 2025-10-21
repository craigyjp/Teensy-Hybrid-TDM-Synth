#include "SettingsService.h"

void settingsMIDICh();
void settingsMIDIOutCh();
void settingsEncoderDir();
void settingsEncoderAccelerate();
void settingsUpdateParams();
void settingsUnisonNotes(int index, const char *value);
void settingsUnisonDetune(int index, const char *value);

int currentIndexMIDICh();
int currentIndexMIDIOutCh();
int currentIndexEncoderDir();
int currentIndexEncoderAccelerate();
int currentIndexUpdateParams();
int currentIndexUnisonDetune();
int currentIndexNotePriority();

void settingsMIDICh(int index, const char *value) {
  if (strcmp(value, "ALL") == 0) {
    midiChannel = MIDI_CHANNEL_OMNI;
  } else {
    midiChannel = atoi(value);
  }
  storeMidiChannel(midiChannel);
}

void settingsMIDIOutCh(int index, const char *value) {
  if (strcmp(value, "Off") == 0) {
    midiOutCh = 0;
  } else {
    midiOutCh = atoi(value);
  }
  storeMidiOutCh(midiOutCh);
}

void settingsEncoderDir(int index, const char *value) {
  if (strcmp(value, "Type 1") == 0) {
    encCW = true;
  } else {
    encCW = false;
  }
  storeEncoderDir(encCW ? 1 : 0);
}

void settingsEncoderAccelerate(int index, const char *value) {
  if (strcmp(value, "Yes") == 0) {
    accelerate = true;
  } else {
    accelerate = false;
  }
  storeEncoderAccelerate(accelerate ? 1 : 0);
}

void settingsUpdateParams(int index, const char *value) {
  if (strcmp(value, "Send Params") == 0) {
    updateParams = true;
  } else {
    updateParams = false;
  }
  storeUpdateParams(updateParams ? 1 : 0);
}

void settingsUnisonNotes(int index, const char *value) {
  uniNotes = atoi(value);            // convert "2" → 2, etc.
  if (uniNotes < 2) uniNotes = 2;
  if (uniNotes > 8) uniNotes = 8;
  storeUnisonNotes(uniNotes);
}

void settingsUnisonDetune(int index, const char *value) {
  if (strcmp(value, "Off") == 0) unidetune = 0;
  if (strcmp(value, "1") == 0) unidetune = 1;
  if (strcmp(value, "2") == 0) unidetune = 2;
  if (strcmp(value, "3") == 0) unidetune = 3;
  if (strcmp(value, "4") == 0) unidetune = 4;
  if (strcmp(value, "5") == 0) unidetune = 5;
  if (strcmp(value, "6") == 0) unidetune = 6;
  if (strcmp(value, "7") == 0) unidetune = 7;
  if (strcmp(value, "8") == 0) unidetune = 8;
  if (strcmp(value, "9") == 0) unidetune = 9;
  if (strcmp(value, "10") == 0) unidetune = 10;
  storeUnisonDetune(unidetune);
}

int currentIndexMIDICh() {
  return getMIDIChannel();
}

int currentIndexMIDIOutCh() {
  return getMIDIOutCh();
}

int currentIndexEncoderDir() {
  return getEncoderDir() ? 0 : 1;
}

int currentIndexEncoderAccelerate() {
  return getEncoderAccelerate() ? 1 : 0;
}

int currentIndexUpdateParams() {
  return getUpdateParams() ? 1 : 0;
}

int currentIndexUnisonNotes() {
  int stored = getUnisonNotes(); // returns 2–12
  return stored - 2;             // convert to index 0–10
}

int currentIndexUnisonDetune() {
  return getUnisonDetune();
}

// add settings to the circular buffer
void setUpSettings() {
  settings::append(settings::SettingsOption{ "MIDI Ch.", { "All", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0" }, settingsMIDICh, currentIndexMIDICh });
  settings::append(settings::SettingsOption{ "MIDI Out Ch.", { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0" }, settingsMIDIOutCh, currentIndexMIDIOutCh });
  settings::append(settings::SettingsOption{ "Encoder", { "Type 1", "Type 2", "\0" }, settingsEncoderDir, currentIndexEncoderDir });
  settings::append(settings::SettingsOption{ "Enc Speed", { "No", "Yes", "\0" }, settingsEncoderAccelerate, currentIndexEncoderAccelerate });
  settings::append(settings::SettingsOption{ "MIDI Params", { "Off", "Send Params", "\0" }, settingsUpdateParams, currentIndexUpdateParams });
  settings::append(settings::SettingsOption{ "Uni Notes", { "2", "3", "4", "5", "6", "7", "8", "\0" }, settingsUnisonNotes, currentIndexUnisonNotes });
  settings::append(settings::SettingsOption{ "Unison Det", { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "\0" }, settingsUnisonDetune, currentIndexUnisonDetune });
}
