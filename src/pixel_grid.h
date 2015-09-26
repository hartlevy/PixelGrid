#include <pebble.h>

#define RECTWIDTH 4
#define RECTHEIGHT  4
#define WIDTH 144 / RECTWIDTH
#define HEIGHT 168 / RECTHEIGHT
#define WHITE 0  
#define RED 1
#define BLUE 2
#define GREEN 3
#define YELLOW 4
#define PURPLE 5
  
static const uint8_t BAT_WARN_LEVEL = 50;
static const uint8_t BAT_ALERT_LEVEL = 20;
static const float HI_COLOR_THRESHOLD = 0.7;
static const float MID_COLOR_THRESHOLD = 0.35;
static const float LO_COLOR_THRESHOLD = 0.1;

  
static const uint8_t RED_COLOR_SET[] = {
  GColorRedARGB8,   
  GColorDarkCandyAppleRedARGB8,
  GColorBulgarianRoseARGB8
};

static const uint8_t WHITE_COLOR_SET[] = {
  GColorWhiteARGB8,   
  GColorLightGrayARGB8,
  GColorDarkGrayARGB8
};

static const uint8_t BLUE_COLOR_SET[] = {
  GColorBlueMoonARGB8,   
  GColorBlueARGB8,
  GColorDukeBlueARGB8
};

static const uint8_t GREEN_COLOR_SET[] = {
  GColorGreenARGB8,   
  GColorIslamicGreenARGB8,
  GColorDarkGreenARGB8
};

static const uint8_t YELLOW_COLOR_SET[] = {
  GColorYellowARGB8,   
  GColorLimerickARGB8,
  GColorArmyGreenARGB8
};

static const uint8_t PURPLE_COLOR_SET[] = {
  GColorMagentaARGB8,   
  GColorPurpleARGB8,
  GColorImperialPurpleARGB8
};
  
static const struct GPathInfo BT_LOGO_POINTS = {
  14, 
  (GPoint []){
    {0,0}, {2,0}, {3,0}, {1,1}, {2,1}, {4,1}, 
    {2,2}, {3,2}, {1,3}, {2,3}, {4,3}, {0,4},
    {2,4}, {3,4}
  }
};     

static const struct GPathInfo BAT_CASE_POINTS = {
  27, 
  (GPoint []){
    {0,0}, {1,0}, {2,0}, {3,0}, {4,0}, {5,0}, 
    {6,0}, {7,0}, {8,0}, {9,0}, {10,0}, {11,0}, 
    {0,2}, {1,2}, {2,2}, {3,2}, {4,2}, {5,2}, 
    {6,2}, {7,2}, {8,2}, {9,2}, {10,2}, {11,2},       
    {0,1}, {11,1}, {12,1}
  }
};

static const struct GPathInfo PM_POINTS = {
  16,
  (GPoint[]){
    {1,0}, {1,1}, {1,2}, {2,0},
    {2,1}, {3,0}, {3,1}, {5,0}, 
    {5,1}, {5,2}, {6,0}, {7,0}, 
    {7,1}, {8,0}, {8,1}, {8,2}
  }
}; 

static const struct GPathInfo CHARGE_POINTS = {
  6,
  (GPoint[]){{6,0}, {5,1}, {5,2}, {6,2}, {6,3}, {5,4}}
};    

static const struct GPathInfo SLASH_POINTS = {
   4, 
  (GPoint[]){{8,2}, {8,3}, {9,0}, {9,1}}
};
