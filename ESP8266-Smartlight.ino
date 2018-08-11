#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "ESPAsyncUDP.h"
#include <ArduinoOTA.h>
#include "SK6812_i2s.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_common.h"
#include "strip.pb.h"
#include "Color.hpp"
#include <ClickButton.h>

/**** OTA ****/
const char* ssid = "<your ssid>";
const char* password = "<your password>";

/**** UDP ****/
AsyncUDP udp;
const uint16_t UDP_PORT = 1996;  // local port to listen on

/**** LEDs ****/
const uint8_t NUM_LEDS = 56;
static SK6812 ledstrip;
static pixel pixels[NUM_LEDS] = {};
static pixel udp_pixels[NUM_LEDS] = {};
const uint8_t CTRL_PIN = 16;

/**** Button ****/
const int BUTTON_PIN = 4;
ClickButton button(BUTTON_PIN, LOW, CLICKBTN_PULLUP);
int8_t button_clicks = 0;

/**** Internal states ****/
enum LightState
{
  OFF,        // light is turned off
  WHITE,      // light is turned on and in white mode
  COLOR,      // light is turned on and in color mode
  CONTROLLED  // light is turned on and controlled over UDP
};
volatile LightState last_state = LightState::OFF;     // when light state changes this keeps track of the last state
volatile LightState state = LightState::OFF;          // current light state
LightState save_state = LightState::OFF;              // this light state is used during power on

/**** fader ****/
uint32_t current_time = 0;

// brightness control
const uint32_t BRT_FADE_DELAY = 10;   // Time in milliseconds between fade steps
uint32_t adjust_brt_fader_time = 0;   // Time to adjust the fader
const uint8_t MAX_BRT = 255;          // the maximum settable brightness by user (only button)
const uint8_t MIN_BRT = 10;           // the minimum settable rightness by user (only button)
uint8_t brt = 20;                     // current brightness
volatile uint8_t last_brt = 20;       // last brightness to keep track of changes
bool brt_dir_changed = false;         // flag to indicate that direction of brightness change should be inversed
bool brt_dir = true;                  // current direction of brightness fade

// color control
const uint32_t COL_FADE_DELAY =  5;   // Time in milliseconds between fade steps
uint32_t adjust_col_fader_time = 0;   // Time to adjust the fader
bool light_color_changed = false;     // flag to indicate that the light color has changed, so the strip can be updated
Color light_color = Color::RGBW{0.0, 0.0, 0.0, 1.0};  // the current light color

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("Light1");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if(udp.listen(UDP_PORT))
  {
    Serial.println("UDP connected");
    udp.onPacket([](AsyncUDPPacket packet)
      {
          handleUDP(packet.data(), packet.length());
      }
    );
  }

  pinMode(CTRL_PIN, OUTPUT);
  digitalWrite(CTRL_PIN, LOW);
  ledstrip.init(NUM_LEDS);
  ledstrip.setBrightness(0);
  ledstrip.show(pixels);
  fillPixels(pixels, light_color);
}

void loop()
{
  ArduinoOTA.handle();
  handleButton();
  updateLEDs();
}

