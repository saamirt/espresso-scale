// Calibrating the load cell
#include <Arduino.h>
#include "soc/rtc.h"
#include "HX711.h"
#include "FS.h"
#include "SPIFFS.h"

#include "debounced_button.h"
#include "scale_predictor.h"

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

const int timeButtonPin = 25;
const int modeButtonPin = 27;
const int tareButtonPin = 26;
const int powerButtonPin = 33;
const int espressoMachineDoubleShotButtonPin = 32;

const float TARGET_WEIGHT = 360.0;
const unsigned long SHOT_STOP_TIME_THRESHOLD = 1100;
const unsigned long BUTTON_TRIGGER_TIME = 1000;
const unsigned long STOP_RECORDING_DELAY_TIME = 10000;
const unsigned long RECORDING_CUTOFF_DELAY = 100000;

unsigned long startTime = millis();

unsigned long recordingStartTime = 0;
// TODO: Replace this with recordingStartTime entirely.
bool isRecording = 0;

HX711 scale;
File file;

int numweightreadings = 0;

DebouncedButton powerButton = DebouncedButton(powerButtonPin, "Power");
DebouncedButton modeButton = DebouncedButton(modeButtonPin, "Mode");
DebouncedButton tareButton = DebouncedButton(tareButtonPin, "Tare");
DebouncedButton timeButton = DebouncedButton(timeButtonPin, "Time");
ScalePredictor predictor(10, TARGET_WEIGHT);

long prevWeight = LONG_MIN;
unsigned long stopRecordingTime = 0;

unsigned long triggerDoubleShotTime = 0;

const String dataFilePath = "/data.csv";

void setup() {
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(185.1423151);
  scale.set_offset(-2.123100148);

  pinMode(espressoMachineDoubleShotButtonPin, OUTPUT);

  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }

  // delay(5000);
  // read_data(dataFilePath);
  // empty_data(&file, dataFilePath);
  file = SPIFFS.open(dataFilePath, FILE_APPEND);

  tare_scale(scale, &isRecording, &file, dataFilePath, predictor);
}


void read_data(const String path) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("\n\n\n\n\n\n\nReading data from file:");
  while (file.available()) {
    String line = file.readStringUntil('\n');
    Serial.print(">>>>\t");
    Serial.println(line);
  }

  Serial.print("File size: ");
  Serial.println(file.size());

  file.close();
  Serial.println("Finished Reading data from file\n\n\n\n\n\n\n");
}

void empty_data(File *file, const String filePath) {
  SPIFFS.format();
  if (file) {
    file->close();
  }
  File fileToClear = SPIFFS.open(filePath, FILE_WRITE);
  if (!fileToClear) {
    Serial.println("Failed to open file for clearing");
    return;
  }
  fileToClear.close();
  Serial.println("File cleared.");

  write_file_buffer(file, filePath);
}

void append_row(File &file, const String &data) {
  if (!file) {
    Serial.println("File is not open for appending");
    return;
  }

  file.println(data);
}


void tare_scale(HX711 &scale, bool *isRecording, File *file, const String filePath, ScalePredictor &predictor) {
  stop_recording(isRecording, file, filePath, predictor);
  Serial.println("Tare... remove any weights from the scale.");
  // delay(200);
  scale.tare();
  Serial.println("Tare done...");
}


void write_file_buffer(File *file, const String filePath) {
  if (file && *file) {
    file->close();
    Serial.println("File updated.");
  }
  file->println("Recording Start...");
  *file = SPIFFS.open(filePath, FILE_APPEND);
}


String build_data_row(unsigned long timeDelta, int powerButtonState, int modeButtonState, int tareButtonState, int timeButtonState, long weight, float predictedTimeToTarget, int isShotTriggered) {
  return String(timeDelta) + "," + String(powerButtonState) + "," + String(modeButtonState) + "," + String(tareButtonState) + "," + String(timeButtonState) + "," + String(weight) + "," + String(predictedTimeToTarget) + "," + String(isShotTriggered);
}


