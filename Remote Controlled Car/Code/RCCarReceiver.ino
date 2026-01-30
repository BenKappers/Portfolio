#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

//RCCar esp32 MAC: EC:E3:34:A3:AF:5C
//Controller esp32 MAC: EC:E3:34:6B:5B:0C

// PWM Pins
const int RPWM_PIN = 25;
const int LPWM_PIN = 26;

// PWM settings
const int PWM_FREQ = 20000;   // 20 kHz
const int PWM_RES  = 8;       // 0–255

Servo servo; // make a servo object

// define same struct that the controller sends as a packet
typedef struct {
  int roll_angle;
  int pitch_angle;
  bool button;
} ControlData;

ControlData incoming; // create an instance of the "packet"


void setup() {

  Serial.begin(115200); // writes to the serial monitor

  WiFi.mode(WIFI_STA);  // set ESP32 wifi mode. needed to use ESP-NOW

// initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);
  Serial.println("Receiver ready");

  // define servo pin
  servo.attach(14);
  servo.writeMicroseconds(1500); // center the sevo

  // set up the motor
  ledcAttach(RPWM_PIN, PWM_FREQ, PWM_RES);
  ledcAttach(LPWM_PIN, PWM_FREQ, PWM_RES);

  // Stop motor
  ledcWrite(RPWM_PIN, 0);
  ledcWrite(LPWM_PIN, 0);

}


// this function runs every time a packet is recieved from the controller
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {

  memcpy(&incoming, data, sizeof(incoming)); //copy info from recieved packet into our packet so we can use the data
  int controller_roll_angle = incoming.roll_angle;
  int controller_pitch_angle = incoming.pitch_angle;
  int controller_pressed = incoming.button;

  setSteerPosition(controller_roll_angle); // always update the steer angle

// only turn on the motor if the button is being pressed
  if( (controller_pressed == 1) && (abs(controller_pitch_angle) > 3) ){
    setMotorSpeed(controller_pitch_angle);
  }else{
    ledcWrite(LPWM_PIN, 0);
    ledcWrite(RPWM_PIN, 0);
  }

// useful for debugging
  Serial.print("Roll angle: ");
  Serial.print(incoming.roll_angle);
  Serial.print(" | Pitch Angle: ");
  Serial.println(incoming.pitch_angle);
  Serial.print(" | Pressed: ");
  Serial.println(incoming.button);
}

//sets the servo steer angle
void setSteerPosition(int controller_roll_angle) {
  //int servo_angle = map(controller_roll_angle, -80, 80, 65, 115);
  int microseconds = map(controller_roll_angle, -60, 60, 1200, 1800);
  servo.writeMicroseconds(microseconds);
}

// sets the motor speed
void setMotorSpeed(int controller_pitch_angle){
  int duty_cycle = map(abs(controller_pitch_angle), 0, 60, 50, 200);
  if (controller_pitch_angle < 0){
    ledcWrite(LPWM_PIN, 0);
    ledcWrite(RPWM_PIN, duty_cycle);
  }else{
    ledcWrite(LPWM_PIN, duty_cycle);
    ledcWrite(RPWM_PIN, 0);
  }

}


void loop() {
}
