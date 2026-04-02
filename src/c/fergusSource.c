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
#define SETTINGS_KEY 1

static void prv_inbox_received_handler(DictionaryIterator* iter, void* context) {
    Tuple* vibrate_t = dict_find(iter, MESSAGE_KEY_VibrateOnDisconnect);

    if (vibrate_t)
    {
        bool new_val = vibrate_t->value->int32 == 1;

        if (new_val != s_vibrate_on_disconnect)
        {
            s_vibrate_on_disconnect = new_val;
            persist_write_bool(SETTINGS_KEY, s_vibrate_on_disconnect);
        }
    }
}

static void battery_update_proc(Layer* layer, GContext* ctx)
{
    GRect bounds = layer_get_bounds(layer);

    // Calculate width dynamically based on the layer's assigned bounds
    int width = (s_battery_level * bounds.size.w) / 100;

    // Draw the background
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Draw the bar
    graphics_context_set_fill_color(ctx, GColorBlack);
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
    // Get information about the Window
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Calculate dynamic centering variables
    int16_t w = bounds.size.w;
    
    // Offset centers the layout on taller screens.
    int16_t y_offset = (bounds.size.h - 168) / 2;

    // Create GFont
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_digital_60));
    s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_digital_24));

    // Create GBitmap & BitmapLayer
    s_cat_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CAT);
    
    // Cat Layer
    s_cat_layer = bitmap_layer_create(GRect(0, -5 + y_offset, w, 80));
    bitmap_layer_set_bitmap(s_cat_layer, s_cat_bitmap);
    bitmap_layer_set_alignment(s_cat_layer, GAlignCenter);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_cat_layer));

    // Date TextLayer
    s_date_layer = text_layer_create(GRect(0, 65 + y_offset, w, 30));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorBlack);
    text_layer_set_font(s_date_layer, s_date_font);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    // Time TextLayer
    s_time_layer = text_layer_create(GRect(0, 90 + y_offset, w, 70));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorBlack);
    text_layer_set_text(s_time_layer, "00:00");
    text_layer_set_font(s_time_layer, s_time_font);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

    // Battery Layer
    int bat_w = 114;
    int bat_x = (w - bat_w) / 2;
    s_battery_layer = layer_create(GRect(bat_x, 98 + y_offset, bat_w, 2));
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    layer_add_child(window_layer, s_battery_layer);
}

static void main_window_unload(Window* window)
{
    // Destroy elements
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    bitmap_layer_destroy(s_cat_layer);
    gbitmap_destroy(s_cat_bitmap);
    layer_destroy(s_battery_layer);

    // Unload GFonts
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

    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer, s_buffer);

    if (units_changed & DAY_UNIT)
    {
        // Write the current date into a buffer
        static char s_date_buffer[16];
        strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", tick_time);

        // Display the date
        text_layer_set_text(s_date_layer, s_date_buffer);
    }
}

static void tick_handler(struct tm* tick_time, TimeUnits units_changed)
{
    update_time(units_changed);
}

static void init()
{
    // Create main Window element and assign to pointer
    s_main_window = window_create();

    // Change the background color
    window_set_background_color(s_main_window, GColorWhite);

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

    // Battery level is displayed from the start
    battery_callback(battery_state_service_peek());

    // Register for Bluetooth connection updates
    connection_service_subscribe((ConnectionHandlers)
    {
        .pebble_app_connection_handler = bluetooth_callback
    });

    // Load the saved setting (default to true)
    s_vibrate_on_disconnect = persist_exists(SETTINGS_KEY) ? persist_read_bool(SETTINGS_KEY) : true;

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