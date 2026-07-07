// Thanks for downloading my watchface, this is the first thing i've made using the Pebble SDK.
// I hope you enjoy this little personal project, and your more than welcome to use and adapt this
// code for future projects you make. A special thanks to Rebble and Core Devices, none of this would have been possible
// without Rebble's incredibly important preservation work and documentation and Core Devices work on new hardware
// and introducing me to the Pebble platform.
// Enjoy this little cat on your wrist!!
// - Adam Edmondson :)


#include <pebble.h>

static Window* s_main_window;
static TextLayer* s_time_layer;
static TextLayer* s_date_layer;
static Layer* s_battery_layer;
static BitmapLayer* s_cat_layer;
static GBitmap* s_cat_bitmap;
static GFont s_time_font;
static GFont s_date_font;
static int s_battery_level;

static bool s_vibrate_on_disconnect = true;
static GColor s_text_colour;
static GColor s_background_colour;

#define SETTINGS_KEY_VIBRATE 1
#define SETTINGS_KEY_TEXT_COLOUR 2
#define SETTINGS_KEY_BACKGROUND_COLOUR 3

static void prv_inbox_received_handler(DictionaryIterator* iter, void* context)
{
    Tuple* vibrate_t = dict_find(iter, MESSAGE_KEY_VibrateOnDisconnect);
    if (vibrate_t)
    {
        bool new_vibe = vibrate_t->value->int32 == 1;

        if (new_vibe != s_vibrate_on_disconnect)
        {
            s_vibrate_on_disconnect = new_vibe;
            persist_write_bool(SETTINGS_KEY_VIBRATE, s_vibrate_on_disconnect);
        }
    }
	
	Tuple* text_colour_t = dict_find(iter, MESSAGE_KEY_TextColour);
    if (text_colour_t)
		{
        int32_t colour_val = text_colour_t->value->int32;
        GColor new_text_colour = GColorFromHEX(colour_val);
        
        if (!gcolor_equal(new_text_colour, s_text_colour))
				{
            s_text_colour = new_text_colour;
            persist_write_int(SETTINGS_KEY_TEXT_COLOUR, colour_val);
        }
    }
	
	Tuple* background_colour_t = dict_find(iter, MESSAGE_KEY_BackgroundColour);
    if (background_colour_t)
		{
        int32_t colour_val = background_colour_t->value->int32;
        GColor new_background_colour = GColorFromHEX(colour_val);
        
        if (!gcolor_equal(new_background_colour, s_background_colour))
				{
            s_background_colour = new_background_colour;
            persist_write_int(SETTINGS_KEY_BACKGROUND_COLOUR, colour_val);
        }
    }
	
	window_set_background_color(s_main_window, s_background_colour);
  text_layer_set_text_color(s_time_layer, s_text_colour);
  text_layer_set_text_color(s_date_layer, s_text_colour);
	layer_mark_dirty(s_battery_layer);
	
}

