//#define DEBUG
#define SERIAL_UI

// global constants
static const int SECS_IN_MINUTE = 60;
static const int MINUTES_IN_HOUR = 60;
// change this to set the default max speed
#define MAX_SPEED_KM_PER_SEC 20.0

char *BANNER = "KITE: version 0.4 09/08/17\n";
class PARAMETERS {
public:
  static const long BAUD = 115200;
  static const long WS_UPDATE_SEC = 1;
  static const float WHISPER500_MAX_SPEED_KM_PER_MS = (MAX_SPEED_KM_PER_SEC/1000); // determines alarm threshold
                                                 // to throw panic switch
  static const float ANEMOMETER__MAGIC_CONSTANT = 2.5;
  static const long UI_UPDATE_SEC = WS_UPDATE_SEC * 10;
  //  static const int HISTORY_LENGTH = SECS_IN_MINUTE * MINUTES_IN_HOUR;
  static const int HISTORY_LENGTH = SECS_IN_MINUTE;

  static int ErrorCount;
  static void setup() {
    ErrorCount = 0;
  }
};

int PARAMETERS::ErrorCount;

class WIRING {
public:
  static const int OPTICAL_ISSOLATOR_PIN = 7;
  static const int ANEMOMETER_PIN = 8;
  static const int ANEMOMETER_LED_PIN = 13;
};

class History {
  float history_[PARAMETERS::HISTORY_LENGTH];
  int cur_;
  float accum_;
  bool valid_;
public:
  History() : cur_(0.0), accum_(0.0), valid_(false) {}

  inline void  recordValue(float val) {
    // store new value into window history of values
    float oldest = history_[cur_];
    history_[cur_] = val;
    cur_++;
    if (cur_ == PARAMETERS::HISTORY_LENGTH) { 
      cur_=0;
      valid_ = true;
    }

    // updated running accumulation
    accum_ -= oldest;  // substract out oldest value from accum
    accum_ += val;     // add in new value to accum
  }

  inline float value() { return accum_; }

  inline float avg() {  
    if (valid_) return accum_ / PARAMETERS::HISTORY_LENGTH; 
    else return 0.0;
  }
};


class Anemometer {
  int pin_;
  int ledPin_;

  int lastState_;
  long risingEdgeCount_;
  long fallingEdgeCount_;

  void flash(int count) {
    for(int i=0;i<count;i++) {
      digitalWrite(ledPin_, HIGH);
      delay(100);
      digitalWrite(ledPin_,LOW);
      delay(100);
    }
  }


public:
  Anemometer(int pin, int ledPin) : 
    pin_(pin), ledPin_(ledPin), 
    risingEdgeCount_(0), fallingEdgeCount_(0) {} 


  void setup(long now) {
    pinMode(ledPin_, OUTPUT);

#ifdef DEBUG
    Serial.println(__PRETTY_FUNCTION__);
    Serial.println(pin_);
    Serial.println(ledPin_);
    flash(20);
#endif
    pinMode(pin_, INPUT);
    lastState_ = digitalRead(pin_);
#ifdef DEBUG
    Serial.println("Aneometer::setup: DONE");
#endif
   }

  void update() {
    int newState = digitalRead(pin_);
    if (newState != lastState_) {
      if (newState == HIGH) risingEdgeCount_++;
      else fallingEdgeCount_++;
      digitalWrite(ledPin_, newState);
    }
    lastState_ = newState;
  }

  inline float windSpeed(float intervalMS) {
    float ws = (PARAMETERS::ANEMOMETER__MAGIC_CONSTANT * 
		(float)fallingEdgeCount_) / intervalMS;  

#ifdef DEBUG
    Serial.print("AN::windSpeed: fCnt: ");
    Serial.print(fallingEdgeCount_);
    Serial.print(" intervalMS: ");
    Serial.print(intervalMS);
    Serial.print(" ws: ");
    Serial.println(ws);
#endif

    // reset edgecount we are using to zero so that calculation
    // will be computed based on the count since the last invocation
    fallingEdgeCount_ = 0;
    return ws;
  }

};

