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
// #define LOOPCOUNT                     // Loops per second number


// Globals
unsigned long timer = 0;
int lastLoop = 0;

const int8_t m[2] = {-1, 1};

// Draws center nav triangle shape
void drawTriangle() {
  
  double x1 = OLED_WIDTH / 2;                      // Center left-right
  double y1 = (OLED_HEIGHT * 0.625);               // Center bottom 3/4 of screen
  
  
  // double outerX = cos(radians(-20));
  // double outerY = sin(radians(-20));
  double outerX =  0.93969;                        // COSINE(-20)
  double outerY = -0.34202;                        // SINE(-20)

  // double lix = cos(radians(-30));
  // double liy = sin(radians(-30));
  double innerX =  0.86603;                        // COSINE(-30)
  double innerY = -0.5;                            // SINE(-30)

  for (int i=0; i<2; i++) {
    display.fillTriangle(
      x1,
      y1,
      x1 - (m[i] * outerX * 20),
      y1 - (outerY * 20),
      x1 - (m[i] * innerX * 12),
      y1 - (innerY * 12),
      WHITE
    );
  }
}

// Draw screen elements. Check param signs.
void drawScreen(int roll, int pitch, int heading) {
  display.clearDisplay();                          // Clear screen

  double x1 = (OLED_WIDTH / 2);                    // Center left-right
  double y1 = (OLED_HEIGHT * 0.625);               // Center bottom 3/4 of screen
  y1 += (pitch / 2);                               // Move up/down from pitch

  double x2 = cos(radians(roll));                  // x & y vals parallel roll
  double y2 = sin(radians(roll));

  double upX = cos(radians(roll+90));              // x & y vals perpendicular roll
  double upY = sin(radians(roll+90));

  #ifdef ARTIFICIAL_HORIZON
    display.writeLine(                             // Draw big horizon
      x1 - (x2 * 255),
      y1 - (y2 * 255),
      x1 + (x2 * 255),
      y1 + (y2 * 255),
      WHITE
    );
  #endif

  #ifdef PITCH_INDICATOR 
    for (int i=-10; i<11; i++) {                   // Line every 10 degrees of pitch

      int length = 1;                              // Normal - 5 length
      if (i%2 == 0) length = 2;                    // Every 2 - 10 length
      if (i%4 == 0) length = 3;                    // Every 4 - 15 length
      if (i == 0) length = 0;                      // Don't draw horizon

      for (int x=0; x<2; x++) {                    // Get -1 and 1 multipliers for
        display.writeLine(                         // left and right sides of line
          x1 - (upX * 5 * i),
          y1 - (upY * 5 * i),
          (x1 - (upX * 5 * i)) - (m[x] * x2 * 5 * length),
          (y1 - (upY * 5 * i)) - (m[x] * y2 * 5 * length),
          WHITE
        );
      }
    }
  #endif

  #ifdef ATTITUDE_ARROW
    drawTriangle();
  #endif

  #ifdef HEADING_NUMBER          
    display.fillRect(                              // Black background behind ticks
      0, 6, OLED_WIDTH, 10, BLACK                  // (stop horizon interfering)
    );

    int16_t textX, textY;
    uint16_t w, h;

    display.getTextBounds(                         // Calc width of new string
      String(heading), OLED_WIDTH/2, 9,
      &textX, &textY, &w, &h
    );        

    display.setCursor(textX - w / 2, textY);       // Set text cursor so number will be centered
    display.print(heading);                        // Print the number
  #endif

  #ifdef HEADING_INDICATOR                         // Draw ticks at top of display

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
      else                                                 // else -> 1px
        display.drawFastVLine(i+yawOffset, 0, 1, WHITE);
    } 
  #endif
  
  #ifdef LOOPCOUNT                                 // Print loops per second

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

                          // Send inverted axes to function (heading inverted later)
  drawScreen(-uav_roll, -uav_pitch, uav_heading);  

  
  #ifdef LOOPCOUNT        // If defined, increase counter every loop, update label every second

    count++;
    if ((millis()-timer) > 1000) {
      lastLoop = count;
      count = 0;
      timer = millis();
    }
  #endif
}