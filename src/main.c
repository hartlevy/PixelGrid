#include <pebble.h>
#include "pixel_grid.h"
  
static Window *s_main_window;
static Layer *bg_layer, *s_hands_layer, *s_battery_layer, *s_bt_layer, *s_date_layer;
static BitmapLayer *s_arm1_layer, *s_arm2_layer,*s_center_layer, *s_face_layer; 
static BitmapLayer *s_side1_layer, *s_side2_layer, *s_side3_layer, *s_side4_layer;
static BitmapLayer *s_pm_layer, *s_pm2_layer, *s_datebg_layer, *s_top_layer, *s_top2_layer;
static BitmapLayer  *s_face2_layer, *s_face3_layer, *s_face4_layer, *s_arm3_layer, *s_arm4_layer;
static GBitmap *s_arm1_bitmap, *s_arm2_bitmap, *s_center_bitmap, *s_pm_bitmap;
static GBitmap *s_datebg_bitmap, *s_face_bitmap;
static GBitmap *s_side1_bitmap, *s_side2_bitmap, *s_top_bitmap;
static GBitmap  *s_face2_bitmap, *s_face3_bitmap, *s_face4_bitmap, *s_arm3_bitmap, *s_arm4_bitmap;
static int seconds_color;
static int minutes_color;
static int hours_color;

static void fillPixel(Layer *layer, int16_t i, int16_t j, GContext *ctx){
  int startX = i * (RECTWIDTH);
  int startY = j * (RECTHEIGHT);

  GRect pixel = GRect(startX, startY, RECTWIDTH-1, RECTHEIGHT-1);
  graphics_fill_rect(ctx, pixel, 0, GCornerNone);
}

static void draw_shape(Layer *layer, GPoint points[], int n, uint8_t startx, uint8_t starty,  GContext *ctx){
  for(int i = 0; i < n; i++){
    fillPixel(layer, startx+points[i].x, starty + points[i].y, ctx);
  }
}

static void setColorShade(float c, uint8_t color, GContext *ctx){ 
  const uint8_t* colors;
  
  switch(color){
    case RED:
      colors = RED_COLOR_SET;    
      break;
    case BLUE:
      colors = BLUE_COLOR_SET;    
      break;
    case GREEN:
      colors = GREEN_COLOR_SET;    
      break;
    case YELLOW:
      colors = YELLOW_COLOR_SET;    
      break;
    case PURPLE:
      colors = PURPLE_COLOR_SET;    
      break;
    default:
      colors = WHITE_COLOR_SET;
      break;
  }
  
  if(c > HI_COLOR_THRESHOLD){
    graphics_context_set_fill_color(ctx, (GColor)colors[0]);
  }else if(c > MID_COLOR_THRESHOLD && c <= HI_COLOR_THRESHOLD){
    graphics_context_set_fill_color(ctx, (GColor)colors[1]);    
  }else if(c > LO_COLOR_THRESHOLD && c <= MID_COLOR_THRESHOLD){
    graphics_context_set_fill_color(ctx, (GColor)colors[2]);
  }else{
    graphics_context_set_fill_color(ctx, GColorOxfordBlue);
  }
  
}

static void plot(Layer *layer, uint8_t x, uint8_t y, float c, uint8_t colorset, GContext *ctx){
    setColorShade(c, colorset, ctx);
  //don't draw bg coloured pixels
    if(c > LO_COLOR_THRESHOLD){
      fillPixel(layer, x, y, ctx);
    }
}

// integer part of x
static uint8_t ipart(float x){
    return (uint8_t) x;
}


// fractional part of x
static float fpart(float x){
    if (x < 0.0){    
        return 1.0 - (x - (int)x);
    }
    return x - (int)x;
}

static float rfpart(float x){
    return 1.0 - fpart(x);
}

static void swap(uint8_t *i, uint8_t *j) {
   int t = *i;
   *i = *j;
   *j = t;
}


