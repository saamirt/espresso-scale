#include "scale_predictor.h"
#include <Arduino.h>

ScalePredictor::ScalePredictor(int capacity, float target)
  : capacity_(capacity), targetWeight_(target), weights_(new float[capacity]), timestamps_(new long[capacity]), index_(0), isFull_(false), readyToPredict_(true) {}

ScalePredictor::~ScalePredictor() {
  delete[] weights_;
  delete[] timestamps_;
}

void ScalePredictor::addReading(long timestamp, float weight) {
  weights_[index_] = weight;
  timestamps_[index_] = timestamp;
  index_ = (index_ + 1) % capacity_;
  if (index_ == 0) isFull_ = true;
}

float ScalePredictor::predictTime() {
  if (!isFull_ || !readyToPredict_) {
    // Serial.print("not enough readings: ");
    // Serial.print(!isFull_);
    // Serial.print(", not readyToPredict: ");
    // Serial.println(readyToPredict_);
    return -1.0f;
  };

  float currentWeight = weights_[(index_ - 1 + capacity_) % capacity_];  // Latest weight reading
  if (currentWeight >= targetWeight_) {
    // Serial.print("already passed target. (");
    // Serial.print(currentWeight);
    // Serial.print(" >= ");
    // Serial.print(targetWeight_);
    // Serial.println(")");
    return 0.0f;
  }  // If target weight is already passed

  float rate = calculateRateOfChange();
  if (rate <= 0) {
    // Serial.print("Non-positive flow rate. (");
    // Serial.print(rate);
    // Serial.println(")");
    return -1.0f;
  }  // If the rate is not positive, prediction is not possible

  float weightToTarget = targetWeight_ - currentWeight;
  return weightToTarget / rate;
}

float ScalePredictor::calculateRateOfChange() const {
  if (index_ < 2) return 0; // Need at least two points to calculate rate

  // Variables for the sums / means
  float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
  int n = 0;
  
  for (int i = 0; i < capacity_; ++i) {
    if (timestamps_[i] != 0) { // Consider only filled entries
      float x = (float)(timestamps_[i] - timestamps_[0]); // Time since first reading
      float y = weights_[i];
      sumX += x;
      sumY += y;
      sumXY += x * y;
      sumX2 += x * x;
      ++n;
    }
  }

  // Means of x and y
  float meanX = sumX / n;
  float meanY = sumY / n;

  // Linear regression slope (beta)
  float beta = (sumXY - n * meanX * meanY) / (sumX2 - n * meanX * meanX);

  return beta; // The slope is the rate of change of weight over time
}

void ScalePredictor::reset() {
  Serial.println("Not ready to predict anymore!");
  index_ = 0;
  isFull_ = false;
  readyToPredict_ = false;  // Not ready to predict right after reset
  for (int i = 0; i < capacity_; i++) {
    weights_[i] = 0;
    timestamps_[i] = 0;
  }
}

void ScalePredictor::startPredicting() {
  Serial.println("Ready to predict!");
  readyToPredict_ = true;
}

bool ScalePredictor::isReadyToPredict() const {
  return readyToPredict_;
}
