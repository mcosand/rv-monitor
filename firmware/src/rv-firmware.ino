/*
 * Project RV-Monitor
 * Description:
 * Author: Matt Cosand
 */
#include "SparkFunRHT03/SparkFunRHT03.h"
//#define NO_CELL 1

// QUICK
/*
const int MOTION_DEBOUNCE_SECONDS = 10;
const int SENSORS_INTERVAL_SECONDS = 60;
const int SLEEP_PERIOD = 20;
*/
// MEDIUM
/*
const int MOTION_DEBOUNCE_SECONDS = 60;
const int SENSORS_INTERVAL_SECONDS = 60 * 30;
const int SLEEP_PERIOD = 30;
*/

// LONG
const int MOTION_DEBOUNCE_SECONDS = 60 * 30;
const int SENSORS_INTERVAL_SECONDS = 60 * 60 * 8;
const int SLEEP_PERIOD = 60;

const float BATTERY_DIVIDER = 29.0/*ohms*/ / (29.0/*ohms*/ + 148.9/*ohms*/);
const float BATTERY_SCALAR = 3.3/*volts*/ / 4095 /*bits*/ / BATTERY_DIVIDER;

const int LED_PIN=D7;
const int BATTERY_PIN=A5;
const int RHT03_DATA_PIN = D3;
const int MOTION_SIGNAL_PIN = D0;

#ifdef NO_CELL
const int EARLY_TIME = 0;
#else
const int EARLY_TIME = 1000000000;
#endif

RHT03 rht;
int lastTime;
int lastSensors;
int lastMotion;

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

// setup() runs once, when the device is first turned on.
void setup() {
  int i;

  pinMode(MOTION_SIGNAL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Call rht.begin() to initialize the sensor and our data pin
  rht.begin(RHT03_DATA_PIN);

  // Put initialization like pinMode and begin functions here.
#ifdef NO_CELL
  Serial.begin(9600); // Serial is used to print sensor readings.
#else
  Cellular.connect();
  while (!Cellular.ready()) delay(200);
  if (!Particle.connected()) Particle.connect();
  while (!Particle.connected()) delay(200);
#endif

  for (i=0;i<10;i++) {
    digitalWrite(LED_PIN, 1);
    delay(400);
    digitalWrite(LED_PIN, 0);
    delay(600);
  }
}

void log(String s) {
#ifdef NO_CELL
  Serial.println(s);
#endif
}

void send(String msg, String data) {
#ifdef NO_CELL
  Serial.println("### " + msg + ": " + data + " ###");
#else
  Particle.publish(msg, data, PRIVATE, NO_ACK);
#endif
  Particle.process();
  delay(1000);
}

void doSleep(int seconds) {
#ifdef NO_CELL
	int start = Time.now();
	bool wasQuiet = digitalRead(MOTION_SIGNAL_PIN) == 0;
	while (Time.now() < start + seconds) {
		if (wasQuiet && digitalRead(MOTION_SIGNAL_PIN) == 1) break;
		wasQuiet = digitalRead(MOTION_SIGNAL_PIN) == 0;
	}
#else
	System.sleep(MOTION_SIGNAL_PIN, RISING, seconds, SLEEP_NETWORK_STANDBY);
#endif
}

void doSensors() {
	int updateRet = rht.update();
	if (updateRet == 1)
	{
		float latestHumidity = rht.humidity();
		float latestTempF = rht.tempF();
    int batteryBits = analogRead(BATTERY_PIN);
    float latestBatt = batteryBits * BATTERY_SCALAR;
		// Now print the values:
    send("s", String(latestBatt, 2) +"," + String(latestTempF, 1) + "," + String(latestHumidity, 1));
	}
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
	int now;

	lastTime = Time.now();
	log("sleeping");
	doSleep(SLEEP_PERIOD);
	now = Time.now();
	log(String(now));
	if (now - lastTime < SLEEP_PERIOD) {
	  log("Because of motion");
	  if (now - lastMotion > MOTION_DEBOUNCE_SECONDS) {
      send("m", "");
  		lastMotion = now;
	  }
	}

	if (now > EARLY_TIME && now - lastSensors > SENSORS_INTERVAL_SECONDS) {
		doSensors();
		lastSensors = now;
	}
}
