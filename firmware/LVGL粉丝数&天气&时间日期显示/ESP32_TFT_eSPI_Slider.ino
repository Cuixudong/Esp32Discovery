#include <lvgl.h>
#include <Ticker.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include "time.h"
#include "FS.h"
#include "SD_MMC.h"
#include "lv_port_fatfs.h"
#include "lv_qrcode.h"

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return;
  Serial.println(&timeinfo, "%F %T %A"); // 格式化输出
}

typedef struct
{
  int fans;//粉丝数
  int follow;//关注数
  int bfl;
  int tm_year;
  int tm_mon;
  int tm_day;
  int tm_week;
  int tm_hour;
  int tm_min;
  int tm_sec;
  bool wifi_sta;
} global_data;

global_data g_data;

WiFiUDP ntpUDP;
const unsigned long HTTP_TIMEOUT = 5000;
//NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 30 * 60 * 1000);
#define LVGL_TICK_PERIOD 60

Ticker tick; /* timer for interrupt handler */
TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

lv_obj_t * slider_label;
int screenWidth = 240;
int screenHeight = 240;

#if 1
const char* wifi_ssid = "My-WIFI";
const char* wifi_passwd = "2773128204";
#else
//const char* wifi_ssid = "@PHICOMM_94";
//const char* wifi_passwd = "";
#endif
const char* userId = "37049168";// UP主UID

int httpCode;
HTTPClient http;
WiFiClient client;

boolean isWIFIConnected()
{
  return !(WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_DISCONNECTED);
}

lv_obj_t * label_wifi_sta;
lv_obj_t * label_up_sta;
lv_obj_t * label_time;
lv_obj_t * label_weather;
String http_str;

void get_http_data(void)
{
  if (client.connect("api.seniverse.com", 80) == 1)//连接服务器判断连接，成功就发送GET请求数据下发
  {
    // S**S 换成你自己在心知天气申请的私钥//改成你所在城市的拼音
    client.print("GET /v3/weather/now.json?key=S**S&location=guangzhou&language=zh-Hans&unit=c HTTP/1.1\r\n"); //心知URL
    client.print("Host:api.seniverse.com\r\n");
    client.print("Accept-Language:zh-cn\r\n");
    client.print("Connection:close\r\n\r\n");           //向心知天气的服务器发送请求。
    //String status_code = client.readStringUntil('\r');//读取GET数据，服务器返回的状态码，若成功则返回状态码200
    //Serial.println(status_code);
    if (client.find("\r\n\r\n") == 1)                   //跳过返回的数据头，直接读取后面的JSON数据，
    {
      http_str = client.readStringUntil('\n'); //读取返回的JSON数据
      //Serial.println(http_str);
      //parseUserData(http_str);                  //将读取的JSON数据，传送到JSON解析函数中进行显示。
      StaticJsonBuffer<512> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(http_str);
      if (!root.success())
      {
        //Serial.println("parseObject() failed");
      }
      String position_ = root["results"][0]["location"]["name"];
      String weather_ = root["results"][0]["now"]["text"];
      String temperature_ = root["results"][0]["now"]["temperature"];
      lv_label_set_text_fmt(label_weather, "%s,%s,%s度", position_, weather_, temperature_);
    }
  }
  client.stop();

  http.begin("api.bilibili.com", 80, "/x/relation/stat?vmid=" + String(userId));
  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    http_str = http.getString();
    //Serial.println(http_str);
    StaticJsonBuffer<512> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(http_str);
    if (!root.success())
    {
      //Serial.println("parseObject() failed");
    }
    g_data.fans = root["data"]["follower"].as<uint32_t>();
    g_data.follow = root["data"]["following"].as<uint32_t>();
    //lv_label_set_text_fmt(label_up_sta, "粉丝数:%d \n关注数:%d", g_data.fans, g_data.follow);
  }
  http.end();

  http.begin("api.bilibili.com", 80, "/x/space/arc/search?mid=37049168&pn=1&ps=1&index=1");
  httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    http_str = http.getString();
    //Serial.println(http_str);
    StaticJsonBuffer<2048> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(http_str);
    if (!root.success())
    {
      //Serial.println("parseObject() failed");
    }
    g_data.bfl = root["data"]["list"]["vlist"][0]["play"].as<uint32_t>();
    lv_label_set_text_fmt(label_up_sta, "粉丝数:%d \n关注数:%d\n播放量:%d", g_data.fans, g_data.follow, g_data.bfl);
  }
  http.end();
}

