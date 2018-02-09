#include <U8g2lib.h>
#include <FS.h>
#include <SPIFFS.h>

#define OLED_PINS 15,4,16 // CLK,DATA,RST
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_INIT_TYPE U8G2_SSD1306_128X64_NONAME_F_SW_I2C

//typedef enum {AHEAD, DOWN, UP, LEFT, RIGHT, ANGRY, SAD, EMOTE_MAX} emote;
typedef enum {AHEAD, DOWN, EMOTE_MAX} emote;

struct emotion_t {
  emote emotion;
  char *filename;
} eyes_list[] = {AHEAD,"/ahead.xbm",DOWN,"/down.xbm", 
//                 UP,"/up.xbm", LEFT,"/left.xbm",RIGHT,"/right.xbm",
//                 ANGRY,"/angry.xbm", SAD, "/sad.xbm",
                 EMOTE_MAX, ""};
uint8_t curEmotion = -1;
uint8_t tgtEmotion = 0;

OLED_INIT_TYPE u8g2(U8G2_R0, OLED_PINS);

void IRAM_ATTR incrementEyes() {
  if (curEmotion == EMOTE_MAX) {
    tgtEmotion=0;
  } else {
    tgtEmotion++;
  }
}

void isr_pin0() {
// Puts an interrupt on pin 0 (the top button on Heltec boards)
  pinMode (0, INPUT);
  attachInterrupt(digitalPinToInterrupt(0), incrementEyes, RISING);
}

void eyes() {
  curEmotion = tgtEmotion;
  if (tgtEmotion == EMOTE_MAX) {
    u8g2.clear();
    return;
  }
  char *filename = eyes_list[tgtEmotion].filename;
  if (!filename) return;
  Serial.print("Reading ");
  Serial.println(filename);
  if (!SPIFFS.exists(filename)) {
    Serial.println("File not found");
    return;
  }
  File imagefile = SPIFFS.open(filename);
  String xbm;
  u8g2_uint_t imageWidth, imageHeight;
  uint8_t imageBits[imagefile.size()/4]; //This seems inefficient
  uint16_t pos = 0;
  const char CR = 10;
  const char comma = 44;
  while(imagefile.available()) {
    char next = imagefile.read();
    if (next == CR) {
      if (xbm.indexOf("#define") == 0) {
        if (xbm.indexOf("width")>0) {
          xbm.remove(0,xbm.lastIndexOf(" "));
          imageWidth = xbm.toInt();
          if (imageWidth > OLED_WIDTH) {
            Serial.println("Image too large for screen");
            return;
          }
        } 
        if (xbm.indexOf("height")>0) {
          xbm.remove(0,xbm.lastIndexOf(" "));
          imageHeight=xbm.toInt();
          if (imageHeight > OLED_HEIGHT) {
            Serial.println("Image too large for screen");
            return;
          }
        }
      }
      xbm = "";
    } else if (next == comma) {
      imageBits[pos++] = (uint8_t) strtol(xbm.c_str(), NULL, 16);
      xbm = "";
    } else {xbm += next;}
  }
  imageBits[pos++] = (int) strtol(xbm.c_str(), NULL, 16);
  imageBits[pos]=0;
  u8g2.drawXBM((OLED_WIDTH-imageWidth)/2, (OLED_HEIGHT-imageHeight)/2, imageWidth, imageHeight, imageBits);
  u8g2.nextPage();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");
  u8g2.begin();
  SPIFFS.begin();
  isr_pin0();
}

void loop() {
  if (curEmotion != tgtEmotion) eyes();
  delay(100);
}
