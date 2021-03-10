#include <Arduino.h>
#include <Wire.h>
// #include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <AltSoftSerial.h>
#include <stdint.h>

#include <Encoder.h>

#define SOFTSERIAL_BAUD 9600

#define OLED_WIDTH    128
#define OLED_HEIGHT   64


#define PROTOCOL_LIGHTTELEMETRY
#define VERTICAL_SCALE 2
#define TICK_SPACING  1

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire);
AltSoftSerial ltmSerial(8, 9);

#include <LightTelemetry.cpp>

// Visibility options
#define ATTITUDE_ARROW                   // Static arrow in center of screen
#define ARTIFICIAL_HORIZON               // Artificial horizon line
#define PITCH_INDICATOR                  // Lines above and below horizon

#define HEADING_INDICATOR                // Heading ticks
#define HEADING_NUMBER                   // Number below heading ticks
// #define PITCH_NUMBER                     // Pitch angle in top left
// #define ROLL_NUMBER                      // Roll angle in top left
// #define LOOPCOUNT                     // Loops per second number


#if defined(LOOPCOUNT)
  unsigned long timer = 0;
  unsigned long count = 0;
  uint16_t lastLoop = 0;
#endif

const double centerX = OLED_WIDTH / 2;                      // Center left-right
const double centerY = OLED_HEIGHT * 0.625;               // Center bottom 3/4 of screen

#if defined(ATTITUDE_ARROW)
  // double outerX = cos(radians(-20));
  // double outerY = sin(radians(-20));
  const double outerX =  0.93969;                        // COSINE(-20)
  const double outerY = -0.34202;                        // SINE(-20)

  // double lix = cos(radians(-30));
  // double liy = sin(radians(-30));
  const double innerX =  0.86603;                        // COSINE(-30)
  const double innerY = -0.5;                            // SINE(-30)
#endif

// Draw screen elements. Check param signs.
void drawScreen(int roll, int pitch, int heading) {
  display.clearDisplay();                          // Clear screen


  #if defined(ARTIFICIAL_HORIZON)

    const double paraX = cos(radians(-roll));                  // x & y vals parallel roll
    const double paraY = sin(radians(-roll));

    const double perpX = -paraY;              // x & y vals perpendicular roll
    const double perpY = paraX;

    const double moveX = (centerX - (perpX * pitch * VERTICAL_SCALE));
    const double moveY = (centerY - (perpY * pitch * VERTICAL_SCALE));

    for (int i=-90; i<=90; i++) {
      int length = 0;
        if (i == 0) length = 255;
      #if defined(PITCH_INDICATOR)
        else if (i % 90 == 0) length = 48;
        else if (i % 10 == 0) length = 22;
        else if (i %  5 == 0) length = 10;
      #endif

      if (length != 0) {
        display.writeLine(
          moveX - (perpX * i * VERTICAL_SCALE) - (paraX * length),
          moveY - (perpY * i * VERTICAL_SCALE) - (paraY * length),
          moveX - (perpX * i * VERTICAL_SCALE) + (paraX * length),
          moveY - (perpY * i * VERTICAL_SCALE) + (paraY * length),
          WHITE
        );
      }
    }
  #endif

  #if defined(ATTITUDE_ARROW)
    display.fillTriangle(
      centerX,
      centerY,
      centerX - (outerX * 20),
      centerY - (outerY * 20),
      centerX - (innerX * 12),
      centerY - (innerY * 12),
      INVERSE
    );
    
    display.fillTriangle(
      centerX,
      centerY,
      centerX + (outerX * 20),
      centerY - (outerY * 20),
      centerX + (innerX * 12),
      centerY - (innerY * 12),
      INVERSE
    );
  #endif

  #if defined(HEADING_NUMBER)  
    int16_t textX, textY;
    uint16_t w, h;

    display.fillRect(                              // Black background behind ticks
      0, 6,
      OLED_WIDTH, 10,
      BLACK                  // (stop horizon interfering)
    );

    display.getTextBounds(                         // Calc width of new string
      String(heading),
      OLED_WIDTH/2, 9,
      &textX, &textY,
      &w, &h
    );        

    display.setCursor(textX - w / 2, textY);       // Set text cursor so number will be centered
    display.print(heading);                        // Print the number
  #endif

  #if defined(HEADING_INDICATOR)                         // Draw ticks at top of display

    display.fillRect(0, 0, OLED_WIDTH, 6, BLACK);

    int yawOffset = -heading * TICK_SPACING;       // Offset to heading value
    yawOffset += OLED_WIDTH / 2;                   // Move to center of screen
    yawOffset -= TICK_SPACING;                     // Offset 1 tick for perfect center

        // Cover screen left | Repeat 360 ticks + cover screen right | ++
    for (int i=-(OLED_WIDTH/2); i<(360*TICK_SPACING + (OLED_WIDTH/2)); i++) {

      if (i == 0)                                          // 0deg -> 6px
        display.drawFastVLine(i+yawOffset, 0, 6, WHITE);
      else if (i % (90 * TICK_SPACING) == 0)               // 90degs -> 5px
        display.drawFastVLine(i+yawOffset, 0, 5, WHITE);
      else if (i % (15 * TICK_SPACING) == 0)               // 15degs -> 4px
        display.drawFastVLine(i+yawOffset, 0, 4, WHITE);
      else if (i % (5 * TICK_SPACING) == 0)                // else -> 1px
        display.drawFastVLine(i+yawOffset, 0, 1, WHITE);
    } 
  #endif

  #if defined(PITCH_NUMBER)
  
    display.setCursor(0, 9);
    display.print(-pitch);
  #endif

  #if defined(ROLL_NUMBER)

    #if defined(PITCH_NUMBER)
      display.print(" ");
    #endif
    display.print(abs(roll));
  #endif
  
  #if defined(LOOPCOUNT)                                 // Print loops per second

    display.setCursor(0, 0);
    display.print(lastLoop);
  #endif

  display.display();                               // Output to display
}

void setup() {
  // put your setup code here, to run once:
  
  ltmSerial.begin(SOFTSERIAL_BAUD);                // Initiate libraries and button
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.setTextColor(WHITE);                     // Initial display config
  display.clearDisplay();
  display.display();
}

void loop() {
  // put your main code here, to run repeatedly:
  ltm_read();                                      // Read from LTM Telemetry

  drawScreen(uav_roll, uav_pitch, uav_heading);  

  
  #if defined(LOOPCOUNT)        // If defined, increase counter every loop, update label every second

    count++;
    if ((millis()-timer) > 1000) {
      lastLoop = count;
      count = 0;
      timer = millis();
    }
  #endif
}