void handleButton()
{
  current_time = (long)millis();
  button.Update();

  if (button.clicks != 0)
  {
    Serial.println("clicks");
    button_clicks = button.clicks;
  }

  // Toggle ON/OFF on single clicks
  if(button.clicks == 1)
  {
    Serial.println("single click");
    if(state == LightState::OFF || state == LightState::CONTROLLED)
    {
      state = (save_state != LightState::OFF ? save_state : LightState::WHITE);
      turnOn(pixels);
    }
    else
    {
      save_state = state;
      state = LightState::OFF;
      turnOff(pixels);
    }
  }

  // toggle mode
  if(button.clicks == 2)
  {
    switch(state)
    {
      case LightState::WHITE:
        light_color = Color::RGBW{1.0, 0.0, 0.0, 0.0};
        light_color_changed = true;
        state = LightState::COLOR;
        break;
      case LightState::CONTROLLED:
      case LightState::COLOR:
        light_color = Color::RGBW{0.0, 0.0, 0.0, 1.0};
        light_color_changed = true;
        state = LightState::WHITE;
        break;
      case LightState::OFF:
      default:
        light_color = Color::RGBW{1.0, 0.0, 0.0, 0.0};
        light_color_changed = true;
        state = LightState::COLOR;
        fillPixels(pixels, light_color);
        turnOn(pixels);
    }
  }

  // change brightness if button is held down during single-click and light is turned on
  if(state != LightState::OFF)
  {
    if(button_clicks == -1 && button.depressed == true)
    {
      brt_dir_changed = true;
      // force lights on, since we want to fade it up or down
      if(state == LightState::OFF || state == LightState::CONTROLLED)
      {
        // also turn on mosfet for leds
        turnOn(pixels);
        state = save_state;
      }

      if ( current_time - adjust_brt_fader_time > BRT_FADE_DELAY)
      {
        adjust_brt_fader_time = current_time;
        if (brt_dir)
        {
          if(brt < MAX_BRT)
          {
            brt++;
          }
        }
        else
        {
          if(brt > MIN_BRT)
          {
            brt--;
          }
        }
      }
    }
    else if (brt_dir_changed)
    {
      brt_dir_changed = false;
      brt_dir = !brt_dir;
    }
  }


  // change hue if button is held down during double-click and state is color
  if(state == LightState::COLOR)
  {
    if(button_clicks == -2 && button.depressed == true)
    {
      // force lights on, since we want to change hue
      if(state == LightState::OFF || state == LightState::CONTROLLED)
      {
        // also turn on mosfet for leds
        turnOn(pixels);
        state = save_state;
      }

      if ( current_time - adjust_col_fader_time > COL_FADE_DELAY)
      {
        Color::HSV col = light_color.getHSV();
        adjust_col_fader_time = current_time;
        if(col.h < 360.0)
        {
           col.h += 0.1;
        }
        else
        {
          col.h = 0;
        }
        light_color_changed = true;
        light_color = col;
      }
    }
  }

  if(!button.depressed)
  {
    // Reset button_clicks
    button_clicks = 0;
  }
}

void updateLEDs()
{
  // only update if no data is received via udp
  if(state == LightState::WHITE || state == LightState::COLOR)
  {
    if(brt != last_brt)
    {
      Serial.printf("brt l %d\n", brt); // l stands for local
      last_brt = brt;
      ledstrip.setBrightness(brt);
      ledstrip.show(pixels);
    }
    if(last_state != state || light_color_changed)
    {
      Serial.println("state changed");
      last_state = state;
      light_color_changed = false;
      fillPixels(pixels, light_color);
      ledstrip.show(pixels);
    }
  }
}

void turnOn(pixel *leds)
{
  digitalWrite(CTRL_PIN, HIGH);
  ledstrip.setBrightness(brt);
  // fade to last state animating from center outwards
  pixel tmp_pixels[NUM_LEDS] = {};
  for(uint8_t anim = 0; anim < NUM_LEDS/4/2; ++anim)
  {
    uint8_t index1 = (NUM_LEDS/4/2) - 1 - anim;
    uint8_t inversed_index1 = (NUM_LEDS/4/2) + anim;
    for(uint8_t anim_fine = 0; anim_fine < 50; ++anim_fine)
    {
      for(uint8_t side = 0; side < 4; ++side)
      {
        tmp_pixels[index1+(NUM_LEDS/4)*side].R += 0.02 * leds[index1+(NUM_LEDS/4)*side].R;
        tmp_pixels[index1+(NUM_LEDS/4)*side].G += 0.02 * leds[index1+(NUM_LEDS/4)*side].G;
        tmp_pixels[index1+(NUM_LEDS/4)*side].B += 0.02 * leds[index1+(NUM_LEDS/4)*side].B;
        tmp_pixels[index1+(NUM_LEDS/4)*side].W += 0.02 * leds[index1+(NUM_LEDS/4)*side].W;
        tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].R += 0.02 * leds[inversed_index1+(NUM_LEDS/4)*side].R;
        tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].G += 0.02 * leds[inversed_index1+(NUM_LEDS/4)*side].G;
        tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].B += 0.02 * leds[inversed_index1+(NUM_LEDS/4)*side].B;
        tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].W += 0.02 * leds[inversed_index1+(NUM_LEDS/4)*side].W;
      }
      ledstrip.show(tmp_pixels);
    }
  }
}

