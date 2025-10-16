#include "SettingsService.h"

void settingsMIDICh(int index, const char *value);
void settingsEncoderDir(char *value);
void settingsPitchBend(int index, const char *value);
void settingsModWheelDepth(int index, const char *value);
void settingsafterTouchDepth(int index, const char *value);
void settingsUnisonNotes(int index, const char *value);
void settingsUnisonDetune(int index, const char *value);
void settingsNotePriority(int index, const char *value);

int currentIndexMIDICh();
int currentIndexEncoderDir();
int currentIndexPitchBend();
int currentIndexModWheelDepth();
int currentIndexafterTouchDepth();
int currentIndexUnisonNotes();
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

void settingsEncoderDir(int index, const char *value) {
  if (strcmp(value, "Type 1") == 0) {
    encCW = true;
  } else {
    encCW = false;
  }
  storeEncoderDir(encCW ? 1 : 0);
}

void settingsPitchBend(int index, const char *value) {
  if (strcmp(value, "Off") == 0) {
    pitchBendRange = 0;
  } else {
    pitchBendRange = atoi(value);
  }
  storePitchBendRange(pitchBendRange);
}

void settingsModWheelDepth(int index, const char *value) {
  if (strcmp(value, "Off") == 0) {
    modWheelDepth = 0;
  } else {
    modWheelDepth = atoi(value);
  }
  storeModWheelDepth(modWheelDepth);
}

void settingsafterTouchDepth(int index, const char *value) {
  if (strcmp(value, "Off") == 0) {
    afterTouchDepth = 0;
  } else {
    afterTouchDepth = atoi(value);
  }
  storeafterTouchDepth(afterTouchDepth);
}

void settingsNotePriority(int index, const char *value) {
  if (strcmp(value, "Top") == 0) NP = 0;
  if (strcmp(value, "Bottom") == 0) NP = 1;
  if (strcmp(value, "Last") == 0) NP = 2;
  storeNotePriority(NP);
}

void settingsUnisonNotes(int index, const char *value) {
  uniNotes = atoi(value);            // convert "2" → 2, etc.
  if (uniNotes < 2) uniNotes = 2;
  if (uniNotes > 12) uniNotes = 12;
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

int currentIndexEncoderDir() {
  return getEncoderDir() ? 0 : 1;
}

int currentIndexPitchBend() {
  return getPitchBendRange();
}

int currentIndexModWheelDepth() {
  return getModWheelDepth();
}

int currentIndexafterTouchDepth() {
  return getafterTouchDepth();
}

int currentIndexNotePriority() {
  return getNotePriority();
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
  settings::append(settings::SettingsOption{ "MIDI In Ch.", { "All", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0" }, settingsMIDICh, currentIndexMIDICh });
  settings::append(settings::SettingsOption{ "Key Priority", { "Top", "Bottom", "Last", "\0" }, settingsNotePriority, currentIndexNotePriority });
  settings::append(settings::SettingsOption{ "Uni Notes", { "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "\0" }, settingsUnisonNotes, currentIndexUnisonNotes });
  settings::append(settings::SettingsOption{ "Unison Det", { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "\0" }, settingsUnisonDetune, currentIndexUnisonDetune });
  settings::append(settings::SettingsOption{ "Encoder", { "Type 1", "Type 2", "\0" }, settingsEncoderDir, currentIndexEncoderDir });
  settings::append(settings::SettingsOption{ "Pitch Bend", { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "\0" }, settingsPitchBend, currentIndexPitchBend });
  settings::append(settings::SettingsOption{ "MW Depth", { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "\0" }, settingsModWheelDepth, currentIndexModWheelDepth });
  settings::append(settings::SettingsOption{ "AT Depth", { "Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "\0" }, settingsafterTouchDepth, currentIndexafterTouchDepth });
}
