#pragma once

#include <Arduino.h>
#include "displayDescriptor.h"

enum DisplayType
{
  k40Flaps = 40,
  k62Flaps = 62
};

class FlapDisplay {
public:
  void gotoFlap(uint8_t flap_index);
  void continuousRun(unsigned long milliseconds = 1000);
  bool isCounting() { return descriptor_->is_counting || descriptor_->must_be_started;};
private:
  friend class FlapDisplayBoardHardware;

  DisplayType display_type_;
  uint8_t adc_pin_;
  DisplayDescriptor* descriptor_;

  FlapDisplay(DisplayType display_type, uint8_t adc_pin, DisplayDescriptor* descriptor);
  int8_t EncoderToPos(uint8_t encoder_value);
  uint8_t PosToEncoder(uint8_t flap_index);
};