#include <pebble.h>

#define RECTWIDTH 4
#define RECTHEIGHT  4
#define WIDTH 144 / RECTWIDTH
#define HEIGHT 168 / RECTHEIGHT
#define NUM_COLOR 8
  
  
  enum {
  WHITE = 0x0,
  RED = 0x1,
  BLUE = 0x2,
  GREEN= 0x3,
  YELLOW = 0x4,
  PURPLE = 0x5,
  CYAN = 0x6,
  ORANGE = 0x7
};
  
static const uint8_t BAT_WARN_LEVEL = 50;
static const uint8_t BAT_ALERT_LEVEL = 20;
static const float HI_COLOR_THRESHOLD = 0.7;
static const float MID_COLOR_THRESHOLD = 0.35;
static const float LO_COLOR_THRESHOLD = 0.1;

static const uint8_t COLOR_SETS[NUM_COLOR][3] = {
  {GColorWhiteARGB8, GColorLightGrayARGB8, GColorDarkGrayARGB8}, //WHITE
  {GColorRedARGB8, GColorDarkCandyAppleRedARGB8, GColorBulgarianRoseARGB8}, //RED
  {GColorBlueMoonARGB8, GColorBlueARGB8, GColorDukeBlueARGB8}, //BLUE
  {GColorGreenARGB8, GColorIslamicGreenARGB8, GColorDarkGreenARGB8}, //GREEN  
  {GColorYellowARGB8, GColorLimerickARGB8, GColorArmyGreenARGB8}, //YELLOW  
  {GColorMagentaARGB8, GColorPurpleARGB8, GColorImperialPurpleARGB8}, //PURPLE
  {GColorCyanARGB8, GColorTiffanyBlueARGB8, GColorMidnightGreenARGB8}, //CYAN
  {GColorChromeYellowARGB8, GColorOrangeARGB8, GColorWindsorTanARGB8} //ORANGE    
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
