#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "num2words.h"

#define MY_UUID { 0xA4, 0x6F, 0x74, 0x2B, 0xAE, 0xC7, 0x41, 0x9A, 0x8C, 0xD5, 0x65, 0xA8, 0xC5, 0x10, 0x1E, 0x1C }
PBL_APP_INFO(MY_UUID,
             "Decepticon", "orviwan",
             2, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define BUFFER_SIZE 43
#define TOTAL_FRAMES 15
#define COOKIE_MY_TIMER 1
#define TIME_SLOT_ANIMATION_DURATION 700

Window window;
BmpContainer bmp_cont;
AppTimerHandle timer_handle;
TextLayer text_date_layer;

typedef struct CommonWordsData {
  TextLayer label;
  char buffer[BUFFER_SIZE];
} CommonWordsData;

static CommonWordsData s_data;
static CommonWordsData s_data_minutes;
static CommonWordsData s_data_sminutes;
static CommonWordsData date;


int current_frame;
int direction;
int loop;

const int IMAGE_ANIM_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_ANIM_1,
  RESOURCE_ID_IMAGE_ANIM_2,
  RESOURCE_ID_IMAGE_ANIM_3,
  RESOURCE_ID_IMAGE_ANIM_4,
  RESOURCE_ID_IMAGE_ANIM_5,
  RESOURCE_ID_IMAGE_ANIM_6,
  RESOURCE_ID_IMAGE_ANIM_7,
  RESOURCE_ID_IMAGE_ANIM_8,
  RESOURCE_ID_IMAGE_ANIM_9,
  RESOURCE_ID_IMAGE_ANIM_10,
  RESOURCE_ID_IMAGE_ANIM_11,
  RESOURCE_ID_IMAGE_ANIM_12,
  RESOURCE_ID_IMAGE_ANIM_13,
  RESOURCE_ID_IMAGE_ANIM_14,
  RESOURCE_ID_IMAGE_ANIM_15
};

void slide_out(PropertyAnimation *animation, CommonWordsData *layer) {
  GRect from_frame = layer_get_frame(&layer->label.layer);

  GRect to_frame = GRect(-window.layer.frame.size.w, from_frame.origin.y,
                          window.layer.frame.size.w, from_frame.size.h);

  property_animation_init_layer_frame(animation, &layer->label.layer, NULL, &to_frame);
  animation_set_duration(&animation->animation, TIME_SLOT_ANIMATION_DURATION);
  animation_set_curve(&animation->animation, AnimationCurveEaseIn);
}

void slide_in(PropertyAnimation *animation, CommonWordsData *layer) {
  GRect to_frame = layer_get_frame(&layer->label.layer);
  GRect from_frame = GRect(2*window.layer.frame.size.w, to_frame.origin.y,
                          window.layer.frame.size.w, to_frame.size.h);

  layer_set_frame(&layer->label.layer, from_frame);
  text_layer_set_text(&layer->label, layer->buffer);
  property_animation_init_layer_frame(animation, &layer->label.layer, NULL, &to_frame);
  animation_set_duration(&animation->animation, TIME_SLOT_ANIMATION_DURATION);
  animation_set_curve(&animation->animation, AnimationCurveEaseOut);
}

void slide_out_animation_stopped(Animation *slide_out_animation, bool finished, void *context) {
  CommonWordsData *layer = (CommonWordsData *)context;
  layer->label.layer.frame.origin.x = 0;
  static PropertyAnimation animation;
  slide_in(&animation, layer);
  animation_schedule(&animation.animation);
}


void set_container_image(BmpContainer *bmp_container, const int resource_id, GPoint origin, Layer *targetLayer) {
  layer_remove_from_parent(&bmp_container->layer.layer);
  bmp_deinit_container(bmp_container);
  bmp_init_container(resource_id, bmp_container);
  GRect frame = layer_get_frame(&bmp_container->layer.layer);
  frame.origin.x = origin.x;
  frame.origin.y = origin.y;
  layer_set_frame(&bmp_container->layer.layer, frame);
  layer_add_child(targetLayer, &bmp_container->layer.layer);
}

