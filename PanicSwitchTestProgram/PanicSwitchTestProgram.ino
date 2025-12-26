class WIRING {
public:
  static const int OPTICAL_ISSOLATOR_PIN = 7;
  static const int ANEMOMETER_PIN = 8;
  static const int ANEMOMETER_LED_PIN = 13;
};

class PanicSwitch {
  int pin_;
  bool state_;
  static const int REMOTE_TOGGLE_DELAY=2000;
  static const int REMOTE_TOGGLE_REPEAT=3;
  
  inline void
  remote_toggle_button(int pin)
  {
    for (int i=0;i<REMOTE_TOGGLE_REPEAT;i++){
      digitalWrite(pin,HIGH);
      delay(REMOTE_TOGGLE_DELAY);
      digitalWrite(pin,LOW);
      delay(REMOTE_TOGGLE_DELAY);
      delay(500);
    }
  }
public:					     
  PanicSwitch(int pin) : pin_(pin) {}
  void panic() { remote_toggle_button(pin_); state_=true; }
  void reset() { state_=false; }
  void setup() { 
#ifdef DEBUG
    Serial.println(__PRETTY_FUNCTION__);
#endif
    pinMode(pin_, OUTPUT); 
    reset(); 
  }
};

// GLOBALS
PanicSwitch panicSW(WIRING::OPTICAL_ISSOLATOR_PIN); 


void setup()
{
  Serial.begin(115200);
  Serial.println("Panic switch test.  Turn switch on via remote and then use 1 to panic off.");
  Serial.println("Send 1 to panic on and 0 to reset");
  panicSW.setup();
}

char inByte;
void loop() {
  if (Serial.available() > 0) {
    inByte = Serial.read();
    Serial.println(inByte);
    if (inByte == '1') {
      Serial.println("Calling panic");
      panicSW.panic();
    } else if (inByte == '0') {
      Serial.println("Calling reset");
      panicSW.reset();
    } else {
      Serial.println("Only supports 1 for panic and 0 for reset");
    }
  }
}
