#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "MIDIUSB.h"

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

Adafruit_MPR121 cap = Adafruit_MPR121();

// Mechanical button setup
const uint8_t buttonPins[4] = {10, 11, 12, 13};
bool lastButtonState[4];

// Notes table for SL1–SL4 (0–3) and SR1–SR4 (4–7)
const uint8_t midiNotes[8][4] = {
  { 51, 54, 61, 65 }, // SL1
  { 54, 58, 65, 68 }, // SL2
  { 68, 71, 63, 66 }, // SL3
  { 58, 62, 65, 68 }, // SL4
  { 51, 54, 73, 65 }, // SR1
  { 54, 58, 65, 68 }, // SR2
  { 68, 71, 75, 66 }, // SR3
  { 58, 62, 65, 68 }  // SR4
};

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found");
    while (1);
  }

  for (uint8_t i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonState[i] = digitalRead(buttonPins[i]);
  }

  Serial.println("Setup complete.");
}

void loop() {
  uint16_t touches = cap.touched();

  for (uint8_t i = 0; i < 4; i++) {
    bool currentState = digitalRead(buttonPins[i]);
    if (currentState != lastButtonState[i]) {
      if (currentState == LOW) {
        Serial.print("Magnet M"); Serial.print(i + 1); Serial.println(" triggered");
        triggerMagnet(i, touches);
      }
      lastButtonState[i] = currentState;
    }
  }

  delay(10); // small debounce
}

void triggerMagnet(uint8_t magnetIndex, uint16_t touches) {
  for (uint8_t sensor = 0; sensor < 8; sensor++) {
    if (touches & _BV(sensor)) {
      uint8_t note = midiNotes[sensor][magnetIndex];
      uint8_t channel = (sensor < 4) ? 0 : 1;

      Serial.print("Note ON: "); Serial.print(note);
      Serial.print(" on channel "); Serial.println(channel + 1);

      noteOn(channel, note, 127);
    }
  }

  MidiUSB.flush();

  // Optional: short note duration, then turn them off
  delay(200);

  for (uint8_t sensor = 0; sensor < 8; sensor++) {
    if (touches & _BV(sensor)) {
      uint8_t note = midiNotes[sensor][magnetIndex];
      uint8_t channel = (sensor < 4) ? 0 : 1;

      noteOff(channel, note, 0);
    }
  }

  MidiUSB.flush();
}

void noteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  midiEventPacket_t noteOn = {0x09, static_cast<uint8_t>(0x90 | channel), pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  midiEventPacket_t noteOff = {0x08, static_cast<uint8_t>(0x80 | channel), pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}