#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char * file, uint32_t line, const char * dsc)
{
  //Serial.printf("%s@%d->%s\r\n", file, line, dsc);
  delay(100);
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint16_t c;

  tft.startWrite(); /* Start new TFT transaction */
  tft.setAddrWindow(area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1)); /* set the working window */
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
      c = color_p->full;
      tft.writeColor(c, 1);
      color_p++;
    }
  }
  tft.endWrite(); /* terminate TFT transaction */
  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

/* Interrupt driven periodic handler */
static void lv_tick_handler(void)
{
  lv_tick_inc(LVGL_TICK_PERIOD);
}

static lv_task_t * task_test;
lv_obj_t *tab;
char time_str[52];
char ip_str[16];

void update_date_time(void)
{
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    g_data.tm_year = timeinfo.tm_year + 1900;
    g_data.tm_mon = timeinfo.tm_mon;
    g_data.tm_day = timeinfo.tm_mday;
    g_data.tm_week = timeinfo.tm_wday;
    g_data.tm_hour = timeinfo.tm_hour;
    g_data.tm_min = timeinfo.tm_min;
    g_data.tm_sec = timeinfo.tm_sec;
    sprintf(time_str, "%4d-%d-%d \n %2d:%02d:%02d", g_data.tm_year
            , g_data.tm_mon+1, g_data.tm_day, g_data.tm_hour, g_data.tm_min, g_data.tm_sec);
  }
}

static lv_task_cb_t m_task_cb(lv_task_t * task)
{
  static int i = 0;
  i ++;
  lv_tabview_set_tab_act(tab, i % 3, 1200);
}

void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */
  if (!SD_MMC.begin())
  {
    Serial.println("Card Mount Failed");
  }
  else
  {
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
      Serial.println("No SD_MMC card attached");
    }
    else
    {
      Serial.print("SD_MMC Card Type: ");
      if (cardType == CARD_MMC) {
        Serial.println("MMC");
      } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
      } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
      } else {
        Serial.println("UNKNOWN");
      }
      uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
      Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
    }
  }

  lv_init();
  lv_fs_if_init();
#if USE_LV_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  tft.begin(); /* TFT init */
  tft.setRotation(0);
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  /*Initialize the display*/
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the input device driver*/
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);             /*Descriptor of a input device driver*/
  indev_drv.type = LV_INDEV_TYPE_POINTER;    /*Touch pad is a pointer-like device*/
  indev_drv.read_cb = NULL;      /*Set your driver function*/
  lv_indev_drv_register(&indev_drv);         /*Finally register the driver*/

  /*Initialize the graphics library's tick*/
  tick.attach_ms(LVGL_TICK_PERIOD, lv_tick_handler);

  /* Create simple label */
  tab = lv_tabview_create(lv_scr_act(), NULL);
  lv_obj_t *tab1 = lv_tabview_add_tab(tab, "网络");
  lv_obj_t *tab2 = lv_tabview_add_tab(tab, "数据");
  lv_obj_t *tab3 = lv_tabview_add_tab(tab, "天气");
  lv_obj_align(tab, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);

  label_wifi_sta = lv_label_create(tab1, NULL);
  label_up_sta = lv_label_create(tab2, NULL);
  label_time = lv_label_create(tab3, NULL);
  label_weather = lv_label_create(tab3, NULL);
  lv_obj_set_style_local_text_font(label_wifi_sta, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &FontST16);
  lv_obj_set_style_local_text_font(label_up_sta, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &FontST16);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &FontST16);
  lv_obj_set_style_local_text_font(label_weather, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &FontST16);
  lv_obj_align(label_wifi_sta, tab1, LV_ALIGN_IN_TOP_LEFT, 10, 10);
  lv_obj_align(label_up_sta, tab2, LV_ALIGN_IN_TOP_LEFT, 10, 10);
  lv_obj_align(label_time, tab3, LV_ALIGN_IN_TOP_LEFT, 10, 10);
  lv_obj_align(label_weather, label_time, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 34);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_passwd);
  lv_label_set_text_fmt(label_wifi_sta, "连接WIFI:\n%s", wifi_ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  lv_label_set_text_fmt(label_wifi_sta, "无线网已连接");
  //timeClient.begin();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  update_date_time();
  lv_task_create((lv_task_cb_t)m_task_cb, 5000, LV_TASK_PRIO_LOW, NULL);
}
long int g_time_val = 0;

void loop()
{
  lv_task_handler(); /* let the GUI do its work */
  delay(5);
  g_time_val ++;
  if (g_time_val % 150 == 1)
  {
    //timeClient.update();
    //time_str = timeClient.getFormattedTime();
    update_date_time();
    lv_label_set_text_fmt(label_time, "时间:%s", time_str);
  }
  if (g_time_val % 3000 == 3)
  {
    get_http_data();
  }
}
