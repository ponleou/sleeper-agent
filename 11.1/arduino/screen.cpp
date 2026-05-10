#include "include/screen.hpp"
#include <cstdint>

Screen::Screen(String initial_text, String initial_subtext) : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {
    this->text = initial_text;
    this->subtext = initial_subtext;
}

void Screen::initialise() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        while (true) {
        }
    }

    delay(2000);
}

void Screen::update() {
    this->display.clearDisplay();
    this->display.setTextColor(WHITE);

    // // text
    // const uint8_t TEXT_FONT_SIZE = 2;
    // this->display.setCursor(0, SCREEN_VERTICAL_MARGIN);
    // this->display.setTextSize(TEXT_FONT_SIZE);
    // this->display.println(this->text);

    // subtext
    const uint8_t SUBTEXT_FONT_SIZE = 1;
    this->display.setCursor(0, SCREEN_HEIGHT - SCREEN_VERTICAL_MARGIN - (SUBTEXT_FONT_SIZE * FONT_SIZE_HEIGHT_PX));
    this->display.setTextSize(SUBTEXT_FONT_SIZE);
    this->display.println("test");

    this->display.display();
}

void Screen::set_text(String text) {
    this->text = text;
}

void Screen::set_subtext(String subtext) {
    this->subtext = subtext;
}