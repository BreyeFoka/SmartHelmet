#include <Wire.h>
#include <Adafruit_ADXL345_U.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// Pin Definitions
#define GSM_RX 7
#define GSM_TX 8
#define GPS_RX 4
#define GPS_TX 3
#define BUZZER_PIN 9
#define LED_PIN 10

// Constants
#define CRASH_THRESHOLD 10000 // Adjust based on sensitivity (raw value)
#define SPEED_LIMIT 60 // Speed limit in km/h

// Objects
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345); // I2C address: 0x53
SoftwareSerial gsmSerial(GSM_RX, GSM_TX); // RX, TX for GSM module
SoftwareSerial gpsSerial(GPS_RX, GPS_TX); // RX, TX for GPS module
TinyGPSPlus gps;

// Flags
bool crashDetected = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  gsmSerial.begin(9600); // Initialize GSM module serial communication
  gpsSerial.begin(9600); // Initialize GPS module serial communication

  // Initialize accelerometer
  if (!accel.begin()) {
    Serial.println("ADXL345 not found");
    while (1);
  }

  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // GSM initialization
  sendGSMCommand("AT");
  sendGSMCommand("AT+CMGF=1"); // Set SMS mode to text
}

void loop() {
  // Accelerometer data processing
  sensors_event_t event;
  accel.getEvent(&event);

  // Crash detection
  if (!crashDetected && detectCrash(event)) {
    crashDetected = true;
    // Trigger emergency alert
    sendEmergencyAlert();
  }

  // GPS data processing
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Speed warning
  if (gps.location.isValid()) {
    float speed = gps.speed.kmph();
    if (speed > SPEED_LIMIT) {
      triggerSpeedWarning();
    }
  }
}

bool detectCrash(sensors_event_t event) {
  // Check if any axis exceeds the threshold
  return abs(event.acceleration.x) > CRASH_THRESHOLD || abs(event.acceleration.y) > CRASH_THRESHOLD || abs(event.acceleration.z) > CRASH_THRESHOLD;
}

void sendEmergencyAlert() {
  if (gps.location.isValid()) {
    String message = "Crash detected! Location: ";
    message += "https://maps.google.com/?q=";
    message += String(gps.location.lat(), 6);
    message += ",";
    message += String(gps.location.lng(), 6);

    sendGSMCommand("AT+CMGS=\"+1234567890\""); // emergency contact number
    gsmSerial.print(message);
    gsmSerial.write(26); //  CTRL+Z to send the SMS
  }
}

void triggerSpeedWarning() {
  // Activate buzzer or LEDs for speed warning
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  delay(1000); // Adjust as needed
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
}

void sendGSMCommand(String command) {
  gsmSerial.println(command);
  delay(1000);
  while (gsmSerial.available()) {
    Serial.print(char(gsmSerial.read()));
  }
}
