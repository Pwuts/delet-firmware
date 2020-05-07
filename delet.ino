#include <Arduino.h>
#include <FastLED.h>

#define DATA_PIN_LED D8
#define SWITCH_PIN D6

#define MODE_COLOR_WHEEL 0
#define MODE_COLOR_FADE  1
#define MODE_COLOR_WALK  2
#define MODE_PROGRESS    3
#define MODE_BLINK_FADE  4

const int overlap = 56;

const int walk_overlap = 72;
const int cycle_length[] = {256, 256, 512 - 4*walk_overlap, 512 - 3*overlap, 256};
const int cycle_int[] = {20, 10, 8, 6, 8};
#define N_MODES 5

int mode = MODE_COLOR_WHEEL;
int mode_reset = 0;

static CRGB leds[4];
int i = 0;
int h = 64;

void setup(void)
{
    Serial.begin(115200);

    FastLED.addLeds < WS2811, DATA_PIN_LED > (leds, 4);
    pinMode(SWITCH_PIN, INPUT_PULLUP);
}

void loop(void)
{
    i = (i + 1) % cycle_length[mode];

    if (!mode_reset && digitalRead(SWITCH_PIN) == LOW) {
        mode_reset = 1;
        mode = (mode + 1) % N_MODES;
        Serial.print("new mode: "); Serial.println(mode);
        Serial.print("h = "); Serial.println(h);

        i = 0;
    } else if (mode_reset && digitalRead(SWITCH_PIN) == HIGH) {
        mode_reset = 0;
    }

    if (mode == MODE_COLOR_WHEEL) {
        color_wheel(i);
    } else if (mode == MODE_COLOR_FADE) {
        color_fade(i);
    } else if (mode == MODE_COLOR_WALK) {
        color_walk(i);
    } else if (mode == MODE_PROGRESS) {
        progress(i);
    } else if (mode == MODE_BLINK_FADE) {
        blink_fade(i);
    }

    FastLED.show();

    delay(cycle_int[mode]);
}

int wave(int x) {
    return x < 0 || x > 255 ? 0 : 255 - cos8(x);
    // x = x % 256;
    // return x < 0 || x > 255 ? 0 : (x < 128 ? 2*x : 511 - 2*x);
    // return x < 0 || x > 255 ? 0 : tripulse(x);
}

int upramp(int x) {
    return x > 128 ? 255 : wave(x);
}

int downramp(int x) {
    return 255 - upramp(x);
}

int tripulse(int x) {
    return x < 73 ? 3*x : x < 128 ? 170 + x*2/3 : x < 183 ? 255 - (x-128)*2/3 : 3*(255 - x);
}

void color_wheel(int i) {
    leds[0] = CHSV(i,               255, 255);
    leds[1] = CHSV((i + 64)  % 255, 255, 255);
    leds[2] = CHSV((i + 128) % 255, 255, 255);
    leds[3] = CHSV((i + 192) % 255, 255, 255);
}

void color_fade(int i) {
    h = i;
    leds[0] = leds[1] = leds[2] = leds[3] = CHSV(i, 255, 255);
}

void color_walk(int i) {
    // h = (h + 1) % 256;
    Serial.print(wave(i + 128 - walk_overlap)); Serial.print(','); Serial.println(wave(i - 384 + 3*walk_overlap));

    leds[1] = CHSV(h, 255, min(wave(i + 256 - 2*walk_overlap) + wave(i - 256 + 2*walk_overlap), 255));
    leds[0] = CHSV(h, 255, min(wave(i + 128 - walk_overlap)   + wave(i - 384 + 3*walk_overlap), 255));
    leds[3] = CHSV(h, 255, wave(i));
    leds[2] = CHSV(h, 255, min(wave(i + 384 - 3*walk_overlap) + wave(i - 128 + walk_overlap), 255));
}

void progress(int p) {
    const int o = overlap/2;
    leds[0] = CHSV(h, 255, upramp(2*p)               - upramp(2*(p - 256 + 4*o)));
    leds[3] = CHSV(h, 255, upramp(2*(p - 64 + o))    - upramp(2*(p - 320 + 5*o)));
    leds[2] = CHSV(h, 255, upramp(2*(p - 128 + 2*o)) - upramp(2*(p - 384 + 6*o)));
    leds[1] = CHSV(h, 255, upramp(2*(p - 192 + 3*o)) - upramp(2*(p - 448 + 7*o)));
}

void blink_fade(int i) {
    leds[0] = leds[1] = leds[2] = leds[3] = CHSV(h, 255, wave(i));
    Serial.print(i); Serial.print(':'); Serial.print(h); Serial.print(':'); Serial.println(wave(i));
    if (i == 0) {
        h = (h + 45) % 256;
        Serial.println("hhhhh");
    }
}