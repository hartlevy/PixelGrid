#include <pebble.h>
#include "pixel_grid.h"
#include "gbitmap_color_palette_manipulator.h"
  
static Window *s_main_window;
static Layer *s_hands_layer, *s_battery_layer, *s_bt_layer, *s_date_layer, *s_temp_layer;

static BitmapLayer *s_faces_layer[9];
static GBitmap *s_faces_bitmap[9];

static BitmapLayer *s_sides_layer[6];
static GBitmap *s_sides_bitmap[6];

static BitmapLayer *s_details_layer[3];
static GBitmap *s_details_bitmap[3];

static BitmapLayer *s_date_digits_layer[5];
static GBitmap *s_date_digits_bitmap[5];

static BitmapLayer *s_temp_digits_layer[4];
static GBitmap *s_temp_digits_bitmap[4];

static BitmapLayer *s_day_layer;
static GBitmap *s_day_bitmap;

static BitmapLayer *s_bt_img_layer;
static GBitmap *s_bt_img_bitmap;

static int seconds_color;
static int minutes_color;
static int hours_color;
static int bt_image_type;
static int temp_scale;
static int date_format;
static bool hide_second_hand;
static bool show_animation;

static int tap_counter = -1;
static int hour_pos = 0;
static int minute_pos = 0;
static int second_pos = 0;
static bool hour_ready = false;
static bool minute_ready = false;
static bool clock_ready = false;

AppTimer *timer;
const int delta = 40;

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
  if(c > HI_COLOR_THRESHOLD){
    graphics_context_set_fill_color(ctx, (GColor)COLOR_SETS[color][0]);
  }else if(c > MID_COLOR_THRESHOLD && c <= HI_COLOR_THRESHOLD){
    graphics_context_set_fill_color(ctx, (GColor)COLOR_SETS[color][1]);    
  }else if(c > LO_COLOR_THRESHOLD && c <= MID_COLOR_THRESHOLD){
    graphics_context_set_fill_color(ctx, (GColor)COLOR_SETS[color][2]);
  }else{
    graphics_context_set_fill_color(ctx, GColorOxfordBlue);
  }  
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, uint8_t x, uint8_t  y) {
  GBitmap *old_image = *bmp_image;
  //*bmp_image = gbitmap_create_with_palette(COLOUR_USER, resource_id);
  *bmp_image = gbitmap_create_with_resource(resource_id);
  
  GPoint origin = { .x = x, .y = y};
  
  GRect frame = (GRect) {
    .origin = origin,
    .size = gbitmap_get_bounds(*bmp_image).size
  };

	bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
	layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
  
  if (old_image != NULL) {
		gbitmap_destroy(old_image);
		old_image = NULL;
  }        
}

static BitmapLayer* create_bitmap_layer(GBitmap *bitmap, Layer *window_layer,
                                    int16_t x, int16_t y, int16_t w, int16_t h){
    GRect frame = GRect(x, y, w, h);
    BitmapLayer* bmp_layer = bitmap_layer_create(frame);
    bitmap_layer_set_background_color(bmp_layer,GColorBlack);
    bitmap_layer_set_bitmap(bmp_layer, bitmap);  
    //replace_gbitmap_color(GColorOxfordBlue, GColorBlack, bitmap, bmp_layer);  
    layer_add_child(window_layer, bitmap_layer_get_layer(bmp_layer));  
    return bmp_layer;
}

static void destroy_bitmap_layer( BitmapLayer *layer, GBitmap *bitmap){
    layer_remove_from_parent(bitmap_layer_get_layer(layer));  
    bitmap_layer_destroy(layer);
    if (bitmap != NULL){ 
      gbitmap_destroy(bitmap);
    }
}

