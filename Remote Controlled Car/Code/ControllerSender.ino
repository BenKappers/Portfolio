#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <MPU6050.h>

//RCCar esp32 MAC: EC:E3:34:A3:AF:5C
//Controller esp32 MAC: EC:E3:34:6B:5D:0C

uint8_t receiverMAC[] = {0xEC, 0xE3, 0x34, 0xA3, 0xAF, 0x5C}; // store the MAC of the RCCar reveicer

//define a struct that we will send as a "packet" to the car
typedef struct{
  int roll_angle;
  int pitch_angle;
  bool button;
} ControlData;

ControlData outgoing; // make an instance of the "packet"

int buttonPin = 4; // define button pin

MPU6050 mpu; // make an instance of mpu6050

//intialize the offset values for calibrating the IMU
float ax_offset = 0, ay_offset = 0, az_offset = 0;
float gx_offset = 0, gy_offset = 0, gz_offset = 0;

const int CALIBRATION_READINGS = 500; // how many calibration readings to take

// initialize variables
float roll = 0;
float pitch = 0;
float dt = 0.1;

float alpha = 0.50; // for the complimentary filter

//calibrates the sensor
void calibrateSensor() {
  long ax_sum = 0, ay_sum = 0, az_sum = 0;
  long gx_sum = 0, gy_sum = 0, gz_sum = 0;

  for (int i = 0; i < CALIBRATION_READINGS; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    ax_sum += ax;
    ay_sum += ay;
    az_sum += az;
    gx_sum += gx;
    gy_sum += gy;
    gz_sum += gz;

    delay(5); // short delay between readings
  }

  // takes the average
  ax_offset = ax_sum / (float)CALIBRATION_READINGS;
  ay_offset = ay_sum / (float)CALIBRATION_READINGS;
  az_offset = az_sum / (float)CALIBRATION_READINGS;
  gx_offset = gx_sum / (float)CALIBRATION_READINGS;
  gy_offset = gy_sum / (float)CALIBRATION_READINGS;
  gz_offset = gz_sum / (float)CALIBRATION_READINGS;

  az_offset -= 16384.0; //because gravity should be fully in the z
}

void setup() {

  Serial.begin(115200); // writes to the serial monitor

  //Calibrate IMU
  pinMode(2, OUTPUT); // initializes built in LED 
  Wire.begin(21, 22); // initializes I2C commuication for the MPU6050
  mpu.initialize(); // intializes the MPU6050

  digitalWrite(2, HIGH); // turn on the LED while calibrating
  calibrateSensor();
  digitalWrite(2, LOW); // turn off LED when done

  pinMode(buttonPin, INPUT_PULLUP); // initialize button pin

  WiFi.mode(WIFI_STA); // needed for ESP-NOW
  
  // initialize ESP-NOW
  if (esp_now_init() != ESP_OK){
    Serial.println("ESP-NOW init failed");
    return;
  }

// define peer info (car)
  esp_now_peer_info_t peer = {}; 
  memcpy(peer.peer_addr, receiverMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;

// add peer (car)
  esp_now_add_peer(&peer);
  Serial.println("Sender ready");

}

void loop() {

  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz); // get raw readings from MPU6050

// subtract offset and convert to "g's"
  float axg = (ax - ax_offset) / 16384.0;
  float ayg = (ay - ay_offset) / 16384.0;
  float azg = (az - az_offset) / 16384.0;

  float gx_rate = (gx - gx_offset)/131.0;
  float gy_rate = (gy - gy_offset)/131.0;


  float roll_acc;

  // calculates pitch angle from accelerometer
  float pitch_acc = atan2(-axg, sqrt(ayg * ayg + azg * azg + 1e-6)) * 180 / PI;

  // When the pitch gets close to 90 degrees the roll starts jumping
  if (abs(pitch_acc) > 80) {
    roll_acc = roll;   // keep previous roll
  } else {
    roll_acc = atan2(ayg, azg) * 180 / PI; // calucate roll from accelerometer
  }

// complementary filter to combine the acceleromter and gyro data
  roll = (alpha)* (roll + gx_rate * dt) + ( (1-alpha) * roll_acc);
  pitch = (alpha)* (pitch + gy_rate * dt) + ( (1-alpha) * pitch_acc);

// clamp angles to +- 60 degrees. Don't want to have to tilt the controller further than that
  if (pitch > 60){
    pitch = 60;
  }else if(pitch < -60){
    pitch = -60;
  }
  
  if (roll > 60){
    roll = 60;
  }else if(roll < -60){
    roll = -60;
  }

// check if the button is being pressed
  bool pressed = digitalRead(buttonPin) == LOW;

//fill in our packet with the data
  outgoing.button = pressed;
  outgoing.roll_angle = roll;
  outgoing.pitch_angle = pitch;

// send the packet to the car
  esp_now_send(receiverMAC, (uint8_t *)&outgoing, sizeof(outgoing));
  Serial.println("Sent Packet");

  delay(100); // wait 100 ms. 10 Hz update rate.

}
