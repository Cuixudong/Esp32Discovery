#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI   13
#define OLED_CLK   14
#define OLED_DC    5
#define OLED_CS    4
#define OLED_RESET 23
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

#define NUMFLAKES     10 // Number of snowflakes in the animation example

//const char* wifi_ssid = "My-WIFI";
//const char* wifi_pswd = "2773128204";
const char* wifi_ssid = "@PHICOMM_94";
const char* wifi_pswd = "";
const char* userId = "37049168";//UP主UID

uint8_t count = 0;
uint8_t refreshing = 0;
int httpCode;
Ticker ticker;
HTTPClient http;
HTTPClient http2;
WiFiClient client;

boolean isWIFIConnected()
{
  return !(WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_DISCONNECTED);
}

void wifiConnect(const char* wifi_ssid, const char* wifi_pswd)
{
  WiFi.begin(wifi_ssid, wifi_pswd);

  u8g2_for_adafruit_gfx.setCursor(0, 12);               // start writing at this position
  u8g2_for_adafruit_gfx.printf("连接到:\n%s", wifi_ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  display.clearDisplay();
  u8g2_for_adafruit_gfx.setCursor(0, 12);
  u8g2_for_adafruit_gfx.printf("连接成功");
  display.display();
}

// bilibili api: follower, view, likes
String UID = "37049168";//UP主UID
String followerUrl = "http://api.bilibili.com/x/relation/stat?vmid=" + UID;   // 粉丝数
String viewAndLikesUrl = "http://api.bilibili.com/x/space/upstat?mid=" + UID; // 播放数、点赞数

long follower = 0;   // 粉丝数
long view = 0;   // 播放数
long likes = 0;   // 获赞数

DynamicJsonBuffer jsonBuffer(1024); // ArduinoJson V5

void getViewAndLikes(String url)
{
  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();
  Serial.printf("[HTTP] GET... code: %d\n", httpCode);

  if (httpCode == 200)
  {
    Serial.println("Get OK");
    String resBuff = http.getString();

    // ---------- ArduinoJson V5 ----------
    JsonObject &root = jsonBuffer.parseObject(resBuff);
    if (!root.success())
    {
      Serial.println("parseObject() failed");
      return;
    }

    likes = root["data"]["likes"];
    view = root["data"]["archive"]["view"];
    u8g2_for_adafruit_gfx.setCursor(0, 16 + 12 * 2);
    u8g2_for_adafruit_gfx.printf("播放量:%d 获赞:%d", view, likes);
    display.display();
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %d\n", httpCode);
  }

  http.end();
}

void getFollower(String url)
{
  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();
  Serial.printf("[HTTP] GET... code: %d\n", httpCode);

  if (httpCode == 200)
  {
    Serial.println("Get OK");
    String resBuff = http.getString();

    // ---------- ArduinoJson V5 ----------
    JsonObject &root = jsonBuffer.parseObject(resBuff);
    if (!root.success())
    {
      Serial.println("parseObject() failed");
      return;
    }

    follower = root["data"]["follower"];
    u8g2_for_adafruit_gfx.setCursor(0, 16 + 12 * 3);
    u8g2_for_adafruit_gfx.printf("粉丝数:%d", follower);
    display.display();
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %d\n", httpCode);
  }

  http.end();
}


void parseUserData(String content)
{
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 210;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.parseObject(content);
  JsonObject& results_0 = root["results"][0];
  JsonObject& results_0_location = results_0["location"];
  const char* results_0_location_id = results_0_location["id"];
  const char* results_0_location_name = results_0_location["name"];
  const char* results_0_location_country = results_0_location["country"];
  const char* results_0_location_path = results_0_location["path"];
  const char* results_0_location_timezone = results_0_location["timezone"];
  const char* results_0_location_timezone_offset = results_0_location["timezone_offset"];

  JsonObject& results_0_now = results_0["now"];
  const char* results_0_now_text = results_0_now["text"];
  const char* results_0_now_code = results_0_now["code"];
  const char* results_0_now_temperature = results_0_now["temperature"];
  const char* results_0_last_update = results_0["last_update"];

  u8g2_for_adafruit_gfx.setCursor(0, 16 + 12);
  u8g2_for_adafruit_gfx.printf("天气:%s,%s,%s度", results_0_location_name, results_0_now_text, results_0_now_temperature);
  display.display();
}

void get_http_data(void)
{
  if (client.connect("api.seniverse.com", 80) == 1)//连接服务器判断连接，成功就发送GET请求数据下发
  {
    //S***S换成你自己在心知天气申请的私钥//改成你所在城市的拼音
    client.print("GET /v3/weather/now.json?key=S***S&location=guangzhou&language=zh-Hans&unit=c HTTP/1.1\r\n"); //心知URL
    client.print("Host:api.seniverse.com\r\n");
    client.print("Accept-Language:zh-cn\r\n");
    client.print("Connection:close\r\n\r\n");           //向心知天气的服务器发送请求。
    //String status_code = client.readStringUntil('\r');//读取GET数据，服务器返回的状态码，若成功则返回状态码200
    //Serial.println(status_code);
    if (client.find("\r\n\r\n") == 1)                   //跳过返回的数据头，直接读取后面的JSON数据，
    {
      String json_from_server = client.readStringUntil('\n'); //读取返回的JSON数据
      parseUserData(json_from_server);                  //将读取的JSON数据，传送到JSON解析函数中进行显示。
    }
  }
  client.stop();

  http.begin("api.bilibili.com", 80, "/x/relation/stat?vmid=" + String(userId));
  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    StaticJsonBuffer<1024> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(http.getString());
    if (!root.success())
    {
      Serial.println("parseObject() failed");
    }
    int fs = root["data"]["follower"].as<uint32_t>();
    int gz = root["data"]["following"].as<uint32_t>();
    u8g2_for_adafruit_gfx.setCursor(0, 16 + 12 * 2);
    u8g2_for_adafruit_gfx.printf("                   ");
    u8g2_for_adafruit_gfx.setCursor(0, 16 + 12 * 2);
    u8g2_for_adafruit_gfx.printf("粉丝数:%d 关注数:%d", fs, gz);
    display.display();
  }
  http.end();
}

