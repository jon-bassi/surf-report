#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *date_layer;
static Layer *battery_layer;
static int battery_level;

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void update_time()
{
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  static char time_buffer[9];
  strftime(time_buffer, sizeof(time_buffer), "%I:%M %p", tick_time);
  text_layer_set_text(text_layer, time_buffer);

  static char day_buffer[12];
  strftime(day_buffer, sizeof(day_buffer), "%a %d, %b", tick_time);
  text_layer_set_text(date_layer, day_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
  update_time();
}

static void battery_callback(BatteryChargeState state)
{
  battery_level = state.charge_percent;
  layer_mark_dirty(battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx)
{
  GRect bounds = layer_get_bounds(layer);

  // find width of bar
  int width = (int)(float)(((float)battery_level / 100.0F) * 114.0F);

  //draw the background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // draw the bar
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // create text layer with specific bounds
  text_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(70,116), bounds.size.w, 40));
  date_layer = text_layer_create(GRect(bounds.size.w / 2 - 14, PBL_IF_ROUND_ELSE(50,144), bounds.size.w / 2, 30));


  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_color(text_layer, GColorBlack);

  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorBlack);

  //text_layer_set_text(text_layer, "testing");
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM));
  text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));

  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  layer_add_child(window_layer, text_layer_get_layer(date_layer));

  /*
   *  battey things
   */
   battery_layer = layer_create(GRect(14, 114, 115, 2));
   layer_set_update_proc(battery_layer, battery_update_proc);
   layer_add_child(window_layer, battery_layer);

}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
  text_layer_destroy(date_layer);
  layer_destroy(battery_layer);

}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  battery_state_service_subscribe(battery_callback);
  tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, tick_handler);

  update_time();

  battery_callback(battery_state_service_peek());

}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
