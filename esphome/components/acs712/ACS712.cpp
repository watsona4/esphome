//
//    FILE: ACS712.cpp
//  AUTHOR: Rob Tillaart, Pete Thompson
// VERSION: 0.3.10
//    DATE: 2020-08-02
// PURPOSE: ACS712 library - current measurement
//     URL: https://github.com/RobTillaart/ACS712

#include "ACS712.h"
#include "esphome/core/log.h"

//  CONSTRUCTOR
ACS712::ACS712(uint8_t analogPin, float volts, uint16_t maxADC, float mVperAmpere) {
  _pin = analogPin;
  _mVperAmpere = mVperAmpere;
  _formFactor = ACS712_FF_SINUS;
  _noisemV = ACS712_DEFAULT_NOISE;  //  21mV according to datasheet

  //  set in setADC()
  //  keep it here until after experimental.
  _maxADC = maxADC;
  _mVperStep = 1000.0 * volts / maxADC;  //  1x 1000.0 for V -> mV
  _mAPerStep = 1000.0 * _mVperStep / _mVperAmpere;
  _midPoint = maxADC / 2;

  //  default ADC is internal.
  setADC(NULL, volts, maxADC);
}

//  MEASUREMENTS
float ACS712::mA_peak2peak(float frequency, uint16_t cycles) {
  uint16_t period = round(1000000UL / frequency);

  if (cycles == 0)
    cycles = 1;
  float sum = 0;

  for (uint16_t i = 0; i < cycles; i++) {
    int minimum, maximum;
    //  Better than using midPoint
    minimum = maximum = _analogRead(_pin);

    //  find minimum and maximum
    uint32_t start = micros();
    while (micros() - start < period)  //  UNO ~180 samples...
    {
      int value = _analogRead(_pin);
      if (_suppresNoise)  //  average 2 samples.
      {
        value = (value + _analogRead(_pin)) / 2;
      }
      //  determine extremes
      if (value < minimum)
        minimum = value;
      else if (value > maximum)
        maximum = value;
    }
    sum += (maximum - minimum);
  }
  float peak2peak = sum * _mAPerStep;
  if (cycles > 1)
    peak2peak /= cycles;

  return peak2peak;
}

float ACS712::mA_AC(float frequency, uint16_t cycles) {
  ESP_LOGD("ACS712::mA_AC()", "frequency: %d", frequency);
  uint16_t period = round(1000000UL / frequency);
  ESP_LOGD("ACS712::mA_AC()", "period: %d", period);

  if (cycles == 0)
    cycles = 1;
  ESP_LOGD("ACS712::mA_AC()", "cycles: %d", cycles);
  float sum = 0;

  //  remove float operation from loop.
  uint16_t zeroLevel = round(_noisemV / _mVperStep);
  ESP_LOGD("ACS712::mA_AC()", "zeroLevel: %d", zeroLevel);

  for (uint16_t i = 0; i < cycles; i++) {
    ESP_LOGD("ACS712::mA_AC()", "i: %d", i);
    uint16_t samples = 0;
    uint16_t zeros = 0;

    int minimum, maximum;
    minimum = maximum = _analogRead(_pin);

    //  find minimum and maximum and count the zero-level "percentage"
    uint32_t start = micros();
    ESP_LOGD("ACS712::mA_AC()", "  start: %d", start);
    while (micros() - start < period)  // UNO ~180 samples...
    {
      samples++;
      int value = _analogRead(_pin);
      if (_suppresNoise)  //  average 2 samples.
      {
        value = (value + _analogRead(_pin)) / 2;
      }
      ESP_LOGD("ACS712::mA_AC()", "    value: %d", value);
      //  determine extremes
      if (value < minimum)
        minimum = value;
      else if (value > maximum)
        maximum = value;
      //  count zeros
      if (abs(value - _midPoint) <= zeroLevel)
        zeros++;
    }
    ESP_LOGD("ACS712::mA_AC()", "  zeros: %d", zeros);
    ESP_LOGD("ACS712::mA_AC()", "  samples: %d", samples);
    ESP_LOGD("ACS712::mA_AC()", "  minimum: %d", minimum);
    ESP_LOGD("ACS712::mA_AC()", "  maximum: %d", maximum);
    int peak2peak = maximum - minimum;
    ESP_LOGD("ACS712::mA_AC()", "  peak2peak: %d", peak2peak);

    //  automatic determine _formFactor / crest factor
    float D = 0;
    float FF = 0;
    if (zeros > samples * 0.025)  //  more than 2% zero's
    {
      D = 1.0 - (1.0 * zeros) / samples;  //  % SAMPLES NONE ZERO
      FF = sqrt(D) * _formFactor;         //  ASSUME NON ZERO PART ~ SINUS
    } else                                //  # zeros is small => D --> 1 --> sqrt(D) --> 1
    {
      FF = _formFactor;
    }
    ESP_LOGD("ACS712::mA_AC()", "  FF: %f", FF);

    //  value could be partially pre-calculated: C = 1000.0 * 0.5 * _mVperStep / _mVperAmpere;
    //  return 1000.0 * 0.5 * peak2peak * _mVperStep * _formFactor / _mVperAmpere);
    sum += peak2peak * FF;
    ESP_LOGD("ACS712::mA_AC()", "  sum: %f", sum);
  }
  ESP_LOGD("ACS712::mA_AC()", "sum: %f", sum);
  float mA = 0.5 * sum * _mAPerStep;
  ESP_LOGD("ACS712::mA_AC()", "mA: %f", mA);
  if (cycles > 1)
    mA /= cycles;
  ESP_LOGD("ACS712::mA_AC()", "mA: %f", mA);

  return mA;
}