void stop_recording(bool *isRecording, File *file, const String filePath, ScalePredictor &predictor) {
  Serial.println("Stopped Recording.");
  *isRecording = false;
  recordingStartTime = 0;
  stopRecordingTime = 0;
  startTime = millis();
  write_file_buffer(file, filePath);
  predictor.startPredicting();
}


void trigger_double_shot() {
  triggerDoubleShotTime = millis();
  digitalWrite(espressoMachineDoubleShotButtonPin, HIGH);
}


void loop() {
  powerButton.update();
  modeButton.update();
  tareButton.update();
  timeButton.update();

  unsigned long currentTime = millis();

  if (digitalRead(espressoMachineDoubleShotButtonPin) == HIGH && !(triggerDoubleShotTime && currentTime < triggerDoubleShotTime + BUTTON_TRIGGER_TIME)) {
    digitalWrite(espressoMachineDoubleShotButtonPin, LOW);
    triggerDoubleShotTime = 0;
  }

  bool powerButtonPressed = powerButton.isPressed();
  bool modeButtonPressed = modeButton.isPressed();
  bool tareButtonPressed = tareButton.isPressed();
  bool timeButtonPressed = timeButton.isPressed();

  if (modeButtonPressed && tareButton.getState() && timeButton.getState()) {
    Serial.println("Mode, Tare, and Time Buttons pressed. Clearing File...\n\n\n");
    empty_data(&file, dataFilePath);
    stop_recording(&isRecording, &file, dataFilePath, predictor);
    return;
  }
  if (modeButtonPressed && tareButton.getState()) {
    Serial.println("Both Mode and Tare Buttons pressed. Reading File...");
    stop_recording(&isRecording, &file, dataFilePath, predictor);
    read_data(dataFilePath);
    return;
  }
  if (tareButtonPressed) {
    tare_scale(scale, &isRecording, &file, dataFilePath, predictor);
  }
  if (timeButtonPressed) {
    if (isRecording && !stopRecordingTime) {
      stopRecordingTime = currentTime + STOP_RECORDING_DELAY_TIME;
    } else {
      isRecording = true;
      recordingStartTime = millis();
      stopRecordingTime = 0;
      Serial.println("Started Recording...");
    }
  }
  if (isRecording && currentTime >= stopRecordingTime && stopRecordingTime != 0) {
    stop_recording(&isRecording, &file, dataFilePath, predictor);
  }
  if (isRecording && recordingStartTime > 0 && currentTime >= recordingStartTime + RECORDING_CUTOFF_DELAY) {
    stop_recording(&isRecording, &file, dataFilePath, predictor);
  }
  if (numweightreadings && numweightreadings % 100) {
    write_file_buffer(&file, dataFilePath);
  }


  if (!isRecording || !scale.is_ready()) { return; }
  long weight = scale.get_units(1);
  if (abs(weight) < 2) {
    weight = 0;
  }

  unsigned long timeDelta = currentTime - startTime;

  float predictedTimeToTarget = -1.0;
  bool isShotTriggered = 0;
  if (predictor.isReadyToPredict()) {
    predictor.addReading(timeDelta, weight);
    predictedTimeToTarget = predictor.predictTime();

    if (predictedTimeToTarget >= 0 && predictedTimeToTarget <= SHOT_STOP_TIME_THRESHOLD) {
      Serial.println("Stopping shot!");
      trigger_double_shot();
      isShotTriggered = 1;
      predictor.reset();
    }
  }

  if (true || powerButton.hasStateChanged() || modeButton.hasStateChanged() || tareButton.hasStateChanged() || timeButton.hasStateChanged() || weight != prevWeight) {
    prevWeight = weight;

    String data = build_data_row(timeDelta, powerButton.getState(), modeButton.getState(), tareButton.getState(), timeButton.getState(), weight, predictedTimeToTarget, isShotTriggered);
    isShotTriggered = 0;
    append_row(file, data);
    Serial.println(data);
  }
}
