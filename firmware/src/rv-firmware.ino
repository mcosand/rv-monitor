/*
 * Project RV-Monitor
 * Description:
 * Author: Matt Cosand
 */
#include "SparkFunRHT03/SparkFunRHT03.h"
#include "Adafruit_SSD1306.h"
#include "pages.h"

const float BATTERY_DIVIDER = 29.0/*ohms*/ / (29.0/*ohms*/ + 148.9/*ohms*/);
const float BATTERY_SCALAR = 3.3/*volts*/ / 4095 /*bits*/ / BATTERY_DIVIDER;

// PINS
#define I2C_SDA D0
#define I2C_SCL D1
#define NUM_BUTTONS 3
const int BUTTON_PINS[NUM_BUTTONS] = {D2, D3, D4};
#define BUZZER_PIN D6
#define LED_PIN D7

#define RHT03_DATA_PIN C0


#define LCD_ADDR 0x3C
#define ADC_ADDR 0x48 // ADDR to GND

const int MOTION_HIGH_TIME = 5;
const int TEMP_MEASURE_INTERVAL = 10;

#ifdef NO_CELL
const int EARLY_TIME = 0;
#else
const int EARLY_TIME = 1000000000;
#endif

PMIC _pmic;
RHT03 rht;
Adafruit_SSD1306 lcd(0);

int rht_updated = 0;
#define RHT_SAMPLE_RATE 5
#define SCREEN_DIM_TIME 30
#define SCREEN_TIMEOUT 40
#define ACTION_TO_SLEEP (SCREEN_TIMEOUT + 5)
#define BUTTON_DEBOUNCE_ms 300

int last_action;
int next_sleep;

unsigned long last_button_down[NUM_BUTTONS];

Page power_page(NULL, drawPower);
Page tanks_page(&power_page, drawTanks);
Page env_page(&tanks_page, drawEnvironment);
String menu_names[] = { "abc", "def" };
SettingsPage settings_page(&env_page, &lcd, menu_names, 2);
Page* current_screen = &power_page;

/*
String logMsg = NULL;
unsigned long last_motion = 0;
unsigned long last_temp = 0;
unsigned long stop_beeping = 0;
*/


SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

/*

const int SLEEP_PERIOD = 60 * 1;
*/
void setup() {
  power_page.SetPrevious(&settings_page);

  lcd.begin(SSD1306_SWITCHCAPVCC, LCD_ADDR);
  lcd.clearDisplay();
  _pmic.disableCharging();

  Serial1.begin(9600);
  log("Start " + Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL));

  rht.begin(RHT03_DATA_PIN);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);

  for (int i=0;i<NUM_BUTTONS;i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLDOWN);
    last_button_down[i] = 0;
  }
  //attachInterrupt(BUTTON_PINS[2], handleButton, RISING);
/*
  // Put initialization like pinMode and begin functions here.
#ifndef NO_CELL
  Cellular.connect();
  while (!Cellular.ready()) delay(200);
  if (!Particle.connected()) Particle.connect();
  while (!Particle.connected()) delay(200);
#endif
*/
  last_action = Time.now();
  next_sleep = Time.now() + ACTION_TO_SLEEP;
}

void loop() {
  int action_delta = Time.now() - last_action;

  handleDisplayTimeout(action_delta);
  handleButton();
  if (lcd.isOn())
  {
    current_screen->Draw();
    lcd.display();
  }
/*
  if (active_button > 0)
  {
    //switch (current_screen) {
    //  case 0:
    //    if (active_button == 2) { current_screen = 1
    //}
    active_button = 0;
  }
*/
  if (Time.now() - RHT_SAMPLE_RATE > rht_updated)
  {
    if (rht.update() == 1) {
      rht_updated = Time.now();
    }
  }

/*
  if (logMsg != NULL) {
    log(logMsg);
    logMsg = NULL;
  }
  if (stop_beeping > 0 && millis() > stop_beeping) {
    stop_beeping = 0;
    digitalWrite(BUZZER_PIN, 0);
  }
  if (okToSleep())
  {
    unsigned long before = Time.now();
    log("Sleeping");
    System.sleep(WKP, RISING, SLEEP_PERIOD, SLEEP_NETWORK_STANDBY);
    before = Time.now() - before;
    for (int i=0;i<NUM_BUTTONS;i++) { last_button_down[i] = 0; }
    log(String(before));
    log(before < SLEEP_PERIOD ? "Wake up!" : "open one eye ...");
  }
  handleButton();
  if (digitalRead(MOTION_PIN) && Time.now() - last_motion > MOTION_HIGH_TIME) {
    last_motion = Time.now();
    beep(200);
    log("Motion!!");
  }
  if (Time.now() - last_temp > TEMP_MEASURE_INTERVAL) {
    if (rht.update() == 1) {
      log("Temp: " + String(rht.tempF(), 1) + "   Humidity: " + String(rht.humidity(), 1));
      last_temp = Time.now();
    }
  }
*/
}


