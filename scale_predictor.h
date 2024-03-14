#ifndef SCALE_PREDICTOR_H
#define SCALE_PREDICTOR_H

class ScalePredictor {
public:
  ScalePredictor(int capacity, float target);
  ~ScalePredictor();
  void addReading(long timestamp, float weight);
  float predictTime();
  void reset();
  void startPredicting();
  bool isReadyToPredict() const;

private:
  int capacity_;
  float targetWeight_;
  float *weights_;
  long *timestamps_;
  int index_;
  bool isFull_;
  bool readyToPredict_;

  float calculateRateOfChange() const;
};

#endif // SCALE_PREDICTOR_H