static void battery_update_proc(Layer* layer, GContext* ctx)
{
    GRect bounds = layer_get_bounds(layer);

    int width = (s_battery_level * bounds.size.w) / 100;

    // Background
    graphics_context_set_fill_color(ctx, s_background_colour);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Bar
    graphics_context_set_fill_color(ctx, s_text_colour);
    graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void battery_callback(BatteryChargeState state)
{
    if (s_battery_level != state.charge_percent)
    {
        // Record the new battery level
        s_battery_level = state.charge_percent;

        // Update meter
        layer_mark_dirty(s_battery_layer);
    }
}

static void bluetooth_callback(bool connected)
{
    if (!connected && s_vibrate_on_disconnect)
    {
        vibes_double_pulse();
    }
}

static void main_window_load(Window* window)
{
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    int16_t w = bounds.size.w;
    int16_t h = bounds.size.h; 

    // Create Font
    #if PBL_DISPLAY_HEIGHT >= 228
        s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_digital_72));
        s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_digital_36));
    #else
        s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_digital_60));
        s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_digital_24));
    #endif

    s_cat_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CAT);
    
    // Layout variables
    int cat_y, cat_h, date_y, date_h, time_y, time_h, bat_w, bat_x, bat_y;

    #if defined(PBL_ROUND)
        cat_y = (h * 5) / 100;
        cat_h = (h * 40) / 100; 
        
        date_y = (h * 40) / 100;
        date_h = (h * 15) / 100; 
        
        time_y = (h * 57) / 100;
        time_h = (h * 42) / 100; 
        
        bat_w = (w * 80) / 100;  
        bat_x = (w - bat_w) / 2;
        bat_y = (h * 58) / 100; 
    #else
        cat_y = -(h * 1) / 100;
        cat_h = (h * 50) / 100;
        
        date_y = (h * 43) / 100;
        date_h = (h * 18) / 100;
        
        time_y = (h * 58) / 100;
        time_h = (h * 42) / 100;
        
        bat_w = (w * 80) / 100;
        bat_x = (w - bat_w) / 2;
        bat_y = (h * 64) / 100;
    #endif

    // Cat Layer
    s_cat_layer = bitmap_layer_create(GRect(0, cat_y, w, cat_h));
		bitmap_layer_set_background_color(s_cat_layer, GColorClear);
    bitmap_layer_set_bitmap(s_cat_layer, s_cat_bitmap);
    bitmap_layer_set_alignment(s_cat_layer, GAlignCenter);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_cat_layer));

    // Date Layer
    s_date_layer = text_layer_create(GRect(0, date_y, w, date_h));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, s_text_colour);
    text_layer_set_font(s_date_layer, s_date_font);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    // Time Layer
    s_time_layer = text_layer_create(GRect(0, time_y, w, time_h));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, s_text_colour);
    text_layer_set_text(s_time_layer, "00:00");
    text_layer_set_font(s_time_layer, s_time_font);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

    // Battery Layer
    s_battery_layer = layer_create(GRect(bat_x, bat_y, bat_w, 2));
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    layer_add_child(window_layer, s_battery_layer);
}

static void main_window_unload(Window* window)
{
    // Destroy
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    bitmap_layer_destroy(s_cat_layer);
    gbitmap_destroy(s_cat_bitmap);
    layer_destroy(s_battery_layer);

    // Unload Fonts
    fonts_unload_custom_font(s_time_font);
    fonts_unload_custom_font(s_date_font);
}

static void update_time(TimeUnits units_changed)
{
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm* tick_time = localtime(&temp);

    // Write the current hours and minutes into a buffer
    static char s_buffer[8];
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

    // Display time
    text_layer_set_text(s_time_layer, s_buffer);

    if (units_changed & DAY_UNIT)
    {
        // Write the current date into a buffer
        static char s_date_buffer[16];
        strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", tick_time);

        // Display Date
        text_layer_set_text(s_date_layer, s_date_buffer);
    }
}

static void tick_handler(struct tm* tick_time, TimeUnits units_changed)
{
    update_time(units_changed);
}

static void init()
{
		s_vibrate_on_disconnect = persist_exists(SETTINGS_KEY_VIBRATE) ? persist_read_bool(SETTINGS_KEY_VIBRATE) : true;
		s_text_colour = persist_exists(SETTINGS_KEY_TEXT_COLOUR) ? GColorFromHEX(persist_read_int(SETTINGS_KEY_TEXT_COLOUR)) : GColorBlack;
    s_background_colour = persist_exists(SETTINGS_KEY_BACKGROUND_COLOUR) ? GColorFromHEX(persist_read_int(SETTINGS_KEY_BACKGROUND_COLOUR)) : GColorWhite;
	
    s_main_window = window_create();
    window_set_background_color(s_main_window, s_background_colour);

    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers)
    {
        .load = main_window_load,
        .unload = main_window_unload
    });

    window_stack_push(s_main_window, true);

    // Register with services
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    battery_state_service_subscribe(battery_callback);

    // Displayed from the start
    battery_callback(battery_state_service_peek());

    // Register for Bluetooth updates
    connection_service_subscribe((ConnectionHandlers)
    {
        .pebble_app_connection_handler = bluetooth_callback
    });

    // Load the saved setting (default to true)
    s_vibrate_on_disconnect = persist_exists(SETTINGS_KEY_VIBRATE) ? persist_read_bool(SETTINGS_KEY_VIBRATE) : true;

    // Register the handler for the phone
    app_message_register_inbox_received(prv_inbox_received_handler);
    app_message_open(128, 128);

    update_time(DAY_UNIT | HOUR_UNIT | MINUTE_UNIT);
}

static void deinit() 
{
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    connection_service_unsubscribe();
    app_message_deregister_callbacks();

    // Destroy Window
    window_destroy(s_main_window);
}

int main(void) 
{
    init();
    app_event_loop();
    deinit();
}