static void plot(Layer *layer, uint8_t x, uint8_t y, float c, uint8_t colorset, GContext *ctx){
  setColorShade(c, colorset, ctx);  
  
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


static void hands_update_proc(Layer *layer, GContext *ctx) {
  GPoint center = { .x = WIDTH/2, .y = WIDTH/2-1};
  int16_t second_hand_length = (WIDTH / 2)*5/6;
  int16_t minute_hand_length = (WIDTH / 2)*2/3;
  int16_t hour_hand_length = (WIDTH / 2)/2;
  
  uint8_t pm_x = WIDTH - 10;
  uint8_t pm_y = WIDTH - 2;
  
  #if defined(PBL_ROUND)  
  pm_y = WIDTH/2 + 18;
  pm_x = WIDTH/2 - 4;  
  center.y = center.y + 1;
  #endif
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  int hour = (((t->tm_hour) % 12) * 6) + (t->tm_min / 10);
  
  //Start-up animation.
  //Flags indicate when each hand is done
  if(show_animation && !clock_ready){
    if(!hour_ready){
      hour_pos+= 2;
      if(hour_pos >= hour){
        hour_pos = hour;
        hour_ready = true;
      }
    }else if(!minute_ready){
      minute_pos+= 2;
      if(minute_pos >= t->tm_min){
        minute_pos = t->tm_min;
        minute_ready = true;
      }
    }else{
      second_pos+= 2;
      if(second_pos >= t->tm_sec){
        second_pos = t->tm_sec;    
        clock_ready = true;
      }
    }
  }else{
    hour_pos = hour;
    second_pos = t->tm_sec;    
    minute_pos = t->tm_min;
  }
  
  int32_t second_angle = TRIG_MAX_ANGLE * second_pos / 60;
  int32_t minute_angle = TRIG_MAX_ANGLE * minute_pos / 60;
  int32_t hour_angle = TRIG_MAX_ANGLE * hour_pos / 72;
  
  //Create hands
  GPoint second_hand = createHand(second_angle,second_hand_length, center.x, center.y);
  GPoint minute_hand = createHand(minute_angle,minute_hand_length, center.x, center.y);
  GPoint hour_hand = createHand(hour_angle,hour_hand_length, center.x, center.y);
  
  // Draw hand
  drawAliasLine(layer, center.x, center.y, hour_hand.x, hour_hand.y, hours_color, true, ctx);
  drawAliasLine(layer, center.x, center.y, minute_hand.x, minute_hand.y, minutes_color, true, ctx); 
  if(!hide_second_hand){
    drawAliasLine(layer, center.x, center.y, second_hand.x, second_hand.y, seconds_color, false, ctx); 
  }
  
  // Draw PM
  if(t->tm_hour >= 0){  
    graphics_context_set_fill_color(ctx, GColorYellow); 
    draw_shape(layer, PM_POINTS.points, PM_POINTS.num_points, pm_x, pm_y, ctx);  
  }    
  
}

void timer_callback(void *data) {
    if(!clock_ready){
      layer_mark_dirty(s_hands_layer);
 
      //Register next execution
      timer = app_timer_register(delta, (AppTimerCallback) timer_callback, NULL);
    }
}

static void battery_update_proc(Layer *layer, GContext *ctx) {  
  BatteryChargeState state = battery_state_service_peek();
  
  uint8_t charge = state.charge_percent;
  GColor charge_color;
  GColor case_color = GColorWhite;  

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
  draw_shape(layer, BAT_CASE_POINTS.points, BAT_CASE_POINTS.num_points, 0, 0, ctx);             

  graphics_context_set_fill_color(ctx, charge_color);         
  for(int i = 0; i < charge/10; i++ ){    
    //battery fill
    fillPixel(layer, i + 1, 1, ctx);    
  }
  
  //Charge icon
  if(state.is_charging){
    graphics_context_set_fill_color(ctx, GColorYellow); 
    draw_shape(layer, CHARGE_POINTS.points, CHARGE_POINTS.num_points, 3, 0, ctx);           
  }
}


static void update_bt_img(bool connected) {  
  if(!connected){
    vibes_short_pulse();
    layer_set_hidden(bitmap_layer_get_layer(s_bt_img_layer), true);      
  }else{
    int bt_id = RESOURCE_ID_BT1;
    if(bt_image_type == BT_IMAGE_LARGE){
      bt_id = RESOURCE_ID_BT2;
    }
    set_container_image(&s_bt_img_bitmap, s_bt_img_layer, bt_id, 0, 0);      
    layer_set_hidden(bitmap_layer_get_layer(s_bt_img_layer), false);      
  }
}

static void update_date(){
  uint8_t x = 0;//(WIDTH-19)*RECTWIDTH;
  uint8_t y = 0;//;(WIDTH + 2)*RECTWIDTH;  
  uint8_t day_x = 6*RECTWIDTH;
  uint8_t day_y = 0;
  #if defined(PBL_ROUND)
  day_x = 3*RECTWIDTH;
  #endif  
  
  GPoint origin = layer_get_frame(s_date_layer).origin;
  
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  uint8_t month = t->tm_mon;
  uint8_t day = t->tm_mday;
  uint8_t wday = t->tm_wday;

  if(date_format == MMDD_DATE_FORMAT){
    swap(&month,&day);
  }
  uint8_t m1 = (month+1)/10;
  uint8_t m2 = (month+1)%10;
  uint8_t d1 = day/10;
  uint8_t d2 = day%10;
  
	set_container_image(&s_date_digits_bitmap[0], s_date_digits_layer[0], DIGIT_IMAGE_RESOURCE_IDS[d1], x, y);  
	set_container_image(&s_date_digits_bitmap[1], s_date_digits_layer[1], DIGIT_IMAGE_RESOURCE_IDS[d2], x + 4*RECTWIDTH, y);  
	set_container_image(&s_date_digits_bitmap[2], s_date_digits_layer[2], RESOURCE_ID_SLASH, x + 8*RECTWIDTH, y);  
	set_container_image(&s_date_digits_bitmap[3], s_date_digits_layer[3], DIGIT_IMAGE_RESOURCE_IDS[m1], x + 11*RECTWIDTH, y);  
	set_container_image(&s_date_digits_bitmap[4], s_date_digits_layer[4], DIGIT_IMAGE_RESOURCE_IDS[m2], x + 15*RECTWIDTH, y);    


	set_container_image(&s_day_bitmap, s_day_layer, DAY_NAME_IMAGE_RESOURCE_IDS[wday], origin.x+day_x, origin.y + day_y);    
}


static void show_tap_display(bool show){
  //swap visible layers
  layer_set_hidden(bitmap_layer_get_layer(s_day_layer), !show);
  layer_set_hidden(s_date_layer, show); 
  
  layer_set_hidden(s_temp_layer, !show);     
  layer_set_hidden(s_bt_layer, show);  
  layer_set_hidden(s_battery_layer, show);
}

static void request_temperature(){
  // Begin dictionary
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  // Add a key-value pair
  dict_write_uint8(iter, 0, 0);

  // Send the message!
  app_message_outbox_send();
}


static void bt_handler(bool connected) {
  update_bt_img(connected);
}

static void battery_handler(BatteryChargeState new_state) {
  layer_mark_dirty(s_battery_layer);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  /*if (direction > 0){
    seconds_color ++;
  }else{
    seconds_color --;
  }
  seconds_color = (seconds_color + NUM_COLOR)%NUM_COLOR;  
*/
  
  tap_counter = TAP_DURATION_MED;
  show_tap_display(true);
  layer_mark_dirty(s_hands_layer);   
}

static void handle_second_tick(struct tm *t, TimeUnits units_changed) {
  if(clock_ready){
    if((!hide_second_hand && (units_changed & SECOND_UNIT)) || (units_changed & MINUTE_UNIT)){
        layer_mark_dirty(s_hands_layer);
    }
  }
  if(units_changed & DAY_UNIT){
      update_date();
  }
  
  // Get weather update every 30 minutes
  if(t->tm_min % 30 == 0 && t->tm_sec % 60 == 0) {
    request_temperature();
  }  
  
  if(tap_counter >= 0){
    if(tap_counter == 0){
      //switch to timer
      show_tap_display(false);
    }
    tap_counter--;
  }  
}

static void parse_config_message(DictionaryIterator *iterator, void *context){
  
  Tuple *t = dict_read_first(iterator);
  
APP_LOG(APP_LOG_LEVEL_ERROR, "configgingg!!");  
    // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_HIDE_SECONDS:
      hide_second_hand = (int)(t->value->int32);
      persist_write_bool(KEY_HIDE_SECONDS,hide_second_hand);
      break;
    case KEY_BT_LOGO_TYPE:
      if((int)t->value->int32){
        bt_image_type = BT_IMAGE_LARGE;
      }else{
        bt_image_type = BT_IMAGE_SMALL;        
      }
      update_bt_img(bluetooth_connection_service_peek());       
      persist_write_int(KEY_BT_LOGO_TYPE, bt_image_type);   
      break;      
    case KEY_TEMP_SCALE:
      if(temp_scale != (int)t->value->int32){
        temp_scale = (int)t->value->int32;
        request_temperature();
      }      
      persist_write_int(KEY_TEMP_SCALE, temp_scale);            
      break;      
    case KEY_SHOW_ANIMATION:
      show_animation = (int)(t->value->int32);
      persist_write_bool(KEY_SHOW_ANIMATION,show_animation);
      break;
    case KEY_HOUR_COLOR:
      hours_color = (int)t->value->int32;
      persist_write_int(KEY_HOUR_COLOR, hours_color);                     
      break;
    case KEY_MINUTE_COLOR:
      minutes_color = (int)t->value->int32;
      persist_write_int(KEY_MINUTE_COLOR, minutes_color);                              
      break;
    case KEY_SECOND_COLOR:
      seconds_color = (int)t->value->int32;
      persist_write_int(KEY_SECOND_COLOR, seconds_color);                                       
      break;      
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
    
    t = dict_read_next(iterator);
  }
  
  layer_mark_dirty(s_hands_layer);
}

