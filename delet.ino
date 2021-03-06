#include <WiFiSettings.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <FastLED.h>

#define DATA_PIN_LED D8
#define SWITCH_PIN D6

#define MODE_COLOR_WHEEL 0
#define MODE_COLOR_FADE  1
#define MODE_PROGRESS    2
#define MODE_SCANNER     3
#define MODE_COLOR_WALK  4
#define MODE_COLOR_BLINK 5

// waveform overlap
const int walk_overlap = 72;
const int progress_overlap = 56;
const int scanner_overlap = 84;

const int cycle_length[] = {
    256,
    256,
    1152 - 9*progress_overlap,
    1280 - 10*scanner_overlap,
    512 - 4*walk_overlap,
    256
};
const int cycle_int[] = {20, 20, 3, 6, 8, 8};   // cycle step interval per mode
#define N_MODES 6

int mode = MODE_COLOR_WHEEL;
bool mode_reset = false;
int t_reset;
bool portal_on = true;

static CRGB leds[4];
int i = 0;
int h = 64; // hue, not used in every mode

// Start ArduinoOTA via WiFiSettings with the same hostname and password
void setup_ota() {
    ArduinoOTA.setHostname(WiFiSettings.hostname.c_str());
    ArduinoOTA.setPassword(WiFiSettings.password.c_str());
    ArduinoOTA.begin();

    ArduinoOTA.onStart([]() {
        h = 160;    // blue
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        showProgress(map(progress, 0, total, 0, 100));
    });
    ArduinoOTA.onEnd([]() {
        leds[0] = leds[2] = CRGB::Lime;
        FastLED.show();
    });
}

int p = 0;
void setup_wifi() {
    WiFiSettings.secure = true;
    WiFiSettings.hostname = "delet";

    WiFiSettings.onPortal = []() {
        setup_ota();
    };
    WiFiSettings.onPortalWaitLoop = []() {
        ArduinoOTA.handle();
        progress(p = (p + 1) % cycle_length[MODE_PROGRESS]);
        FastLED.show();
    };
    WiFiSettings.onConnect = []() {
        h = 96; // green
        portal_on = false;
    };

    WiFiSettings.connect();

    Serial.print("Password: ");
    Serial.println(WiFiSettings.password);
}

void setup(void)
{
    Serial.begin(115200);
    FastLED.addLeds < WS2811, DATA_PIN_LED > (leds, 4);
    showProgress(0);

    LittleFS.begin();  // Will format on the first run after failing to mount
    showProgress(25);

    setup_wifi(); showProgress(50);
    setup_ota(); showProgress(75);

    pinMode(SWITCH_PIN, INPUT_PULLUP);

    showProgress(100);
}

void loop(void)
{
    ArduinoOTA.handle();

    i = (i + 1) % cycle_length[mode];

    if (!mode_reset && digitalRead(SWITCH_PIN) == LOW) {
        mode_reset = true;
        t_reset = millis();
        mode = (mode + 1) % N_MODES;

        Serial.print("new mode: "); Serial.println(mode);
        Serial.print("h = "); Serial.println(h);

        i = 0;
    } else if (mode_reset) {
        if (digitalRead(SWITCH_PIN) == HIGH) {
            mode_reset = false;

            if (millis() - t_reset > 3e3) { // show heap indicator after 3 second press
                h = map(ESP.getFreeHeap(), 0, 81920, 0, 160);
                showProgress(map(ESP.getFreeHeap(), 0, 81920, 0, 100));
                delay(5e3);
            }
        } else if (!portal_on) {
            const int reset_time = millis() - t_reset;
            if (reset_time > 10e3) {    // turn on captive portal after 10 second press
                portal_on = true;
                WiFiSettings.portal();
            }
            if (reset_time > 1e3) {
                h = 0;
                showProgress(reset_time/100);
                delay(10);
                return;
            }
        }
    }

    if (mode == MODE_COLOR_WHEEL) {
        color_wheel(i);
    } else if (mode == MODE_COLOR_FADE) {
        color_fade(i);
    } else if (mode == MODE_PROGRESS) {
        progress(i);
    } else if (mode == MODE_SCANNER) {
        scanner(i);
    } else if (mode == MODE_COLOR_WALK) {
        color_walk(i);
    } else if (mode == MODE_COLOR_BLINK) {
        color_blink(i);
    }

    FastLED.show();

    delay(cycle_int[mode]);
}

// wave functions

int wave(int x)
{
    return x < 0 || x > 255 ? 0 : 255 - cos8(x);
}

int upramp(int x)
{
    return x > 128 ? 255 : wave(x);
}

int downramp(int x)
{
    return 255 - upramp(x);
}

// animations

void color_wheel(int i)
{
    leds[0] = CHSV(i,               255, 255);
    leds[1] = CHSV((i + 64)  % 255, 255, 255);
    leds[2] = CHSV((i + 128) % 255, 255, 255);
    leds[3] = CHSV((i + 192) % 255, 255, 255);
}

void color_fade(int i)  // fades between colors continuously
{
    h = i;
    leds[0] = leds[1] = leds[2] = leds[3] = CHSV(i, 255, 255);
}

void progress(int p) { progress(p, progress_overlap); }
void progress(int p, int o) // wind-on wind-off progress/loading animation
{
    leds[0] = CHSV(h, 255, upramp(p)             - upramp(p - 512 + 4*o));
    leds[3] = CHSV(h, 255, upramp(p - 128 + o)   - upramp(p - 640 + 5*o));
    leds[2] = CHSV(h, 255, upramp(p - 256 + 2*o) - upramp(p - 768 + 6*o));
    leds[1] = CHSV(h, 255, upramp(p - 384 + 3*o) - upramp(p - 896 + 7*o));
}

void showProgress(int p) { progress(map(p, 0, 100, 0, 512), 0); FastLED.show(); }

void scanner(int i) { scanner(2*i, scanner_overlap); }
void scanner(int i, int o)  // rotary larson scanner :)
{
    const int srp = 64;
    leds[0] = CHSV(h, 255, min(wave(i)             + wave(i - srp - 1024 + 8*o), 255));
    leds[3] = CHSV(h, 255, min(wave(i - 128 + o)   + wave(i - srp - 896 + 7*o), 255));
    leds[2] = CHSV(h, 255, min(wave(i - 256 + 2*o) + wave(i - srp - 768 + 6*o), 255));
    leds[1] = CHSV(h, 255, min(wave(i - 384 + 3*o) + wave(i - srp - 640 + 5*o), 255));
}

void color_walk(int i) { color_walk(i, walk_overlap); }
void color_walk(int i, int o)   // rotates and changes color slowly
{
    if (i % 10 == 0) h = (h + 1) % 256;

    leds[1] = CHSV(h, 255, min(wave(i + 256 - 2*o) + wave(i - 256 + 2*o), 255));
    leds[0] = CHSV(h, 255, min(wave(i + 128 - o)   + wave(i - 384 + 3*o), 255));
    leds[3] = CHSV(h, 255, wave(i));
    leds[2] = CHSV(h, 255, min(wave(i + 384 - 3*o) + wave(i - 128 + o), 255));
}

void color_blink(int i) // changes color between blinks
{
    leds[0] = leds[1] = leds[2] = leds[3] = CHSV(h, 255, wave(i));
    if (i == 0) {
        h = (h + 45) % 256;
    }
}
