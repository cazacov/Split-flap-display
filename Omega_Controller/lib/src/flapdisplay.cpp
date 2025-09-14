#include "flapdisplay.h"
#include <Arduino.h>

FlapDisplay::FlapDisplay(DisplayType display_type, uint8_t adc_pin, DisplayDescriptor* descriptor) :
    display_type_(display_type),
    adc_pin_(adc_pin),
    descriptor_(descriptor)
{
  pinMode(adc_pin_, OUTPUT);

  descriptor->adc_pin = adc_pin;
  descriptor->is_counting = false;
  descriptor->target_millis = 0;
  descriptor->target_encoder = 0;
  descriptor->must_be_started = false;
  descriptor->current_encoder = 0;
}

void FlapDisplay::continuousRun(unsigned long milliseconds) {
  descriptor_->target_encoder = 0;
  descriptor_->target_millis = millis() + milliseconds;
  descriptor_->must_be_started = true;  
}

void FlapDisplay::gotoFlap(uint8_t flap_index) {

  if (isCounting()) {
    Serial.println("Is already counting. Skip gotoFlap");
    return;
  }

  uint8_t current_encoder = descriptor_->current_encoder;
  int8_t current_pos = EncoderToPos(current_encoder); 
  uint8_t target_encoder = PosToEncoder(flap_index);

  Serial.println("");
  Serial.print("GOTO ");
  Serial.println(flap_index);
  Serial.print("Current pos: ");
  Serial.println(current_pos);
  Serial.print("Current encoder: ");
  Serial.println(current_encoder);
  Serial.print("Target encoder: ");
  Serial.println(target_encoder);

  if (current_encoder == target_encoder) {
    Serial.println("nothing to do");
    return;  
  }

  int8_t steps = 0;

  uint8_t flaps_count = (display_type_ == k40Flaps) ? 40 : 62;

  if (current_encoder) {
    if (current_pos >= 0) {
      steps = flap_index - current_pos;
      if (steps < 0) {
        steps += flaps_count;
      }
    }
  }
  Serial.print("Steps: ");
  Serial.println(steps);

  long del = 20;
  
  if (steps > 0) {
    del = 452L * 1000 * steps / (81 * flaps_count) - 40;
  }
  Serial.print("Delay: ");
  Serial.println(del);

  descriptor_->target_encoder = target_encoder;
  descriptor_->target_millis = millis() + del;
  descriptor_->must_be_started = true;  
}

int8_t FlapDisplay::EncoderToPos(uint8_t encoder_value) {
  
  if (!encoder_value) {
    return -1;
  }

  switch (display_type_) {
    if (encoder_value == 0) {
      return -1;
    }
    case k40Flaps:
      if (encoder_value > 40) {
        return -2;
      }
      return encoder_value - 1;
      break;
    case k62Flaps:
      if (encoder_value > 62) {
        return -2;
      }
      return encoder_value - 1;
      break;
    default:
      return -3;
  }
}

uint8_t FlapDisplay::PosToEncoder(uint8_t flap_index) {
  switch (display_type_) {
    case k40Flaps:
      if (flap_index >= 40) {
        return 0;
      }
    case k62Flaps:
      if (flap_index >= 62) {
        return 0;
      }
  }
  for (uint8_t encoder = 1; encoder < 64; encoder++) {
    if (EncoderToPos(encoder) == flap_index) {
      return encoder;
    }
  }
  return 0;
}
