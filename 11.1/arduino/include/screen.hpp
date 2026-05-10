#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define FONT_SIZE_HEIGHT_PX 8
#define SCREEN_VERTICAL_MARGIN 10

#define REFRESH_RATE_HZ 2
#define SCREEN_WAKEUP_MS 10000

class IScreen {
    public:
     virtual void update(bool force_display = false);
     virtual void set_text(String text);
     virtual void set_subtext(String subtext);
};

class Screen : public IScreen {
    private:
    Adafruit_SSD1306 display;
    String text;
    String subtext;
    unsigned long last_refresh_ms;

    const int sensor_pin;
    static volatile bool screen_wakeup;
    unsigned long screen_wakeup_time_ms;

    static void screen_wakeup_isr();


    public:
     Screen(int sensor_pin, String initial_text = "", String initial_subtext = "");
     void initialise();
     void update(bool force_display = false) override;
     void set_text(String text) override;
     void set_subtext(String subtext) override;
};