static void handle_minute_tick(AppContextRef app_ctx, PebbleTickEvent* e) {
  PblTm *t = e->tick_time;
  if((e->units_changed & MINUTE_UNIT) == MINUTE_UNIT) {

    if(current_frame==TOTAL_FRAMES) {
      direction = 1;
    }
    if(current_frame==0) {
      direction = 0;
    }
    loop = 0;
    timer_handle = app_timer_send_event(app_ctx, 75 /* milliseconds */, COOKIE_MY_TIMER);

    fuzzy_full_minutes_to_words(t->tm_hour, t->tm_min, s_data_sminutes.buffer, BUFFER_SIZE);
    static PropertyAnimation animation2;
    slide_out(&animation2, &s_data_sminutes);
    animation_set_handlers(&animation2.animation, (AnimationHandlers){
      .stopped = (AnimationStoppedHandler)slide_out_animation_stopped
    }, (void *) &s_data_sminutes);
    animation_schedule(&animation2.animation);
  }
  if ((e->units_changed & HOUR_UNIT) == HOUR_UNIT) {
    fuzzy_hours_to_words(t->tm_hour, t->tm_min, s_data.buffer, BUFFER_SIZE);
    static PropertyAnimation animation3;
    slide_out(&animation3, &s_data);
    animation_set_handlers(&animation3.animation, (AnimationHandlers){
      .stopped = (AnimationStoppedHandler)slide_out_animation_stopped
    }, (void *) &s_data);
    animation_schedule(&animation3.animation);
  }
  
  unsigned short display_second = e->tick_time->tm_sec;
  if(display_second==0)
  {
    //animation_unschedule_all();

  }
}

void transform() {
  if(direction==1) {
    current_frame--;
  }
  else {
    current_frame++;
  }
  GPoint point;
  if(current_frame > 8) {
    point = GPoint(18, 8);
  }
  else {
    point = GPoint(17, 1);
  }
  loop++;
  set_container_image(&bmp_cont, IMAGE_ANIM_RESOURCE_IDS[current_frame], point, &window.layer);
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
    (void)ctx;
    (void)handle;
    
    if (cookie == COOKIE_MY_TIMER) {
        transform();
        if(loop<TOTAL_FRAMES) {
          timer_handle = app_timer_send_event(ctx, 75, COOKIE_MY_TIMER);
        }      
    }
}
  
void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Decepticon");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  current_frame = 0;
  direction = 0;
  loop = 0;

  resource_init_current_app(&APP_RESOURCES);
  set_container_image(&bmp_cont, IMAGE_ANIM_RESOURCE_IDS[current_frame], GPoint(17, 1), &window.layer);

//hours 
  text_layer_init(&s_data.label, GRect(0, 118, window.layer.frame.size.w, 40));
  text_layer_set_background_color(&s_data.label, GColorClear);
  text_layer_set_text_color(&s_data.label, GColorWhite);
  text_layer_set_font(&s_data.label, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TRANSFORMERS_25)));
  text_layer_set_text_alignment(&s_data.label, GTextAlignmentCenter);
  layer_add_child(&window.layer, &s_data.label.layer);

// single digits
  text_layer_init(&s_data_sminutes.label,GRect(0, 144, 144, 40));
  text_layer_set_background_color(&s_data_sminutes.label, GColorClear);
  text_layer_set_text_color(&s_data_sminutes.label, GColorWhite);
  text_layer_set_font(&s_data_sminutes.label, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TRANSFORMERS_18)));
  text_layer_set_text_alignment(&s_data_sminutes.label, GTextAlignmentCenter);
  layer_add_child(&window.layer, &s_data_sminutes.label.layer);

//show your face
  PblTm t;
  get_time(&t);

  fuzzy_full_minutes_to_words(t.tm_hour, t.tm_min, s_data_sminutes.buffer, BUFFER_SIZE);
  static PropertyAnimation animation1;
  slide_in(&animation1, &s_data_sminutes);
  animation_schedule(&animation1.animation);

  fuzzy_hours_to_words(t.tm_hour, t.tm_min, s_data.buffer, BUFFER_SIZE);
  static PropertyAnimation animation2;
  slide_in(&animation2, &s_data);
  animation_schedule(&animation2.animation);

}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  bmp_deinit_container(&bmp_cont);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .timer_handler = &handle_timer,
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
