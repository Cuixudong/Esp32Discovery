
#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     21
#define SIOD_GPIO_NUM     23
#define SIOC_GPIO_NUM     22

#define Y9_GPIO_NUM       34
#define Y8_GPIO_NUM       19
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       26
#define Y5_GPIO_NUM       33
#define Y4_GPIO_NUM       35
#define Y3_GPIO_NUM       32
#define Y2_GPIO_NUM       25
#define VSYNC_GPIO_NUM    36
#define HREF_GPIO_NUM     39
#define PCLK_GPIO_NUM     27

#else
#error "Camera model not selected"
#endif
