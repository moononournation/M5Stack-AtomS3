/*******************************************************************************
 * Animated GIF Image Viewer
 * GIF source:
 * https://emojipedia.org/animated-noto-color-emoji/
 *
 * Setup steps:
 * 1. Change your LCD parameters in Arduino_GFX setting
 * 2. Upload Animated GIF file
 *   LittleFS:
 *     upload LittleFS data with ESP8266 LittleFS Data Upload:
 *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
 ******************************************************************************/

/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#define GFX_BL DF_GFX_BL
Arduino_GFX *gfx = create_default_Arduino_GFX();
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

#include "driver/ledc.h"

#include <LittleFS.h>
static uint8_t gif_count = 0;
static String gif_file_list[255];
static uint8_t current_gif_idx = 0;

#include "GifClass.h"
static GifClass gifClass;

#define BUTTON_PIN 41
bool btn_press_handled = true;
void IRAM_ATTR ISR()
{
  ++current_gif_idx;
  if (current_gif_idx >= gif_count)
  {
    current_gif_idx = 0;
  }
  btn_press_handled = false;
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Animated GIF Image Viewer");

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
//  pinMode(GFX_BL, OUTPUT);
//  digitalWrite(GFX_BL, HIGH);
  ledcSetup(0, 500, 8);
  ledcAttachPin(GFX_BL, 0);
  ledcWrite(0, 127);
#endif

  if (!LittleFS.begin())
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
    exit(0);
  }

  File root = LittleFS.open("/");
  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }
  Serial.println("Read path /");
  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("DIR: ");
      Serial.println(file.name());
    }
    else
    {
      Serial.print("FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
      gif_file_list[gif_count] = "/";
      gif_file_list[gif_count++] += file.name();
    }
    file = root.openNextFile();
  }
  file.close();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, ISR, FALLING);
}

void loop()
{
  File gifFile = LittleFS.open(gif_file_list[current_gif_idx].c_str(), "r");
  if (!gifFile || gifFile.isDirectory())
  {
    Serial.println(F("ERROR: open gifFile Failed!"));
    gfx->println(F("ERROR: open gifFile Failed!"));
  }
  else
  {
    // read GIF file header
    gd_GIF *gif = gifClass.gd_open_gif(&gifFile);
    if (!gif)
    {
      Serial.println(F("gd_open_gif() failed!"));
    }
    else
    {
      uint8_t *buf = (uint8_t *)malloc(gif->width * gif->height);
      if (!buf)
      {
        Serial.println(F("buf malloc failed!"));
      }
      else
      {
        int16_t x = (gfx->width() - gif->width) / 2;
        int16_t y = (gfx->height() - gif->height) / 2;

        // Serial.println(F("GIF video start"));
        int32_t t_fstart, t_delay = 0, t_real_delay, delay_until;
        int32_t res = 1;
        int32_t duration = 0, remain = 0;
        while ((res >= 0) && (btn_press_handled))
        {
          t_fstart = millis();
          t_delay = gif->gce.delay * 10;
          res = gifClass.gd_get_frame(gif, buf);
          if (res < 0)
          {
            Serial.println(F("ERROR: gd_get_frame() failed!"));
            break;
          }
          else if (res == 0)
          {
            gifClass.gd_rewind(gif);
            delay(1000);
          }
          else if (res > 0)
          {
            gfx->drawIndexedBitmap(x, y, buf, gif->palette->colors, gif->width, gif->height);

            t_real_delay = t_delay - (millis() - t_fstart);
            duration += t_delay;
            remain += t_real_delay;
            delay_until = millis() + t_real_delay;
            while (millis() < delay_until)
            {
              delay(1);
            }
          }
        }

        gifClass.gd_close_gif(gif);
        free(buf);

        btn_press_handled = true;
      }
    }
  }
}