void handleDisplayTimeout(int since_last)
{
  static int last_time;
  if (since_last < SCREEN_DIM_TIME) {
    lcd.on(true);
    lcd.dim(0);
  }
  else if (since_last >= SCREEN_DIM_TIME && last_time < SCREEN_DIM_TIME)
  {
    lcd.dim(1);
  }
  else if (since_last >= SCREEN_TIMEOUT && last_time < SCREEN_TIMEOUT)
  {
    lcd.on(false);
  }
  last_time = since_last;
}

void drawPower() {
  lcd.fillRect(0, 0, lcd.width(), 16, BLACK);
  lcd.setTextColor(WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(0,0);
  lcd.print("Power");
}

void drawEnvironment() {
  lcd.fillRect(0, 16, lcd.width(), 16 * 2, BLACK);
  lcd.setTextColor(WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(0,21);

  if (rht_updated == 0)
  {
    lcd.print("Please wait ...");
    return;
  }

  lcd.print("Temp: ");
  lcd.setCursor(0,37);
  lcd.print("Humid: ");
  lcd.setTextSize(2);
  lcd.setCursor(42,16);
  lcd.print(String(rht.tempF(), 0) + (char)247);
  lcd.setCursor(42,32);
  lcd.print(String(rht.humidity(), 0) + '%');
}

void drawTanks() {
  lcd.fillRect(0, 0, lcd.width(), 16, BLACK);
  lcd.setTextColor(WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(0,0);
  lcd.print("Tanks");
}

void handleButton() {
  for (int i=0; i<NUM_BUTTONS; i++)
  {
    if (digitalRead(BUTTON_PINS[i]) && millis() - last_button_down[i] > BUTTON_DEBOUNCE_ms)
    {
      last_button_down[i] = millis();
      last_action = Time.now();
      Page* new_screen = &power_page;

      if (last_action < next_sleep)
      {
        // the UI wasn't sleeping
        new_screen = current_screen->HandleButton(i);
      }
      next_sleep = last_action + ACTION_TO_SLEEP;

      if (new_screen == NULL) new_screen = &power_page;
      if (new_screen != current_screen) {
        lcd.clearDisplay();
        current_screen = new_screen;
      }
    }
  }
}
/*
bool okToSleep() {
  const unsigned long now = millis();
  for (int i=0;i<NUM_BUTTONS;i++)
  {
    if (now - last_button_down[i] < 10 * 1000) return false;
  }
  if (stop_beeping > 0) return false;
  return (digitalRead(MOTION_PIN) == 0);
}

void beep(int t) {
  digitalWrite(BUZZER_PIN, 1);
  stop_beeping = millis() + t;
  //delay(t);
  //digitalWrite(BUZZER_PIN, 0);
}
*/



/*
void logDebug() {
  Serial1.println("# SysBat: " + String(fuel.getVCell(), 2) + ", " + String(fuel.getSoC(), 1) + '%%');
  Serial1.println("# RSSI: " + String(Cellular.RSSI().rssi));



  CellularData data;
  if (!Cellular.getDataUsage(data)) {
      Serial1.print("!! Error Not able to get data.");
  }
  else {
      Serial1.printlnf("# CID: %d SESSION TX: %d RX: %d TOTAL TX: %d RX: %d",
          data.cid,
          data.tx_session, data.rx_session,
          data.tx_total, data.rx_total);
  }
}
*/

/*
void doSensors() {
  logDebug();
  int updateRet = rht.update();
  if (updateRet == 1)
  {
    float latestHumidity = rht.humidity();
    float latestTempF = rht.tempF();
    int batteryBits = analogRead(BATTERY_PIN);
    float latestBatt = batteryBits * BATTERY_SCALAR;

    String strength = Cellular.ready() ? String(map(Cellular.RSSI().rssi, -131, -51, 0, 9)) : "_";

    // Now print the values:
    send("s", strength + String(latestBatt, 2) +"," + String(latestTempF, 1) + "," + String(latestHumidity, 1));
  } else {
    log("!!Error reading temperature");
  }
}
*/
// loop() runs over and over again, as quickly as it can execute.


/*
  int now;

  lastTime = Time.now();
  Serial1.println(".");
  System.sleep(MOTION_SIGNAL_PIN, RISING, SLEEP_PERIOD, SLEEP_NETWORK_STANDBY);
  now = Time.now();
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
*/




void log(String s) {
  Serial1.println(Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL) + " " + s);
}

void send(String msg, String data) {
  log("### " + msg + ": " + data + " ###");
  if (Particle.connected()) {
    Particle.publish(msg, data, PRIVATE, NO_ACK);
    Particle.process();
  }
  delay(1000);
}
