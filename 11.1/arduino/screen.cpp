#include "include/screen.hpp"

volatile bool Screen::screen_wakeup = false;

Screen::Screen(int sensor_pin, String initial_text, String initial_subtext)
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1), sensor_pin(sensor_pin) {
    this->text = initial_text;
    this->subtext = initial_subtext;
    this->last_refresh_ms = 0;

    this->screen_wakeup_time_ms = 0;
}

void Screen::screen_wakeup_isr() {
    Screen::screen_wakeup = true;
}

void Screen::initialise() {
    Wire.begin();
    Wire.setTimeout(3000);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        while (true) {
        }
    }
    delay(2000);

    pinMode(sensor_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(sensor_pin), Screen::screen_wakeup_isr, RISING);
}

void Screen::update(bool force_display) {
    if (millis() - this->last_refresh_ms < (1 / REFRESH_RATE_HZ) * 1000) {
        return;
    }

    this->last_refresh_ms = millis();
    this->display.clearDisplay();

    if (this->screen_wakeup) {
        this->screen_wakeup = false;
        this->screen_wakeup_time_ms = millis();
    }

    if (!force_display && millis() - this->screen_wakeup_time_ms >= SCREEN_WAKEUP_MS) {
        this->display.display();
        return;
    }

    this->display.setTextColor(WHITE);

    // text
    const uint8_t TEXT_FONT_SIZE = 2;
    this->display.setCursor(0, SCREEN_VERTICAL_MARGIN);
    this->display.setTextSize(TEXT_FONT_SIZE);
    this->display.println(this->text);

    // subtext
    const uint8_t SUBTEXT_FONT_SIZE = 1;
    this->display.setCursor(0, SCREEN_HEIGHT - SCREEN_VERTICAL_MARGIN - (SUBTEXT_FONT_SIZE * FONT_SIZE_HEIGHT_PX));
    this->display.setTextSize(SUBTEXT_FONT_SIZE);
    this->display.println(this->subtext);

    this->display.display();
}

void Screen::set_text(String text) {
    this->text = text;
}

void Screen::set_subtext(String subtext) {
    this->subtext = subtext;
}