float ACS712::mA_AC_sampling(float frequency, uint16_t cycles) {
  ESP_LOGD("ACS712::mA_AC_sampling()", "frequency: %d", frequency);
  uint32_t period = round(1000000UL / frequency);
  ESP_LOGD("ACS712::mA_AC_sampling()", "period: %d", period);

  if (cycles == 0)
    cycles = 1;
  ESP_LOGD("ACS712::mA_AC_sampling()", "cycles: %d", cycles);
  float sum = 0;

  //  float noiseLevel = _noisemV/_mVperStep;

  for (uint16_t i = 0; i < cycles; i++) {
    ESP_LOGD("ACS712::mA_AC_sampling()", "i: %d", i);
    uint16_t samples = 0;
    float sumSquared = 0;

    uint32_t start = micros();
    ESP_LOGD("ACS712::mA_AC_sampling()", "  start: %d", start);
    while (micros() - start < period) {
      samples++;
      int value = _analogRead(_pin);
      if (_suppresNoise)  //  average 2 samples.
      {
        value = (value + _analogRead(_pin)) / 2;
      }
      ESP_LOGD("ACS712::mA_AC_sampling()", "    value: %d", value);
      float current = value - _midPoint;
      sumSquared += (current * current);
      //  not adding noise squared might be more correct for small currents.
      //  if (abs(current) > noiseLevel)
      //  {
      //    sumSquared += (current * current);
      //  }
    }
    ESP_LOGD("ACS712::mA_AC_sampling()", "  samples: %d", samples);
    ESP_LOGD("ACS712::mA_AC_sampling()", "  sumSquared: %d", sumSquared);
    sum += sqrt(sumSquared / samples);
    ESP_LOGD("ACS712::mA_AC_sampling()", "  sum: %f", sum);
  }
  ESP_LOGD("ACS712::mA_AC_sampling()", "sum: %f", sum);
  float mA = sum * _mAPerStep;
  ESP_LOGD("ACS712::mA_AC_sampling()", "mA: %f", mA);
  if (cycles > 1)
    mA /= cycles;
  ESP_LOGD("ACS712::mA_AC_sampling()", "mA: %f", mA);

  return mA;
}

float ACS712::mA_DC(uint16_t cycles) {
  //  read at least twice to stabilize the ADC
  _analogRead(_pin);
  if (cycles == 0)
    cycles = 1;
  float sum = 0;
  for (uint16_t i = 0; i < cycles; i++) {
    int value = _analogRead(_pin);
    if (_suppresNoise)  //  average 2 samples.
    {
      value = (value + _analogRead(_pin)) / 2;
    }
    //  for RTOS
    if ((i & 0x0001) == 0x0001)  //  every 2nd iteration
    {
      yield();
    }
    sum += (value - _midPoint);
  }
  float mA = sum * _mAPerStep;
  if (cycles > 1)
    mA /= cycles;

  return mA;
}

