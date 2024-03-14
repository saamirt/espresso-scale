#ifndef DEBOUNCED_BUTTON_H
#define DEBOUNCED_BUTTON_H

#include <Arduino.h>

class DebouncedButton {
public:
  DebouncedButton(uint8_t pin, String buttonName = "", unsigned long debounceDelay = 60);
  void update();
  bool hasStateChanged();
  int getState();
  bool isPressed();

private:
  uint8_t pin;
  unsigned long debounceDelay;
  String buttonName;
  int lastState;
  int currentState;
  unsigned long lastDebounceTime;
  bool stateChanged;
};

#endif // DEBOUNCED_BUTTON_H
