#include "debounced_button.h"

DebouncedButton::DebouncedButton(uint8_t pin, String buttonName, unsigned long debounceDelay)
  : pin(pin), debounceDelay(debounceDelay), buttonName(buttonName), lastState(LOW), currentState(LOW),
    lastDebounceTime(0), stateChanged(false) {
  pinMode(pin, INPUT_PULLUP);
}

void DebouncedButton::update() {
  int reading = !digitalRead(pin);
  if (reading != lastState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currentState) {
      if (reading){
        if (buttonName){
          Serial.print(buttonName + " ");
        }
        Serial.println("Button pressed.");
      }
      currentState = reading;
      stateChanged = true;
    }
  } else {
    stateChanged = false;
  }

  lastState = reading;
}

bool DebouncedButton::hasStateChanged() {
  if (stateChanged) {
    stateChanged = false;
    return true;
  }
  return false;
}

bool DebouncedButton::isPressed() {
  return currentState && hasStateChanged();
}

int DebouncedButton::getState() {
  return currentState;
}