void turnOff(pixel *leds)
{
  ledstrip.setBrightness(brt);
  pixel tmp_pixels[NUM_LEDS] = {};
  for(uint8_t led = 0; led < NUM_LEDS; ++led)
  {
    tmp_pixels[led] = leds[led];
  }
  // fade to dark animating from center outwards
  for(uint8_t anim = 0; anim < NUM_LEDS/4/2; ++anim)
  {
    uint8_t index1 = anim;
    uint8_t inversed_index1 = (NUM_LEDS/4) - 1 - anim;
    for(uint8_t anim_fine = 0; anim_fine < 51; ++anim_fine)
    {
      for(uint8_t side = 0; side < 4; ++side)
      {
        if(anim_fine < 50)
        {
          tmp_pixels[index1+(NUM_LEDS/4)*side].R -= floor(0.02 * pixels[index1+(NUM_LEDS/4)*side].R);
          tmp_pixels[index1+(NUM_LEDS/4)*side].G -= floor(0.02 * pixels[index1+(NUM_LEDS/4)*side].G);
          tmp_pixels[index1+(NUM_LEDS/4)*side].B -= floor(0.02 * pixels[index1+(NUM_LEDS/4)*side].B);
          tmp_pixels[index1+(NUM_LEDS/4)*side].W -= floor(0.02 * pixels[index1+(NUM_LEDS/4)*side].W);
          tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].R -= floor(0.02 * pixels[inversed_index1+(NUM_LEDS/4)*side].R);
          tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].G -= floor(0.02 * pixels[inversed_index1+(NUM_LEDS/4)*side].G);
          tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].B -= floor(0.02 * pixels[inversed_index1+(NUM_LEDS/4)*side].B);
          tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].W -= floor(0.02 * pixels[inversed_index1+(NUM_LEDS/4)*side].W);
        }
        else
        {
          tmp_pixels[index1+(NUM_LEDS/4)*side].R = 0;
          tmp_pixels[index1+(NUM_LEDS/4)*side].G = 0;
          tmp_pixels[index1+(NUM_LEDS/4)*side].B = 0;
          tmp_pixels[index1+(NUM_LEDS/4)*side].W = 0;
          tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].R = 0;
          tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].G = 0;
          tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].B = 0;
          tmp_pixels[inversed_index1+(NUM_LEDS/4)*side].W = 0;
        }
      }
      ledstrip.show(tmp_pixels);
    }
  }
  ledstrip.setBrightness(0);
  digitalWrite(CTRL_PIN, LOW);
}

bool led_callback(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    Strip_Led led = {};

    if (!pb_decode(stream, Strip_Led_fields, &led))
    {
      Serial.print("pb_decode failed");
      return false;
    }
    // make sure to not overflow the buffer
    if(led.num < NUM_LEDS)
    {
      uint32_t rgbw = led.rgbw;
      //leds[led.num] = {static_cast<uint8_t>(rgbw >> 24), static_cast<uint8_t>(rgbw >> 16), static_cast<uint8_t>(rgbw >> 8), static_cast<uint8_t>(rgbw)};
      udp_pixels[led.num] = {static_cast<uint8_t>(rgbw >> 24), static_cast<uint8_t>(rgbw >> 16), static_cast<uint8_t>(rgbw >> 8), static_cast<uint8_t>(rgbw)};
    }
    else
    {
      return false;
    }

    return true;
}

void handleUDP(uint8_t *udp_pkt, size_t udp_len)
{
  if (udp_len)
  {
    // allocate space for message
    Strip message = Strip_init_zero;
    // create stream to read from udp_buffer
    pb_istream_t stream = pb_istream_from_buffer(udp_pkt, udp_len);
    // add callback for all leds
    message.leds.funcs.decode = &led_callback;
    // decode message and check if decode faied
    if (!pb_decode_delimited(&stream, Strip_fields, &message))
    {
      Serial.printf("pb_decode_delimited failed: %s\n", PB_GET_ERROR(&stream));
      return;
    }

    if(message.has_brt)
    {
      Serial.printf("brt %d\n", message.brt);
      brt = message.brt;
      ledstrip.setBrightness(brt);
    }
    if(message.has_whole)
    {
      Serial.println("whole");
      uint32_t rgbw = message.whole.rgbw;
      uint8_t red = static_cast<uint8_t>(rgbw >> 24);
      uint8_t gre = static_cast<uint8_t>(rgbw >> 16);
      uint8_t blu = static_cast<uint8_t>(rgbw >> 8);
      uint8_t whi = static_cast<uint8_t>(rgbw);
      fillPixels(udp_pixels, Color{Color::RGBW{red, gre, blu, whi}});
    }

    if(state != LightState::CONTROLLED)
    {
      if(state == LightState::OFF)
      {
        turnOn(udp_pixels);
      }
      state = LightState::CONTROLLED;
    }
    ledstrip.show(udp_pixels);
  }
}

void fillPixels(pixel *leds, Color col)
{
  Color::RGBW col_rgbw = col.getRGBW();
  for(uint8_t led = 0; led < NUM_LEDS; ++led)
  {
    pixels[led].R = col_rgbw.r*255;
    pixels[led].G = col_rgbw.g*255;
    pixels[led].B = col_rgbw.b*255;
    pixels[led].W = col_rgbw.w*255;
  }
}
