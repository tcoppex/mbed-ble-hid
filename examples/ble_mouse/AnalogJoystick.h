#ifndef ANALOG_JOYSTICK_H_
#define ANALOG_JOYSTICK_H_

#include "signal_utils.h"


class AnalogJoystick {  
 private:
  static const int kDefaultCalibrationSamples = 8;
  static const int kDefaultCalibrationSamplingDelay = 50;

  static constexpr float MAX_POWER_SUPPLY  = 5.0f;
  static constexpr float POWER_SUPPLY      = 5.0f;
  static constexpr float kPSUFactor = POWER_SUPPLY / MAX_POWER_SUPPLY;

 public:
  AnalogJoystick(int pin_x, int pin_y, int pin_button) :
    pin_x_(pin_x),
    pin_y_(pin_y),
    pin_button_(pin_button),
    x_(0.0f),
    y_(0.0f),
    button_(0)
  {}

  /* To call once at joystick initialization */
  void Calibrate(int numSamples = kDefaultCalibrationSamples, int samplingDelay = kDefaultCalibrationSamplingDelay)
  {
    calibration_x_ = 0.0f;
    calibration_y_ = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
      Read(false);
      calibration_x_ += x_;
      calibration_y_ += y_;
      delay(samplingDelay);
    }
    calibration_x_ /= numSamples;
    calibration_y_ /= numSamples;
    // save calibration as default value if filtering is used afterwards.
    x_ = calibration_x_;
    y_ = calibration_y_;
  }

  void Read(bool filter=true)
  {    
    x_ = analogRead(pin_x_);
    y_ = analogRead(pin_y_);

    const float minValue = 0.0f;
    const float maxValue = lerp(0.0f, 1023.0f, kPSUFactor);
    x_ = mmap(x_, minValue, maxValue, -1.0f, 1.0f);
    y_ = mmap(y_, minValue, maxValue, -1.0f, 1.0f);
    
    button_ = !digitalRead(pin_button_);

    if (filter) {
      Filter();
    }
  }

  float x() const {
    return x_;
  }

  float y() const {
    return y_;
  }

  int button() const {
    return button_;
  }

 private:
  void Filter()
  {
    float last_x = x_;
    float last_y = y_;

    const float eps = 0.001f;
    if (fabs(x_ - calibration_x_) < eps) {
      x_ = mmap(x_, -1.0f, calibration_x_, -1.0f, 0.0f);
    } else {
      x_ = mmap(x_, calibration_x_, 1.0f, 0.0f, 1.0f);
    }

    if (fabs(y_ - calibration_y_) < eps) {
      y_ = mmap(y_, -1.0f, calibration_y_, -1.0f, 0.0f);
    } else {
      y_ = mmap(y_, calibration_y_, 1.0f, 0.0f, 1.0f);
    }

    const float l = 0.01f;
    x_ *= smoothstep(l, 1.0f-l, fabs(x_));
    y_ *= smoothstep(l, 1.0f-l, fabs(y_));

    const float damp_factor = 0.96f;
    x_ = lerp( last_x, x_, damp_factor);
    y_ = lerp( last_y, y_, damp_factor);
  }


  int pin_x_;
  int pin_y_;
  int pin_button_;

  float calibration_x_;
  float calibration_y_;

  float x_;
  float y_;
  int button_;
};

#endif  // ANALOG_JOYSTICK_H_