
#include "core/powerSave.h"
#include <interface.h>

void _setup_gpio() {
    pinMode(UP_BTN, INPUT);
    pinMode(SEL_BTN, INPUT);
    pinMode(DW_BTN, INPUT);
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    gpio_pulldown_dis(GPIO_NUM_36);
    gpio_pullup_dis(GPIO_NUM_36);
    pinMode(32, OUTPUT);
    pinMode(33, OUTPUT);
    digitalWrite(32, LOW);
    digitalWrite(33, HIGH);
    int pin_shared_ctrl = 33;
    int pin_sck = 0;
    pinMode(pin_shared_ctrl, OUTPUT);
    pinMode(pin_sck, OUTPUT);
    digitalWrite(pin_shared_ctrl, HIGH);
    delay(10);
    for (int i = 0; i < 80; i++) {
        digitalWrite(pin_sck, HIGH);
        delayMicroseconds(10);
        digitalWrite(pin_sck, LOW);
        delayMicroseconds(10);
    }
    digitalWrite(pin_shared_ctrl, HIGH);
}

void _setBrightness(uint8_t brightval) {
    if (brightval == 0) {
        analogWrite(TFT_BL, brightval);
    } else {
        int bl = MINBRIGHT + round(((255 - MINBRIGHT) * brightval / 100));
        analogWrite(TFT_BL, bl);
    }
}

void InputHandler(void) {
    static unsigned long tm = 0;
    static unsigned long dwPressStart = 0;
    static bool dwWasPressed = false;
    static unsigned long selPressStart = 0;
    static bool selWasPressed = false;
    if (millis() - tm < 200 && !LongPress) return;

    bool selPressed = (digitalRead(SEL_BTN) == LOW);
    bool dwPressed = (digitalRead(DW_BTN) == LOW);

    if (dwPressed && !dwWasPressed) {
        dwPressStart = millis();
        dwWasPressed = true;
    } else if (!dwPressed) {
        dwWasPressed = false;
    }

    if (selPressed && !selWasPressed) {
        selPressStart = millis();
        selWasPressed = true;
    } else if (!selPressed) {
        selWasPressed = false;
    }

    bool dwLongPress = dwWasPressed && (millis() - dwPressStart > 700);
    bool selLongPress = selWasPressed && (millis() - selPressStart > 3000);

    bool anyPressed = selPressed || dwPressed;
    if (anyPressed) tm = millis();
    if (anyPressed && wakeUpScreen()) return;

    AnyKeyPress = anyPressed;
    PrevPress = false;
    EscPress = dwLongPress;
    NextPress = dwPressed && !dwLongPress;
    SelPress = selPressed && !selLongPress;
    LongPress = selLongPress;
}

void powerOff() {
    digitalWrite(4, LOW);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)SEL_BTN, LOW);
    esp_deep_sleep_start();
}

void checkReboot() {
    int countDown = 0;
    if (digitalRead(SEL_BTN) == LOW) {
        uint32_t time_count = millis();
        while (digitalRead(SEL_BTN) == LOW) {
            if (millis() - time_count > 500) {
                if (countDown == 0) {
                    int textWidth = tft.textWidth("PWR OFF IN 3/3", 1);
                    tft.fillRect(60, 7, textWidth, 18, bruceConfig.bgColor);
                }
                tft.setCursor(60, 12);
                tft.setTextSize(1);
                tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
                countDown = (millis() - time_count) / 1000 + 1;
                tft.printf(" PWR OFF IN %d/3\n", countDown);
                vTaskDelay(10 / portTICK_RATE_MS);
                if (countDown >= 3) {
                    powerOff();
                }
            }
        }
        if (millis() - time_count > 500) {
            tft.fillRect(60, 12, 16 * LW, tft.fontHeight(1), bruceConfig.bgColor);
            drawStatusBar();
        }
        SelPress = true;
    }
}

bool isCharging() { return false; }
