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
#define DHTPIN 4     // DHT22 connected to pin 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

/* Initialize the GIGA Display Shield with a resolution of 800x480 pixels */
Arduino_H7_Video Display(480, 800, GigaDisplayShield);
Arduino_GigaDisplayTouch Touch;

#if (LVGL_VERSION_MAJOR >= 9)
    #error "SquareLine Studio has ended its collaboration with LVGL. It only supports LVGL version 8 and earlier."
#endif

void setup() {
    Display.begin();
    Touch.begin();

    /* Initialize the user interface designed with SquareLine Studio */
    ui_init();
    dht.begin();
    Serial.begin(9600);
}

void loop() {
    /* Feed LVGL engine */
    lv_timer_handler();

  static unsigned long lastUpdate = 0;

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

    }
    }
}