//  CALIBRATION MIDPOINT
uint16_t ACS712::setMidPoint(uint16_t midPoint) {
  if (midPoint <= _maxADC)
    _midPoint = (int) midPoint;
  return _midPoint;
};

uint16_t ACS712::getMidPoint() { return _midPoint; };

uint16_t ACS712::incMidPoint() {
  if (_midPoint < (int) (_maxADC))
    _midPoint += 1;
  return _midPoint;
};

uint16_t ACS712::decMidPoint() {
  if (_midPoint > 0)
    _midPoint -= 1;
  return _midPoint;
};

//  configure by sampling for 2 cycles of AC
//  Also works for DC as long as no current flowing
//  note this is blocking!
uint16_t ACS712::autoMidPoint(float frequency, uint16_t cycles) {
  ESP_LOGD("ACS712::autoMidPoint()", "frequency: %f", frequency);
  ESP_LOGD("ACS712::autoMidPoint()", "cycles: %d", cycles);
  uint16_t twoPeriods = round(2000000UL / frequency);
  ESP_LOGD("ACS712::autoMidPoint()", "twoPeriods: %d", twoPeriods);

  if (cycles == 0)
    cycles = 1;
  ESP_LOGD("ACS712::autoMidPoint()", "cycles: %d", cycles);

  uint32_t total = 0;
  ESP_LOGD("ACS712::autoMidPoint()", "total: %d", total);
  for (uint16_t i = 0; i < cycles; i++) {
    ESP_LOGD("ACS712::autoMidPoint()", "  i: %d", i);
    uint32_t subTotal = 0;
    uint32_t samples = 0;
    uint32_t start = micros();
    while (micros() - start < twoPeriods) {
      uint16_t reading = _analogRead(_pin);
      ESP_LOGD("ACS712::autoMidPoint()", "    reading: %d", reading);
      subTotal += reading;
      samples++;
      //  Delaying prevents overflow
      //  since we'll perform a maximum of 40,000 reads @ 50 Hz.
      delayMicroseconds(1);
    }
    ESP_LOGD("ACS712::autoMidPoint()", "  subTotal: %d", subTotal);
    ESP_LOGD("ACS712::autoMidPoint()", "  samples: %d", samples);
    total += (subTotal / samples);
    ESP_LOGD("ACS712::autoMidPoint()", "  total: %d", total);
  }
  _midPoint = (total + (cycles / 2)) / cycles;  //  rounding.
  ESP_LOGD("ACS712::autoMidPoint()", "_midPoint: %d", _midPoint);
  return _midPoint;
}

uint16_t ACS712::autoMidPointDC(uint16_t cycles) {
  if (cycles == 0)
    cycles = 1;
  uint32_t total = 0;
  for (uint16_t i = 0; i < cycles; i++) {
    total += analogRead(_pin);
  }
  _midPoint = (total + (cycles / 2)) / cycles;  //  rounding.
  return _midPoint;
}

uint16_t ACS712::resetMidPoint() {
  _midPoint = _maxADC / 2;
  return _midPoint;
};

//  CALIBRATION FORM FACTOR
void ACS712::setFormFactor(float formFactor) { _formFactor = formFactor; };

float ACS712::getFormFactor() { return _formFactor; };

//  CALIBRATION NOISE
//  noise defaults 21 datasheet
void ACS712::setNoisemV(uint8_t noisemV) { _noisemV = noisemV; };

uint8_t ACS712::getNoisemV() { return _noisemV; };

float ACS712::mVNoiseLevel(float frequency, uint16_t cycles) {
  float mA = mA_peak2peak(frequency, cycles);
  //  divide by 2 as the level is half of the peak to peak range
  return mA * _mVperAmpere * 0.001 / 2;
}

void ACS712::suppressNoise(bool flag) { _suppresNoise = flag; }

