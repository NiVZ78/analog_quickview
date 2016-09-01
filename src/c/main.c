#include <pebble.h>

// pointer to main window
static Window *s_main_window;

// pointer to main window layer
static Layer *s_main_window_layer;

// pointer to dial layer
static Layer *s_dial_layer;

// co-ordinate variable which will hold the value of the center of the screen (so content will be centered on all Pebble watches)
static GPoint s_main_window_center;

// Array of 12 GPoints to hold the dial locations
static GPoint dial_point[12];

// Hand length
static int hand_length;

// Convenience function to calcuate a point co-ordinate given a center point, angle and length
static GPoint calc_point(GPoint center, int32_t angle, int length){
  
  //int32_t angle = TRIG_MAX_ANGLE * ratioAngle;
  
  return (GPoint) {
    .x = (int16_t)(sin_lookup(angle) * (int32_t)length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(angle) * (int32_t)length / TRIG_MAX_RATIO) + center.y,
  };
  
}


// Convenience function to draw a line in the graphics context with a given color between two points at a specified width
static void draw_line(GContext *ctx, GColor color, GPoint p1, GPoint p2, int width){
  
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, width);
  graphics_draw_line(ctx, p1, p2);
  
}




int32_t abs32(int32_t a) {return (a^(a>>31)) - (a>>31);}     // returns absolute value of A (only works on 32bit signed)

GPoint getPointOnRect(GRect r, int angle) {
  int32_t sin = sin_lookup(angle), cos = cos_lookup(angle);  // Calculate once and store, to make quicker and cleaner
  int32_t dy = sin>0 ? (r.size.h/2) : (0-r.size.h)/2;        // Distance to top or bottom edge (from center)
  int32_t dx = cos>0 ? (r.size.w/2) : (0-r.size.w)/2;        // Distance to left or right edge (from center)
  if(abs32(dx*sin) < abs32(dy*cos)) {                        // if (distance to vertical line) < (distance to horizontal line)
    dy = (dx * sin) / cos;                                   // calculate distance to vertical line
  } else {                                                   // else: (distance to top or bottom edge) < (distance to left or right edge)
    dx = (dy * cos) / sin;                                   // move to top or bottom line
  }
  return GPoint(dx+r.origin.x+(r.size.w/2), dy+r.origin.y+(r.size.h/2));  // Return point on rectangle
}



static void update_points(){
  
  int border = 8;
  
  GRect dial_bounds = layer_get_unobstructed_bounds(s_dial_layer);
  s_main_window_center = grect_center_point(&dial_bounds);

  hand_length = dial_bounds.size.w > dial_bounds.size.h ? (dial_bounds.size.h/2)*80/100 : (dial_bounds.size.w/2)*80/100;
  
  // calculate the 12 dial number co-ordinates for RECT or ROUND
  for (int i=0; i<12; i++){
      #ifdef PBL_ROUND
        dial_point[i] = gpoint_from_polar(grect_inset(dial_bounds, GEdgeInsets(border)), GOvalScaleModeFillCircle, TRIG_MAX_ANGLE * i / 12);
      #else 
        dial_point[i] =  getPointOnRect(grect_inset(dial_bounds, GEdgeInsets(border)), (i-3)*TRIG_MAX_ANGLE/12);
      #endif
    }
  
} 

  




// function to redraw the watch dial
static void dial_update_proc(Layer *this_layer, GContext *ctx) {

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_pixel(ctx, s_main_window_center);
  
  graphics_context_set_text_color(ctx, GColorWhite);
  
  for (int i=0; i<12; i++){
    graphics_draw_pixel(ctx, dial_point[i]);
        
  }
  
  // Get the current time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  // minute hand
  int32_t minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  draw_line(ctx, GColorWhite, s_main_window_center, calc_point(s_main_window_center, minute_angle, hand_length), 3);
  
  // hour hand
  int32_t hour_angle = TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10)) / (12 * 6);
  draw_line(ctx, GColorWhite, s_main_window_center, calc_point(s_main_window_center, hour_angle, hand_length*2/3), 5);
  

  
}



static void main_window_load(Window *window) {
  
  // get the main window layer
  s_main_window_layer = window_get_root_layer(s_main_window);
    
  // Create the layer we will draw on
  s_dial_layer = layer_create(layer_get_unobstructed_bounds(s_main_window_layer));
  
  // Add the layer to our main window layer
  layer_add_child(s_main_window_layer, s_dial_layer);

  // Set the update procedure for our layer
  layer_set_update_proc(s_dial_layer, dial_update_proc);
  
  update_points();
}


static void main_window_unload(Window *window) {
    
}


void prv_unobstructed_change(AnimationProgress progress, void *context) {
  
  update_points();
  layer_mark_dirty(s_dial_layer);
  
}

static void init(void) {
  
  // Create the main window
  s_main_window = window_create();
  
  // set the background colour
  window_set_background_color(s_main_window, GColorBlack);
  
  // set the window load and unload handlers
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  

#if PBL_API_EXISTS(unobstructed_area_service_subscribe)
    UnobstructedAreaHandlers handlers = {
  .change = prv_unobstructed_change
};
unobstructed_area_service_subscribe(handlers, NULL);
#endif
  
  // show the window on screen
  window_stack_push(s_main_window, true);
  
}



static void deinit(void) {
  
  // Destroy the main window
  window_destroy(s_main_window);
  
}


int main(void) {
  
  init();
  app_event_loop();
  deinit();
  
}