void get_bfl(void)
{
  http.begin("api.bilibili.com",80,"/x/space/arc/search?mid=37049168&pn=1&ps=1&index=1");
  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    StaticJsonBuffer<2048> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(http.getString());
    if (!root.success())
    {
      Serial.println("parseObject() failed");
    }
    int bfl = root["data"]["list"]["vlist"][0]["play"].as<uint32_t>();
    u8g2_for_adafruit_gfx.setCursor(0, 16 + 12 * 3);
    u8g2_for_adafruit_gfx.printf("              ");
    u8g2_for_adafruit_gfx.setCursor(0, 16 + 12 * 3);
    u8g2_for_adafruit_gfx.printf("播放量:%8d ", bfl);
    display.display();
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC))
  {
    Serial.printf("SSD1306 allocation failed\n");
  }
  ticker.attach(1, []() {
    count ++;
  });
  WiFi.mode(WIFI_STA);
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  u8g2_for_adafruit_gfx.begin(display);
  display.clearDisplay();                               // clear the graphcis buffer
  u8g2_for_adafruit_gfx.setFontDirection(0);            // left to right (this is default)
  u8g2_for_adafruit_gfx.setForegroundColor(WHITE);      // apply Adafruit GFX color
  //  u8g2_for_adafruit_gfx.setFont(u8g2_font_siji_t_6x10);  // icon font
  //  u8g2_for_adafruit_gfx.setFontMode(1);                 // use u8g2 transparent mode (this is default)
  //  u8g2_for_adafruit_gfx.drawGlyph(0, 10, 0x0e200);  // Power Supply
  u8g2_for_adafruit_gfx.setFont(u8g2_font_wqy12_t_gb2312b);  // extended font
  u8g2_for_adafruit_gfx.setFontMode(1);                 // use u8g2 transparent mode (this is default)
  display.display();
  wifiConnect(wifi_ssid, wifi_pswd);

  get_http_data();
  get_bfl();
}

void loop()
{
  while (!isWIFIConnected())
  {
    Serial.printf("Reconnecting...\n");
    wifiConnect(wifi_ssid, wifi_pswd);
  }
  if (count % 20 == 0)
  {
    if (refreshing != count)
    {
      refreshing = count;
      get_http_data();
      get_bfl();
    }
  }
}
