#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define FONT_SIZE_HEIGHT_PX 8
#define SCREEN_VERTICAL_MARGIN 10

class IScreen {
    public:
     virtual void update();
     virtual void set_text(String text);
     virtual void set_subtext(String subtext);
};

class Screen : public IScreen {
    Adafruit_SSD1306 display;
    String text;
    String subtext;

    public:
     Screen(String initial_text = "", String initial_subtext = "");
     void initialise();
     void update() override;
     void set_text(String text) override;
     void set_subtext(String subtext) override;
};