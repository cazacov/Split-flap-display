#include "flapdisplayboard.h"

ESP32Timer FlapDisplayBoardHardware::itimer_(0);

DisplayDescriptor FlapDisplayBoardHardware::displays_[16];
uint8_t FlapDisplayBoardHardware::display_count_ = 0;
uint8_t FlapDisplayBoardHardware::start_pin_ = 0;
uint8_t FlapDisplayBoardHardware::adl_pin_ = 0;
uint8_t FlapDisplayBoardHardware::data0_pin_ = 0;
uint8_t FlapDisplayBoardHardware::data1_pin_ = 0;
uint8_t FlapDisplayBoardHardware::data2_pin_ = 0;
uint8_t FlapDisplayBoardHardware::data3_pin_ = 0;
uint8_t FlapDisplayBoardHardware::data4_pin_ = 0;
uint8_t FlapDisplayBoardHardware::data5_pin_ = 0;
#ifdef FLAPDISPLAY_DEBUG
std::vector<LogEntry> FlapDisplayBoardHardware::log;
#endif

uint8_t FlapDisplayBoardHardware::ReadEncoder(uint8_t adc_pin) {
  EnableModule(adc_pin);
  uint8_t input = 0;
  input += (digitalRead(data0_pin_) == LOW) ? 1 << 0 : 0;
  input += (digitalRead(data1_pin_) == LOW) ? 1 << 1 : 0;
  input += (digitalRead(data2_pin_) == LOW) ? 1 << 2 : 0;
  input += (digitalRead(data3_pin_) == LOW) ? 1 << 3 : 0;
  input += (digitalRead(data4_pin_) == LOW) ? 1 << 4 : 0;
  input += (digitalRead(data5_pin_) == LOW) ? 1 << 5 : 0;
  DisableModule(adc_pin);
  return input;
}

void FlapDisplayBoardHardware::init(uint8_t start_pin, uint8_t adl_pin, uint8_t data0_pin, uint8_t data1_pin, uint8_t data2_pin, uint8_t data3_pin, uint8_t data4_pin, uint8_t data5_pin) {
  start_pin_ = start_pin;
  adl_pin_ = adl_pin;
  data0_pin_ = data0_pin;
  data1_pin_ = data1_pin;
  data2_pin_ = data2_pin;
  data3_pin_ = data3_pin;
  data4_pin_ = data4_pin;
  data5_pin_ = data5_pin;
  display_count_ = 0;

  pinMode(data0_pin_, INPUT_PULLUP);
  pinMode(data1_pin_, INPUT_PULLUP);
  pinMode(data2_pin_, INPUT_PULLUP);
  pinMode(data3_pin_, INPUT_PULLUP);
  pinMode(data4_pin_, INPUT_PULLUP);
  pinMode(data5_pin_, INPUT_PULLUP);

  pinMode(start_pin_, OUTPUT);
  pinMode(adl_pin_, OUTPUT);
  digitalWrite(start_pin_, LOW);
  digitalWrite(adl_pin_, LOW);

#if USING_ESP32_S2_TIMER_INTERRUPT
  Serial.println("S2");
  Serial.println(ESP32_S2_TIMER_INTERRUPT_VERSION);
#else
  Serial.println("not S2");
  Serial.println(ESP32_TIMER_INTERRUPT_VERSION);
#endif

  if (itimer_.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, FlapDisplayBoardHardware::TimerHandler))
  {
    Serial.print(F("Starting  ITimer OK, millis() = ")); Serial.println(millis());
  }
  else {
    Serial.println(F("Can't set ITimer0. Select another freq. or timer"));  
  }
}

void FlapDisplayBoardHardware::EnableModule(uint8_t adc_pin) {
  digitalWrite(adl_pin_, HIGH);
  digitalWrite(adc_pin, HIGH);
//  delayMicroseconds(READ_DELAY_MICROS);
}

void FlapDisplayBoardHardware::DisableModule(uint8_t adc_pin) {
  digitalWrite(adl_pin_, LOW);
  digitalWrite(adc_pin, LOW);
}

void FlapDisplayBoardHardware::MotorStart(uint8_t adc_pin) {
  EnableModule(adc_pin);
  digitalWrite(start_pin_, HIGH);
  delayMicroseconds(READ_DELAY_MICROS);
  DisableModule(adc_pin);
  delayMicroseconds(READ_DELAY_MICROS);
  digitalWrite(start_pin_, LOW);
}

void FlapDisplayBoardHardware::MotorStop(uint8_t adc_pin) {
  EnableModule(adc_pin);
  delayMicroseconds(READ_DELAY_MICROS);
  DisableModule(adc_pin);
}

#if USING_ESP32_S2_TIMER_INTERRUPT
  void IRAM_ATTR FlapDisplayBoardHardware::TimerHandler(void * timerNo)
#else
  void IRAM_ATTR FlapDisplayBoardHardware::TimerHandler(void)
#endif
{
#if USING_ESP32_S2_TIMER_INTERRUPT
/////////////////////////////////////////////////////////
// Always call this for ESP32-S2 before processing ISR
TIMER_ISR_START(timerNo);
/////////////////////////////////////////////////////////
#endif
  unsigned long now = millis();
  for (size_t i = 0; i < display_count_; i++) {
    if (displays_[i].must_be_started) {
      displays_[i].must_be_started = false;
      MotorStart(displays_[i].adc_pin);
      displays_[i].is_counting = true;
#ifdef FLAPDISPLAY_DEBUG        
      log.clear();
#endif      
      continue;
    }
    if (displays_[i].is_counting && displays_[i].target_millis < now) {
      MotorStop(displays_[i].adc_pin);
      uint8_t current_encoder = ReadEncoder(displays_[i].adc_pin);
#ifdef FLAPDISPLAY_DEBUG        
      LogEntry entry;
      entry.millis = now;
      entry.encoder = current_encoder;
      log.push_back(entry);
#endif      
      if (current_encoder) {
        displays_[i].current_encoder = current_encoder;
      }
      if (current_encoder == displays_[i].target_encoder) {
        displays_[i].is_counting = false;
        Serial.print("Target encoder reached: ");
        Serial.println(displays_[i].target_encoder);
      }
      else {
        displays_[i].target_millis += 20;
        MotorStart(displays_[i].adc_pin);
      }
    }
  }
#if USING_ESP32_S2_TIMER_INTERRUPT
  /////////////////////////////////////////////////////////
  // Always call this for ESP32-S2 after processing ISR
  TIMER_ISR_END(timerNo);
  /////////////////////////////////////////////////////////
#endif  
}

FlapDisplay* FlapDisplayBoardHardware::CreateDisplay(DisplayType display_type, uint8_t adc_pin) {
  FlapDisplay* result = new FlapDisplay(display_type, adc_pin, displays_ + display_count_);
  display_count_++;
  MotorStop(adc_pin);
  return result;
}

void FlapDisplayBoardHardware::PrintLog() {
#ifdef FLAPDISPLAY_DEBUG    
  for (size_t i = 0; i < log.size(); i++) {
    Serial.print(log[i].millis);
    Serial.print("\t");
    Serial.println(log[i].encoder);
    if (i == 20) {
      break;
    }
  }
#endif  
}

FlapDisplayBoardHardware FlapDisplayBoard;