class WindMill {
public:
   struct WindMillState {
     History history;
     float currentWS;	
     float maxWS;
     float minWS;
     WindMillState() : currentWS(0.0), maxWS(0.0), minWS(0.0) {}
   } State; 
protected:
  Anemometer an_;
  // max speedin km/ms
  float maxSpeed_;
  // update frequence values
  long wsUpdateMS_;
  long lastNow_;

public:
  WindMill(int anPin, int anLedPin, float maxSpeed) : 
    an_(anPin,anLedPin), maxSpeed_(maxSpeed), 
    lastNow_(0),
    wsUpdateMS_(PARAMETERS::WS_UPDATE_SEC * 1000)
  {}

  float getMaxSpeed() { return maxSpeed_; }
  
  inline bool isAlarm() { 
    // adjust to the logic you want
    //    return State.currentWS >= maxSpeed_;
    return  State.history.avg() >= maxSpeed_;
  };

  inline void update(long now) { 
    an_.update();
    long elapsed = now - lastNow_;

    if ( elapsed >= wsUpdateMS_) {
      State.currentWS = an_.windSpeed(elapsed);
      State.history.recordValue(State.currentWS);

      // update stats
      if (State.currentWS < State.minWS) State.minWS = State.currentWS;
      if (State.currentWS > State.maxWS) State.maxWS = State.currentWS;
    
#ifdef DEBUG
      Serial.print("wm::update elapsed:");
      Serial.print(elapsed);
      Serial.print(" currentWS: ");
      Serial.print(State.currentWS * 1000);
      Serial.print(" min:");
      Serial.print(State.minWS * 1000);
      Serial.print(" max:");
      Serial.println(State.maxWS * 1000);
#endif
      lastNow_ = now;
      if (elapsed != wsUpdateMS_) PARAMETERS::ErrorCount++;
    }
  }

  void setup(long now) {
#ifdef DEBUG
    Serial.println(__PRETTY_FUNCTION__);
#endif
    an_.setup(now);
  }
};

class Whisper500 : public WindMill {
public:
  Whisper500(int anPin, int anLedPin) : 
    WindMill(anPin, anLedPin, 
    PARAMETERS::WHISPER500_MAX_SPEED_KM_PER_MS) {}
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
Whisper500 windMill(WIRING::ANEMOMETER_PIN,
		    WIRING::ANEMOMETER_LED_PIN);

class SerialUI {
  long lastNow_;
public:		
  void setup(long now)  {
    lastNow_ = now;
    Serial.begin(PARAMETERS::BAUD);
    Serial.println(BANNER);
  }

  inline void update(long now, bool alarm) {
    if (now - lastNow_ >= PARAMETERS::UI_UPDATE_SEC * 1000) {
      if (alarm) Serial.print("** ALARM ** : ");
      if (PARAMETERS::ErrorCount)  {
	Serial.print(PARAMETERS::ErrorCount);
        Serial.print(" ");
      }
      Serial.print("CurrentSpeed: ");
      Serial.print(windMill.State.currentWS * 1000);
      Serial.print("\t[");
      Serial.print(windMill.State.minWS * 1000);
      Serial.print(":");
      Serial.print(windMill.State.maxWS * 1000);
      Serial.print("] ");
      Serial.print("history avg: ");
      Serial.print(windMill.State.history.avg() * 1000);
      Serial.print(" max:");
      Serial.println(windMill.getMaxSpeed() *1000);
      lastNow_ = now;
    }
  }
};

class LcdUI {
public:		
  void setup() {
    // do lcd specific setup
  }
  inline void update(long now, bool alarm) {
  }
};

SerialUI serialUI;
LcdUI    lcdUI;

/// START OF MAIN ARDUINO LOGIC 
void setup() 
{
  long now;

  PARAMETERS::setup();
  // UI setup
#ifdef SERIAL_UI
  now = millis();
  serialUI.setup(now);
#endif
  lcdUI.setup();

  panicSW.setup();
  now = millis();
  windMill.setup(now);
}

bool alarm = false;

void loop() 
{
  long now = millis();

  // sense
  windMill.update(now);  

  // think and act
  if (alarm == false && windMill.isAlarm()) { 
    panicSW.panic();
    alarm = true;
  }

  // Deal with UI
#ifdef SERIAL_UI
    serialUI.update(now,alarm);
#endif
    lcdUI.update(now,alarm);
} 