//  CALIBRATION mV PER AMP
//  Adjusting resolution AC and DC
void ACS712::setmVperAmp(float mVperAmpere) {
  _mVperAmpere = mVperAmpere;
  _mAPerStep = 1000.0 * _mVperStep / _mVperAmpere;
};

float ACS712::getmVperAmp() { return _mVperAmpere; };

float ACS712::getmAPerStep() { return _mAPerStep; };

float ACS712::getAmperePerStep() { return _mAPerStep * 0.001; };

//  FREQUENCY DETECTION
//  uses oversampling and averaging to minimize variation
//  blocks for substantial amount of time, depending on minimalFrequency
float ACS712::detectFrequency(float minimalFrequency) {
  int maximum = 0;
  int minimum = 0;
  maximum = minimum = _analogRead(_pin);

  //  determine maxima
  uint32_t timeOut = round(1000000.0 / minimalFrequency);
  uint32_t start = micros();
  while (micros() - start < timeOut) {
    int value = _analogRead(_pin);
    if (value > maximum)
      maximum = value;
    if (value < minimum)
      minimum = value;
  }

  //  calculate quarter points
  //  using quarter points is less noise prone than using one single midpoint
  int Q1 = (3 * minimum + maximum) / 4;
  int Q3 = (minimum + 3 * maximum) / 4;

  //  10x passing Quantile points
  //  wait for the right moment to start
  //  to prevent endless loop a timeout is checked.
  timeOut *= 10;
  start = micros();
  //  casting to int to keep compiler happy.
  while ((int(_analogRead(_pin)) > Q1) && ((micros() - start) < timeOut))
    ;
  while ((int(_analogRead(_pin)) <= Q3) && ((micros() - start) < timeOut))
    ;
  start = micros();
  for (int i = 0; i < 10; i++) {
    while ((int(_analogRead(_pin)) > Q1) && ((micros() - start) < timeOut))
      ;
    while ((int(_analogRead(_pin)) <= Q3) && ((micros() - start) < timeOut))
      ;
  }
  uint32_t stop = micros();

  //  calculate frequency
  float wavelength = stop - start;
  float frequency = 1e7 / wavelength;
  if (_microsAdjust != 1.0)
    frequency *= _microsAdjust;
  return frequency;
}

//  timing for FREQUENCY DETECTION
void ACS712::setMicrosAdjust(float factor) { _microsAdjust = factor; };

float ACS712::getMicrosAdjust() { return _microsAdjust; };

//  DEBUG
uint16_t ACS712::getMinimum(uint16_t milliSeconds) {
  uint16_t minimum = _analogRead(_pin);

  //  find minimum
  uint32_t start = millis();
  while (millis() - start < milliSeconds) {
    uint16_t value = _analogRead(_pin);
    if (value < minimum)
      minimum = value;
  }
  return minimum;
}

uint16_t ACS712::getMaximum(uint16_t milliSeconds) {
  uint16_t maximum = _analogRead(_pin);

  //  find maximum
  uint32_t start = millis();
  while (millis() - start < milliSeconds) {
    uint16_t value = _analogRead(_pin);
    if (value > maximum)
      maximum = value;
  }
  return maximum;
}

void ACS712::setADC(uint16_t (*f)(uint8_t), float volts, uint16_t maxADC) {
  _readADC = f;

  _maxADC = maxADC;
  ESP_LOGD("setADC()", "_maxADC: %d", _maxADC);
  _mVperStep = 1000.0 * volts / maxADC;  //  1x 1000 for V -> mV
  ESP_LOGD("setADC()", "_mVperStep: %d", _mVperStep);
  _mAPerStep = 1000.0 * _mVperStep / _mVperAmpere;
  ESP_LOGD("setADC()", "_mAPerStep: %d", _mAPerStep);
  _midPoint = maxADC / 2;
  ESP_LOGD("setADC()", "_midPoint: %d", _midPoint);
}

//////////////////////////////////////////////////////////////////////
//
//  PRIVATE
//
uint16_t ACS712::_analogRead(uint8_t pin) {
  //  if external ADC is defined use it.
  if (_readADC != NULL) {
    return _readADC(pin);
  }
  return analogRead(pin);
}

//  -- END OF FILE --