static void parse_weather_message(DictionaryIterator *iterator, void *context){
  int temperature = 0;
  bool got_temperature = false;
  bool neg_temp = false;  
  
      APP_LOG(APP_LOG_LEVEL_ERROR, "Getting weather");
  
  Tuple *t = dict_read_first(iterator);

    // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      temperature = (int)t->value->int32;
      if(temp_scale == FAHRENHEIT_SCALE){
        temperature = temperature * 9/5 + 32;
      }
      got_temperature = true;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }  
  
  APP_LOG(APP_LOG_LEVEL_ERROR, "Temp: %d", temperature);

  if(got_temperature){
    if(temperature < 0){
      temperature = -temperature;
      neg_temp = true;
    }
    
    int t1 = temperature/100;
    int t2 = (temperature%100)/10;
    int t3 = temperature%10;
    
    int x = 0;
    int y = 0;
    #if defined(PBL_ROUND)
    y = y + RECTWIDTH;    
    if(t1 == 0 && !neg_temp){  
      x = x + RECTWIDTH;
    }
    #endif
    
    if(t1 != 0){
      set_container_image(&s_temp_digits_bitmap[0], s_temp_digits_layer[0], DIGIT_IMAGE_RESOURCE_IDS[t1], x, y);  
      x += 4*RECTWIDTH;
    } else if(neg_temp){
      set_container_image(&s_temp_digits_bitmap[0], s_temp_digits_layer[0], RESOURCE_ID_NEGATIVE, x, y);        
      x += 4*RECTWIDTH;
    } else if(s_temp_digits_bitmap[0] != NULL){
      gbitmap_destroy(s_temp_digits_bitmap[0]);
      s_temp_digits_bitmap[0] = NULL;
      bitmap_layer_set_bitmap(s_temp_digits_layer[0], NULL);      
    }
    if(t2 != 0 || t1 != 0){
      set_container_image(&s_temp_digits_bitmap[1], s_temp_digits_layer[1], DIGIT_IMAGE_RESOURCE_IDS[t2], x, y);  
      x += 4*RECTWIDTH;  	
    }
    else{
      x += 2*RECTWIDTH;
      if(s_temp_digits_bitmap[1] != NULL){
        gbitmap_destroy(s_temp_digits_bitmap[1]);
        s_temp_digits_bitmap[1] = NULL;
        bitmap_layer_set_bitmap(s_temp_digits_layer[1], NULL);
      }
    }
    set_container_image(&s_temp_digits_bitmap[2], s_temp_digits_layer[2], DIGIT_IMAGE_RESOURCE_IDS[t3], x, y);  
    x += 4*RECTWIDTH;  	
    set_container_image(&s_temp_digits_bitmap[3], s_temp_digits_layer[3], RESOURCE_ID_DEGREE, x, y);          
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {  
  // Read first item
  Tuple *weather_tuple = dict_find(iterator, WEATHER_MESSAGE);
  //Tuple *config_tuple = dict_find(iterator, CONFIG_MESSAGE);
  
  if((int)weather_tuple->value->int32 == 1){
    APP_LOG(APP_LOG_LEVEL_ERROR, "Get weather");
    parse_weather_message(iterator,  context);
  }
  else{
    APP_LOG(APP_LOG_LEVEL_ERROR, "Get config");    
    parse_config_message(iterator,  context);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GRect dummy_frame = { {0, 0}, {0, 0} };
  
  uint8_t c_x = RECTWIDTH*(int)(bounds.size.w/(2*RECTWIDTH));
  //uint8_t datelayer_y = c_x + RECTWIDTH*20;
  uint8_t batlayer_y = c_x + RECTWIDTH*21;  
  uint8_t batlayer_x = RECTWIDTH;  
  uint8_t daylayer_y = c_x + RECTWIDTH*20;  
  uint8_t daylayer_x = c_x - RECTWIDTH;    
  uint8_t bt_y = c_x + RECTWIDTH*14;  
  uint8_t bt_x = RECTWIDTH;    
  #if defined(PBL_ROUND)
  //datelayer_y = 0;
  batlayer_y = 2*RECTWIDTH;
  batlayer_x = c_x - 6*RECTWIDTH;
  daylayer_y = c_x + RECTWIDTH*13;  
  daylayer_x = c_x - RECTWIDTH*8;    
  bt_x = c_x - 3*RECTWIDTH;
  bt_y = c_x - RECTWIDTH*17;
  
  s_faces_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_BG_ROUND);  
  s_faces_layer[0] = create_bitmap_layer(s_faces_bitmap[0], window_layer, 0,0,RECTWIDTH*WIDTH,RECTWIDTH*HEIGHT);
    
  #elif defined (PBL_RECT)
  //create bg layers
  uint8_t datelayer_y = c_x + RECTWIDTH*20;
  
  s_faces_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_BG_ARM1);  
  s_faces_layer[0] = create_bitmap_layer(s_faces_bitmap[0], window_layer, c_x - RECTWIDTH,c_x - RECTWIDTH*18,RECTWIDTH*3,RECTWIDTH*16);
  s_faces_bitmap[1] = gbitmap_create_with_resource(RESOURCE_ID_BG_ARM2);   
  s_faces_layer[1] = create_bitmap_layer(s_faces_bitmap[1], window_layer, c_x - RECTWIDTH,c_x + RECTWIDTH,RECTWIDTH*3,RECTWIDTH*19);
  s_faces_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_BG_ARM3);    
  s_faces_layer[2] = create_bitmap_layer(s_faces_bitmap[2], window_layer, c_x + RECTWIDTH*2, c_x - RECTWIDTH*2,RECTWIDTH*16,RECTWIDTH*3);  
  s_faces_bitmap[3] = gbitmap_create_with_resource(RESOURCE_ID_BG_ARM4);     
  s_faces_layer[3] = create_bitmap_layer(s_faces_bitmap[3], window_layer, c_x - RECTWIDTH*18,c_x - RECTWIDTH*2,RECTWIDTH*17,RECTWIDTH*3);
  s_faces_bitmap[4] = gbitmap_create_with_resource(RESOURCE_ID_BG_CENTER);     
  s_faces_layer[4] = create_bitmap_layer(s_faces_bitmap[4], window_layer,  c_x - RECTWIDTH,c_x - RECTWIDTH*2,RECTWIDTH*3,RECTWIDTH*3);
  s_faces_bitmap[5] = gbitmap_create_with_resource(RESOURCE_ID_BG_FACE);     
  s_faces_layer[5] = create_bitmap_layer(s_faces_bitmap[5], window_layer, c_x - RECTWIDTH*14,c_x - RECTWIDTH*15,RECTWIDTH*13,RECTWIDTH*13);
  s_faces_bitmap[6] = gbitmap_create_with_resource(RESOURCE_ID_BG_FACE2);     
  s_faces_layer[6] = create_bitmap_layer(s_faces_bitmap[6], window_layer, c_x + RECTWIDTH*2,c_x - RECTWIDTH*15,RECTWIDTH*13,RECTWIDTH*13);
  s_faces_bitmap[7] = gbitmap_create_with_resource(RESOURCE_ID_BG_FACE3);     
  s_faces_layer[7] = create_bitmap_layer(s_faces_bitmap[7], window_layer, c_x + RECTWIDTH*2,c_x + RECTWIDTH,RECTWIDTH*13,RECTWIDTH*13);
  s_faces_bitmap[8] = gbitmap_create_with_resource(RESOURCE_ID_BG_FACE4);     
  s_faces_layer[8] = create_bitmap_layer(s_faces_bitmap[8], window_layer, c_x - RECTWIDTH*14,c_x + RECTWIDTH,RECTWIDTH*13,RECTWIDTH*13);

  s_sides_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_BG_SIDE1);     
  s_sides_layer[0] = create_bitmap_layer(s_sides_bitmap[0], window_layer, c_x - RECTWIDTH*18,c_x - RECTWIDTH*18,RECTWIDTH*4,RECTWIDTH*16);
  s_sides_bitmap[1] = gbitmap_create_with_resource(RESOURCE_ID_BG_SIDE2);     
  s_sides_layer[1] = create_bitmap_layer(s_sides_bitmap[1], window_layer, c_x - RECTWIDTH*18,c_x + RECTWIDTH,RECTWIDTH*4,RECTWIDTH*13);
  s_sides_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_BG_SIDE1);     
  s_sides_layer[2] = create_bitmap_layer(s_sides_bitmap[2], window_layer, c_x + RECTWIDTH*15,c_x - RECTWIDTH*18,RECTWIDTH*3,RECTWIDTH*16);
  s_sides_bitmap[3] = gbitmap_create_with_resource(RESOURCE_ID_BG_SIDE2);     
  s_sides_layer[3] = create_bitmap_layer(s_sides_bitmap[3], window_layer, c_x + RECTWIDTH*15,c_x + RECTWIDTH,RECTWIDTH*3,RECTWIDTH*13);
  s_sides_bitmap[4] = gbitmap_create_with_resource(RESOURCE_ID_BG_TOP);     
  s_sides_layer[4] = create_bitmap_layer(s_sides_bitmap[4], window_layer, c_x - RECTWIDTH*14,c_x - RECTWIDTH*18,RECTWIDTH*13,RECTWIDTH*3);
  s_sides_bitmap[5] = gbitmap_create_with_resource(RESOURCE_ID_BG_TOP);     
  s_sides_layer[5] = create_bitmap_layer(s_sides_bitmap[5], window_layer, c_x + RECTWIDTH*2,c_x - RECTWIDTH*18,RECTWIDTH*13,RECTWIDTH*3);
 
  s_details_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_BG_PM);     
  s_details_layer[0] = create_bitmap_layer(s_details_bitmap[0], window_layer, c_x - RECTWIDTH*18,c_x + RECTWIDTH*14,RECTWIDTH*17,RECTWIDTH*6);
  s_details_bitmap[1] = gbitmap_create_with_resource(RESOURCE_ID_BG_PM);     
  s_details_layer[1] = create_bitmap_layer(s_details_bitmap[1], window_layer, c_x + RECTWIDTH*2,c_x + RECTWIDTH*14,RECTWIDTH*17,RECTWIDTH*6);
  s_details_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_BG_DATE);     
  s_details_layer[2] = create_bitmap_layer(s_details_bitmap[2], window_layer, c_x - RECTWIDTH*18,datelayer_y,RECTWIDTH*36,RECTWIDTH*4); 
  #endif
  
  
  
  //create hands layer
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  
  //create date layer
  s_date_layer = layer_create(GRect(daylayer_x,daylayer_y,20*RECTWIDTH,4*RECTWIDTH));
 // layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);
  
  memset(&s_date_digits_layer, 0, sizeof(s_date_digits_layer));
  
  for (int i = 0; i < 5; ++i) {
    s_date_digits_layer[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(s_date_layer, bitmap_layer_get_layer(s_date_digits_layer[i]));
  }
    
  //create day of week layer
  s_day_layer = bitmap_layer_create(GRect(daylayer_x,daylayer_y,20*RECTWIDTH,4*RECTWIDTH));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_day_layer));
  layer_set_hidden(bitmap_layer_get_layer(s_day_layer), true);
  
  //create temperature layer
  s_temp_layer = layer_create(GRect(batlayer_x + RECTWIDTH, batlayer_y - RECTWIDTH,17*RECTWIDTH,5*RECTWIDTH));
  layer_add_child(window_layer, s_temp_layer);
  
  memset(&s_temp_digits_layer, 0, sizeof(s_temp_digits_layer));
  
  for (int i = 0; i < 4; ++i) {
    s_temp_digits_layer[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(s_temp_layer, bitmap_layer_get_layer(s_temp_digits_layer[i]));
  }  
  
  layer_set_hidden(s_temp_layer, true);
  
  //create battery layer
  s_battery_layer = layer_create(GRect(batlayer_x, batlayer_y,13*RECTWIDTH,3*RECTWIDTH));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);
  
  //create bluetooth layer
  s_bt_layer = layer_create(GRect(bt_x, bt_y,7*RECTWIDTH,7*RECTWIDTH));
  layer_add_child(window_layer, s_bt_layer);  

  //Bluetooth img
  s_bt_img_layer = bitmap_layer_create(dummy_frame);
  layer_add_child(s_bt_layer, bitmap_layer_get_layer(s_bt_img_layer)); 
    
  
  //Initial draw of details
  update_date();  
  update_bt_img(bluetooth_connection_service_peek());  

  layer_mark_dirty(window_get_root_layer(s_main_window));
}