static void drawAliasLine(Layer *layer, uint8_t x0, uint8_t y0,uint8_t x1,uint8_t y1, uint8_t colorset, bool thick, GContext *ctx){
    bool steep = abs(y1 - y0) > abs(x1 - x0);
    
    if(steep){
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if(x0 > x1){
        swap(&x0, &x1);
        swap(&y0, &y1);
    }
    
    int16_t dx = x1 - x0;
    int16_t dy = y1 - y0;
    float gradient = (float) dy / (float) dx;
    float intery = y0; // first y-intersection for the main loop    

    for (uint8_t x = x0; x <= x1; x++){
        if(steep){
            if(thick){
              plot(layer, ipart(intery)-1, x, rfpart(intery)/2, colorset, ctx);
              plot(layer, ipart(intery), x, 1, colorset, ctx);     
            }else{
              plot(layer, ipart(intery), x, rfpart(intery), colorset, ctx);                   
            }
            plot(layer, ipart(intery)+1, x,  fpart(intery), colorset, ctx);
        }else{
            if(thick){
              plot(layer, x, ipart(intery)-1, rfpart(intery)/2, colorset, ctx);
              plot(layer, x, ipart(intery), 1, colorset, ctx);     
            }else{
              plot(layer, x, ipart(intery), rfpart(intery), colorset, ctx);                   
            }                 
            plot(layer, x, ipart(intery)+1, fpart(intery), colorset, ctx);
        }
        intery = intery + gradient;
    }
  
    //Make sure endpoint gets drawn
    if (steep){
        plot(layer, y1, x1, 1, colorset, ctx);
    }else{
        plot(layer, x1, y1, 1, colorset, ctx);
    }  
}


static GPoint createHand(int32_t angle, int16_t length, int x, int y){
  GPoint hand = {
    .x = (int16_t)(sin_lookup(angle) * length / TRIG_MAX_RATIO) + x,
    .y = (int16_t)(-cos_lookup(angle) * length / TRIG_MAX_RATIO) + y,
  };
  return hand;
}

static float* createHandFloat(int32_t angle, int16_t length, int x, int y){
  float * hand = malloc(sizeof(float) * 2);
  hand[0] = (float)sin_lookup(angle) * (float)length / (float)TRIG_MAX_RATIO + (float)x;
  hand[1] = -(float)cos_lookup(angle) * (float)length / (float)TRIG_MAX_RATIO + (float)y;

  return hand;
}

static void draw_digit(Layer *layer, uint8_t x, uint8_t y, uint8_t n, GContext *ctx){
  if(true){
    fillPixel(layer, x, y, ctx);
  }
  if(n != 4){
    fillPixel(layer, x+1, y, ctx);
  }
  if(n != 1){
    fillPixel(layer, x+2, y, ctx);
  }
  
  if((n > 3 && n != 7) || n == 0){
    fillPixel(layer, x, y+1, ctx);
  }
  if(n > 7 || n == 1){
    fillPixel(layer, x+1, y+1, ctx);
  }
  if(n != 5 && n != 6 && n != 1){
    fillPixel(layer, x+2, y+1, ctx);
  }
  
  if(n % 2 == 0 && n != 4){
    fillPixel(layer, x, y+2, ctx);
  }
  if(n > 0 && n < 7){
    fillPixel(layer, x+1, y+2, ctx);
  }
  if(n != 2 && n!=1){
    fillPixel(layer, x+2, y+2, ctx);
  }
  
  if(n != 4 && n != 7){
    fillPixel(layer, x, y+3, ctx);
    fillPixel(layer, x+1, y+3, ctx);    
  }
  if(true){
    fillPixel(layer, x+2, y+3, ctx);
  }  
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  GPoint start = { .x = WIDTH-19, .y = WIDTH + 2};
  
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int month = t->tm_mon;
  int day = t->tm_mday;
  
  int m1 = (month+1)/10;
  int m2 = (month+1)%10;
  int d1 = day/10;
  int d2 = day%10;
    
  graphics_context_set_fill_color(ctx, GColorWhite);  
  draw_digit(layer, start.x, start.y, d1, ctx );
  draw_digit(layer, start.x + 4, start.y, d2, ctx );                          
  draw_shape(layer, SLASH_POINTS.points, SLASH_POINTS.num_points, start.x, start.y, ctx);   
  draw_digit(layer, start.x + 11, start.y, m1, ctx );
  draw_digit(layer, start.x + 15, start.y, m2, ctx );
}


static void hands_update_proc(Layer *layer, GContext *ctx) {
  GPoint center = { .x = WIDTH/2, .y = WIDTH/2-1};
  int16_t second_hand_length = WIDTH / 2 - 3;
  int16_t minute_hand_length = WIDTH / 2 - 6;
  int16_t hour_hand_length = WIDTH / 2 - 9;
  
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  int32_t minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  int32_t hour_angle = (TRIG_MAX_ANGLE * ((((t->tm_hour) % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  
  //Create hands
  GPoint second_hand = createHand(second_angle,second_hand_length, center.x, center.y);
  GPoint minute_hand = createHand(minute_angle,minute_hand_length, center.x, center.y);
  GPoint hour_hand = createHand(hour_angle,hour_hand_length, center.x, center.y);
  
  // Draw hand
  drawAliasLine(layer, center.x, center.y, hour_hand.x, hour_hand.y, hours_color, true, ctx);
  drawAliasLine(layer, center.x, center.y, minute_hand.x, minute_hand.y, minutes_color, true, ctx);    
  drawAliasLine(layer, center.x, center.y, second_hand.x, second_hand.y, seconds_color, false, ctx);  
  
  // Draw A/P
  if(t->tm_hour >= 12){  
    graphics_context_set_fill_color(ctx, GColorYellow); 
    draw_shape(layer, PM_POINTS.points, PM_POINTS.num_points, WIDTH-10, WIDTH-2, ctx);  
  }  
}

static void draw_bg(Layer *this_layer, GContext *ctx) {  
  //fill bg
  GRect bg = GRect(0, 0, 144, 168);  
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bg, 0, GCornerNone);
  
  //Draw rect grid
  graphics_context_set_fill_color(ctx, GColorOxfordBlue);
  for(int i = 0; i < WIDTH; i++){
    for(int j = 0; j < HEIGHT; j++){
      fillPixel(this_layer, i, j, ctx);
    }
  }    
  graphics_context_set_fill_color(ctx, GColorWhite);  
  int32_t tic_angle = 0;
  GPoint tic_hand;
  
  //Draw hour tics
  for(int k = 0; k < 12; k++){
    tic_angle = TRIG_MAX_ANGLE * k / 12;
    tic_hand = createHand(tic_angle, WIDTH/2 - 1, WIDTH/2, WIDTH/2-1);
    fillPixel(this_layer, tic_hand.x, tic_hand.y, ctx);
    
    if(k == 0 || k == 6){
          fillPixel(this_layer, tic_hand.x - 1, tic_hand.y, ctx);
    }
    if(k == 3 || k == 9){
          fillPixel(this_layer, tic_hand.x, tic_hand.y -1, ctx);
    }    
  }
}

static void battery_update_proc(Layer *layer, GContext *ctx) {  
  BatteryChargeState state = battery_state_service_peek();
  
  uint8_t charge = state.charge_percent;
  GColor charge_color;
  GColor case_color = GColorWhite;  
  int bat_x = 1;
  int bat_y = WIDTH + 3;  
    
  if(state.is_plugged){
    case_color = GColorGreen;
  }
  
  if(charge >= BAT_WARN_LEVEL){
    charge_color = GColorGreen;
  }
  else if(charge > BAT_ALERT_LEVEL && charge <BAT_WARN_LEVEL){
    charge_color = GColorYellow;
  }
  else{
    charge_color = GColorRed;
  }    
  
  graphics_context_set_fill_color(ctx, case_color);  
  draw_shape(layer, BAT_CASE_POINTS.points, BAT_CASE_POINTS.num_points, bat_x, bat_y, ctx);             

  graphics_context_set_fill_color(ctx, charge_color);         
  for(int i = 0; i < charge/10; i++ ){    
    //battery fill
    fillPixel(layer, bat_x + i + 1, bat_y+1, ctx);    
  }
  
  //Charge icon
  if(state.is_charging){
    graphics_context_set_fill_color(ctx, GColorYellow); 
    draw_shape(layer, CHARGE_POINTS.points, CHARGE_POINTS.num_points, bat_x, bat_y-1, ctx);           
  }
}

static void bt_update_proc(Layer *layer, GContext *ctx) {
  bool connected = bluetooth_connection_service_peek();
  int bt_x = 1;
  int bt_y = WIDTH - 3; 
  
  if(connected){
    graphics_context_set_fill_color(ctx, GColorBlueMoon);           
    draw_shape(layer, BT_LOGO_POINTS.points, BT_LOGO_POINTS.num_points, bt_x, bt_y, ctx);               
  }
}


static void bt_handler(bool connected) {
  if(!connected){
    vibes_short_pulse();
  }
  layer_mark_dirty(s_bt_layer);
}
static void battery_handler(BatteryChargeState new_state) {
  layer_mark_dirty(s_battery_layer);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (direction > 0){
    seconds_color ++;
  }else{
    seconds_color --;
  }
  seconds_color = (seconds_color + 6)%6;
  layer_mark_dirty(s_hands_layer);  
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
  if(units_changed & DAY_UNIT){
      layer_mark_dirty(s_date_layer);
  }     
}

static void create_bitmap_add_layer(GBitmap *bitmap, BitmapLayer *layer, Layer *window_layer,
                                    int16_t x, int16_t y, int16_t w, int16_t h, uint32_t resource_id){
    bitmap = gbitmap_create_with_resource(resource_id);  
    layer = bitmap_layer_create(GRect(x, y, w, h));
    bitmap_layer_set_background_color(layer,GColorBlack);
    bitmap_layer_set_bitmap(layer, bitmap);  
    layer_add_child(window_layer, bitmap_layer_get_layer(layer));  
}

static void destroyBitmap( BitmapLayer *layer, GBitmap *bitmap){
    layer_remove_from_parent(bitmap_layer_get_layer(layer));  
    bitmap_layer_destroy(layer);
    gbitmap_destroy(bitmap);
}


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
/*  //create bg layer 
  bg_layer = layer_create(bounds);
  layer_set_update_proc(bg_layer, draw_bg);
  layer_add_child(window_layer, bg_layer);
  */
  
  //create bg layers
  create_bitmap_add_layer(s_arm1_bitmap, s_arm1_layer, window_layer, RECTWIDTH*17,0,RECTWIDTH*3,RECTWIDTH*16,RESOURCE_ID_BG_ARM1);
  create_bitmap_add_layer(s_arm2_bitmap, s_arm2_layer, window_layer, RECTWIDTH*17,RECTWIDTH*19,RECTWIDTH*3,RECTWIDTH*19,RESOURCE_ID_BG_ARM2);
  create_bitmap_add_layer(s_arm3_bitmap, s_arm3_layer, window_layer, RECTWIDTH*20,RECTWIDTH*16,RECTWIDTH*16,RECTWIDTH*3, RESOURCE_ID_BG_ARM3);  
  create_bitmap_add_layer(s_arm4_bitmap, s_arm4_layer, window_layer, 0,RECTWIDTH*16,RECTWIDTH*17,RECTWIDTH*3, RESOURCE_ID_BG_ARM4);
  create_bitmap_add_layer(s_center_bitmap, s_center_layer, window_layer, RECTWIDTH*17,RECTWIDTH*16,RECTWIDTH*3,RECTWIDTH*3,RESOURCE_ID_BG_CENTER);
  create_bitmap_add_layer(s_face_bitmap, s_face_layer, window_layer, RECTWIDTH*4,RECTWIDTH*3,RECTWIDTH*13,RECTWIDTH*13,RESOURCE_ID_BG_FACE);
  create_bitmap_add_layer(s_face2_bitmap, s_face2_layer, window_layer, RECTWIDTH*20,RECTWIDTH*3,RECTWIDTH*13,RECTWIDTH*13,RESOURCE_ID_BG_FACE2);
  create_bitmap_add_layer(s_face3_bitmap, s_face3_layer, window_layer, RECTWIDTH*20,RECTWIDTH*19,RECTWIDTH*13,RECTWIDTH*13,RESOURCE_ID_BG_FACE3);
  create_bitmap_add_layer(s_face4_bitmap, s_face4_layer, window_layer, RECTWIDTH*4,RECTWIDTH*19,RECTWIDTH*13,RECTWIDTH*13,RESOURCE_ID_BG_FACE4);
  create_bitmap_add_layer(s_side1_bitmap, s_side1_layer, window_layer, 0,0,RECTWIDTH*4,RECTWIDTH*16,RESOURCE_ID_BG_SIDE1);
  create_bitmap_add_layer(s_side2_bitmap, s_side2_layer, window_layer, 0,RECTWIDTH*19,RECTWIDTH*4,RECTWIDTH*13,RESOURCE_ID_BG_SIDE2);
  create_bitmap_add_layer(s_side1_bitmap, s_side3_layer, window_layer, RECTWIDTH*33,0,RECTWIDTH*3,RECTWIDTH*16,RESOURCE_ID_BG_SIDE1);
  create_bitmap_add_layer(s_side2_bitmap, s_side4_layer, window_layer, RECTWIDTH*33,RECTWIDTH*19,RECTWIDTH*3,RECTWIDTH*13,RESOURCE_ID_BG_SIDE2);
  create_bitmap_add_layer(s_top_bitmap, s_top_layer, window_layer, RECTWIDTH*4,0,RECTWIDTH*13,RECTWIDTH*3,RESOURCE_ID_BG_TOP);
  create_bitmap_add_layer(s_top_bitmap, s_top2_layer, window_layer, RECTWIDTH*20,0,RECTWIDTH*13,RECTWIDTH*3,RESOURCE_ID_BG_TOP);
  create_bitmap_add_layer(s_pm_bitmap, s_pm_layer, window_layer, 0,RECTWIDTH*32,RECTWIDTH*17,RECTWIDTH*6,RESOURCE_ID_BG_PM);
  create_bitmap_add_layer(s_pm_bitmap, s_pm2_layer, window_layer, RECTWIDTH*20,RECTWIDTH*32,RECTWIDTH*17,RECTWIDTH*6,RESOURCE_ID_BG_PM);
  create_bitmap_add_layer(s_datebg_bitmap, s_datebg_layer, window_layer, 0,RECTWIDTH*38,RECTWIDTH*36,RECTWIDTH*4,RESOURCE_ID_BG_DATE); 
  
  //create hands layer
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  
  //create date layer
  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);
  
  //create battery layer
  s_battery_layer = layer_create(bounds);
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);
  
  //create bluetooth layer
  s_bt_layer = layer_create(bounds);
  layer_set_update_proc(s_bt_layer, bt_update_proc);
  layer_add_child(window_layer, s_bt_layer);  

  layer_mark_dirty(window_get_root_layer(s_main_window));
}

static void main_window_unload(Window *window) {
    // Destroy Layers
    layer_destroy(bg_layer); 
    layer_destroy(s_hands_layer);    
    layer_destroy(s_battery_layer);  
    layer_destroy(s_date_layer);    
    layer_destroy(s_bt_layer);  
  
    destroyBitmap(s_arm1_layer,s_arm1_bitmap);
    destroyBitmap(s_arm2_layer,s_arm2_bitmap);
    destroyBitmap(s_arm3_layer,s_arm3_bitmap);
    destroyBitmap(s_arm4_layer,s_arm4_bitmap);  
    destroyBitmap(s_side1_layer,s_side1_bitmap);
    destroyBitmap(s_side2_layer,s_side2_bitmap);
    bitmap_layer_destroy(s_side3_layer);
    bitmap_layer_destroy(s_side4_layer);  
    destroyBitmap(s_center_layer,s_center_bitmap);
    destroyBitmap(s_pm_layer,s_pm_bitmap);
    bitmap_layer_destroy(s_pm2_layer);  
    destroyBitmap(s_datebg_layer,s_datebg_bitmap);
    destroyBitmap(s_face_layer,s_face_bitmap);
    destroyBitmap(s_face2_layer,s_face2_bitmap);
    destroyBitmap(s_face3_layer,s_face3_bitmap);
    destroyBitmap(s_face4_layer,s_face4_bitmap);
    destroyBitmap(s_top_layer,s_top_bitmap);
    bitmap_layer_destroy(s_top2_layer);
  
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  seconds_color = BLUE;
  minutes_color = WHITE;
  hours_color = WHITE;
  
  // Register with Services
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  accel_tap_service_subscribe(tap_handler);  
  battery_state_service_subscribe(battery_handler);
  bluetooth_connection_service_subscribe(bt_handler);  
}


static void deinit() {
    tick_timer_service_unsubscribe(); 
    accel_tap_service_unsubscribe();
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    // Destroy Window
    window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

