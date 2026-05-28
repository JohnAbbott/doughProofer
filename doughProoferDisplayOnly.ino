/*
 * squarelinestudio_lvgl
 *
 * This example demonstrates the integration of a SquareLine Studio project with the Arduino GIGA Display Shield.
 * SquareLine Studio is an easy-to-use drag-and-drop UI editor tool for LVGL.
 *
 * Instructions:
 * 1. Create a SquareLine Studio project with the following settings:
 *    - Resolution: 800x480
 *    - Color depth: 16-bit
 *    - LVGL version: 8.3.x (NOTE: It only supports LVGL version 8 and earlier.)
 * 2. Design your GUI using the drag-and-drop tool.
 * 3. Export the LVGL UI files.
 * 4. Open the exported file and copy the 'libraries/ui' folder into your 'Arduino/libraries' directory.
 * 
 * Inside the sketch folder, you will find a preconfigured example of the 'ui' folder, exported from SquareLine Studio.
 * This example displays a white screen with a clickable blue button in the center.
 *  
 * Initial author: Leonardo Cavagnis @leonardocavagnis
 */

#include "Arduino_H7_Video.h"
#include "Arduino_GigaDisplayTouch.h"

#include "lvgl.h"
#include "ui.h"
#include "DHT.h"

static const char APP_VERSION[] = "1.1";
static const char VERSION_LABEL[] = "Rainbird Baking 1.1 - May 2026";

static const uint8_t DHTPIN = 4;     // DHT22 connected to pin 4
static const uint8_t HEAT_RELAY_PIN = 6;
static const uint8_t LIGHT_RELAY_PIN = 7;
static const bool RELAY_ACTIVE_HIGH = true;

static const float BREAD_TARGET_F = 85.0;
static const float CROISSANT_TARGET_F = 100.0;
static const float TEMP_HYSTERESIS_F = 1.0;

#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

enum ProofProfile {
    PROOF_BREAD,
    PROOF_CROISSANT
};

bool proofingRequested = false;
bool proofingActive = false;
bool heatRelayOn = false;
unsigned long proofStartAt = 0;

void setRelay(uint8_t pin, bool on)
{
    digitalWrite(pin, on == RELAY_ACTIVE_HIGH ? HIGH : LOW);
}

void setHeatRelay(bool on)
{
    if (heatRelayOn == on) {
        return;
    }

    heatRelayOn = on;
    setRelay(HEAT_RELAY_PIN, on);
    Serial.print("Heat relay: ");
    Serial.println(on ? "ON" : "OFF");
}

void stopProofing()
{
    proofingRequested = false;
    proofingActive = false;
    proofStartAt = 0;
    setHeatRelay(false);
    lv_label_set_text(ui_startButtonLabel, "Start!");
}

unsigned long selectedDelayMs()
{
    static const unsigned long delayOptionsMs[] = {
        30UL * 60UL * 1000UL,
        60UL * 60UL * 1000UL,
        90UL * 60UL * 1000UL,
        120UL * 60UL * 1000UL
    };

    uint16_t selected = lv_dropdown_get_selected(ui_delayStartAmount);
    if (selected >= sizeof(delayOptionsMs) / sizeof(delayOptionsMs[0])) {
        selected = 0;
    }

    return delayOptionsMs[selected];
}

ProofProfile selectedProfile()
{
    return lv_dropdown_get_selected(ui_proofType) == 1 ? PROOF_CROISSANT : PROOF_BREAD;
}

float targetTemperatureF()
{
    return selectedProfile() == PROOF_CROISSANT ? CROISSANT_TARGET_F : BREAD_TARGET_F;
}

void startProofing()
{
    proofingRequested = true;
    proofingActive = !lv_obj_has_state(ui_delayStart, LV_STATE_CHECKED);
    proofStartAt = proofingActive ? millis() : millis() + selectedDelayMs();
    lv_label_set_text(ui_startButtonLabel, proofingActive ? "Stop" : "Armed");
}

void startButtonEvent(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }

    if (proofingRequested) {
        stopProofing();
    } else {
        startProofing();
    }
}

void updateLights()
{
    setRelay(LIGHT_RELAY_PIN, lv_obj_has_state(ui_LightsOn, LV_STATE_CHECKED));
}

void updateProofing(float temperatureF)
{
    if (!proofingRequested) {
        setHeatRelay(false);
        return;
    }

    if (!proofingActive && millis() >= proofStartAt) {
        proofingActive = true;
        lv_label_set_text(ui_startButtonLabel, "Stop");
    }

    if (!proofingActive) {
        setHeatRelay(false);
        return;
    }

    const float targetF = targetTemperatureF();
    if (temperatureF <= targetF - TEMP_HYSTERESIS_F) {
        setHeatRelay(true);
    } else if (temperatureF >= targetF + TEMP_HYSTERESIS_F) {
        setHeatRelay(false);
    }
}

/* Initialize the GIGA Display Shield with a resolution of 800x480 pixels */
Arduino_H7_Video Display(480, 800, GigaDisplayShield);
Arduino_GigaDisplayTouch Touch;

#if (LVGL_VERSION_MAJOR >= 9)
    #error "SquareLine Studio has ended its collaboration with LVGL. It only supports LVGL version 8 and earlier."
#endif

void setup() {
    pinMode(HEAT_RELAY_PIN, OUTPUT);
    pinMode(LIGHT_RELAY_PIN, OUTPUT);
    setRelay(HEAT_RELAY_PIN, false);
    setRelay(LIGHT_RELAY_PIN, false);

    Display.begin();
    Touch.begin();

    /* Initialize the user interface designed with SquareLine Studio */
    ui_init();
    lv_label_set_text(ui_VersionInfo, VERSION_LABEL);
    lv_obj_add_event_cb(ui_startButton, startButtonEvent, LV_EVENT_ALL, NULL);
    dht.begin();
    Serial.begin(9600);
    Serial.print("Dough proofer firmware ");
    Serial.println(APP_VERSION);
}

void loop() {
    /* Feed LVGL engine */
    lv_timer_handler();

  static unsigned long lastUpdate = 0;
  updateLights();

    if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();
    
    // Read sensor data
    float h = dht.readHumidity();
    float f = dht.readTemperature(true); // Fahrenheit
    
    if (!isnan(h) && !isnan(f)) {
      //temp = f; // Update cloud variable
      
      // Update display
      char tempBuf[10];
      char humidBuf[10];
      int f_int = (int)f;
      int h_int = (int)h;
      //sprintf(tempBuf, "%f°", trunc(f));
      sprintf(tempBuf, "%d", f_int);
      sprintf(humidBuf, "%d%%", h_int);
      lv_label_set_text(ui_TemperatureValue,tempBuf);
      lv_label_set_text(ui_humidity,humidBuf);
      Serial.print("Temperature is:  ");
      Serial.println(tempBuf);
      Serial.print("Humidity is:  ");
      Serial.println(humidBuf);
      Serial.print("Target temperature is:  ");
      Serial.println(targetTemperatureF());

      updateProofing(f);

    } else {
      Serial.println("DHT22 read failed; heat relay forced off.");
      setHeatRelay(false);
    }
    }
}