static void main_window_unload(Window *window) {
    // Destroy Layers
  
  for(int i = 0; i < 9; i++){
    destroy_bitmap_layer(s_faces_layer[i], s_faces_bitmap[i] );
  }
  
  for(int i = 0; i < 6; i++){
    destroy_bitmap_layer(s_sides_layer[i], s_sides_bitmap[i] );    
  }
  
  for(int i = 0; i < 3; i++){
    destroy_bitmap_layer(s_details_layer[i], s_details_bitmap[i] );    
  }   
  
  for(int i = 0; i < 5; i++){
    destroy_bitmap_layer(s_date_digits_layer[i], s_date_digits_bitmap[i] );    
  }   
  
  for(int i = 0; i < 4; i++){
    destroy_bitmap_layer(s_temp_digits_layer[i], s_temp_digits_bitmap[i] );    
  }  
  
  destroy_bitmap_layer(s_bt_img_layer, s_bt_img_bitmap);   
  destroy_bitmap_layer(s_day_layer, s_day_bitmap);        
  
  layer_destroy(s_hands_layer);    
  layer_destroy(s_battery_layer);  
  layer_destroy(s_date_layer);    
  layer_destroy(s_bt_layer);  
  layer_destroy(s_temp_layer); 
  
}


static void init() {
  
  srand(time(NULL));
  
  seconds_color = BLUE;
  minutes_color = WHITE;
  hours_color = WHITE;
  bt_image_type = BT_IMAGE_SMALL;
  temp_scale = CELSIUS_SCALE; 
  date_format = DDMM_DATE_FORMAT;
  hide_second_hand = false;
  show_animation = true;  
  
  if(persist_exists(KEY_SECOND_COLOR)){
    seconds_color = persist_read_int(KEY_SECOND_COLOR);
  }
  if(persist_exists(KEY_HOUR_COLOR)){
    hours_color = persist_read_int(KEY_HOUR_COLOR);
  }
  if(persist_exists(KEY_MINUTE_COLOR)){
    minutes_color = persist_read_int(KEY_MINUTE_COLOR);
  }
  if(persist_exists(KEY_TEMP_SCALE)){
    temp_scale = persist_read_int(KEY_TEMP_SCALE);
  }
  if(persist_exists(KEY_BT_LOGO_TYPE)){
      bt_image_type = persist_read_int(KEY_BT_LOGO_TYPE);
  }
  if(persist_exists(KEY_SHOW_ANIMATION)){
    show_animation = persist_read_bool(KEY_SHOW_ANIMATION);
  }  
  if(persist_exists(KEY_HIDE_SECONDS)){
    hide_second_hand = persist_read_bool(KEY_HIDE_SECONDS);
  }  
  /*if(persist_exists(KEY_DATE_FORMAT)){
    date_format = persist_read_int(KEY_DATE_FORMAT);
  }  */
  
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Register with Services
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  accel_tap_service_subscribe(tap_handler);  
  battery_state_service_subscribe(battery_handler);
  bluetooth_connection_service_subscribe(bt_handler);  
  
  
  if(show_animation){
    timer = app_timer_register(delta, (AppTimerCallback) timer_callback, NULL); 
  }else{
    clock_ready = true;
  }
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
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

