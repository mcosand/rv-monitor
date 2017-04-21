/*
 * Project RV-Monitor
 * Description:
 * Author: Matt Cosand
 */
#include "SparkFunRHT03/SparkFunRHT03.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_ADS1015.h"
#include "pages.h"

const float BATTERY_DIVIDER = 29.0/*Kohms*/ / (29.0/*Kohms*/ + 148.9/*Kohms*/);
const float BATTERY_SCALAR = 3.3/*volts*/ / 4095 /*bits*/ / BATTERY_DIVIDER;

// AmpsPerBit = Instant amperage per bit at ADC. (100A / 50mV) [shunt values] * (256mV / 32768 bits) [from datasheet at 16X gain] = 0.0156...
// Bits to AmpHours = AmpsPerBit * BATTERY_SAMPLE_ms * (1s / 1000 ms) * (1min / 60s) * (1hr / 60min)
// Find reciprocal so we can store a big int instead of small fraction

// 50 mV   32768 bits            1             1000 ms   60 s    60 min     230400000 bits            1
// ----- * ---------- * -------------------- * ------- * ----- * ------ =   -------------- * --------------------
// 100 A     256 mV       BATTERY_SAMPLE ms      1 s     1 min    1 hr         1 A hr          BATTERY_SAMPLE ms
#define BATTERY_SAMPLE_ms 500
#define BATTERY_USABLE_CAPACITY_AH 100
const unsigned int BATTERY_AMP_HOURS_TO_BITS = 230400000 / BATTERY_SAMPLE_ms;
#define BATTERY_BITS_TO_AMPS 64

// PINS
#define I2C_SDA D0
#define I2C_SCL D1
#define NUM_BUTTONS 3
const int BUTTON_PINS[NUM_BUTTONS] = {D2, D3, D4};
#define BUZZER_PIN D6
#define LED_PIN D7

#define RHT03_DATA_PIN C0

#define BATTERY_VOLTS_PIN A5


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
Adafruit_ADS1115 ads;
uint16_t last_battery_draw = 0;
float last_battery_volts = 0.0F;
// assume battery starts out full
double battery_capacity = BATTERY_USABLE_CAPACITY_AH * BATTERY_AMP_HOURS_TO_BITS;

unsigned long last_battery_ms = 0;

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

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
  power_page.SetPrevious(&settings_page);

  lcd.begin(SSD1306_SWITCHCAPVCC, LCD_ADDR);
  lcd.clearDisplay();
  _pmic.disableCharging();

  Serial1.begin(9600);
  log("Start " + Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL));

  rht.begin(RHT03_DATA_PIN);

  ads.setGain(GAIN_SIXTEEN);
  ads.begin();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, 0);

  for (int i=0;i<NUM_BUTTONS;i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLDOWN);
    last_button_down[i] = 0;
  }
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

  if (Time.now() - RHT_SAMPLE_RATE > rht_updated)
  {
    if (rht.update() == 1) {
      rht_updated = Time.now();
    }
  }

  if (millis() - last_battery_ms > BATTERY_SAMPLE_ms) {
    last_battery_draw = ads.readADC_Differential_0_1();
    last_battery_volts = analogRead(BATTERY_VOLTS_PIN) * BATTERY_SCALAR;
    battery_capacity += last_battery_draw;
    last_battery_ms = millis();
  }
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
  lcd.clearDisplay();
  lcd.setTextColor(WHITE);
  lcd.setTextSize(1);
  lcd.setCursor(0,0);
  lcd.println("Power");
  lcd.println("");
  lcd.println("Volts: " + String(last_battery_volts, 2) + "V");
  lcd.println("Draw:  " + String((float)last_battery_draw / (float)BATTERY_BITS_TO_AMPS, 3) + "A");
  lcd.println(String(battery_capacity / BATTERY_AMP_HOURS_TO_BITS, 1) + "AH remaining");
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
