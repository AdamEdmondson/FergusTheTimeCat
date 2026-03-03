// Thanks for downloading my watchface, this is the first thing iv'e made using the Pebble SDK.
// I hope you enjoy this little personal project, and your more than welcome to use and adapt this
// code for future projects you make. A special thanks to Rebble, none of this would have been possible
// without their incredibly important preservation work and documentation.
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
	if (vibrate_t) {
		s_vibrate_on_disconnect = vibrate_t->value->int32 == 1;

		persist_write_bool(SETTINGS_KEY, s_vibrate_on_disconnect);
	}
}

static void battery_update_proc(Layer* layer, GContext* ctx)
{
	GRect bounds = layer_get_bounds(layer);

	// Find the width of the bar (total width = 114px)
	int width = (s_battery_level * 114) / 100;

	// Draw the background
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);

	// Draw the bar
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void battery_callback(BatteryChargeState state)
{
	// Record the new battery level
	s_battery_level = state.charge_percent;

	// Update meter
	layer_mark_dirty(s_battery_layer);
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

	// Create GFont
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_digital_60));
	s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_digital_24));

	// Create GBitmap & BitmapLayer to display the GBitmap
	s_cat_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CAT);
	s_cat_layer = bitmap_layer_create(GRect(0, -5, bounds.size.w, 80));

	// Set the bitmap onto the layer and add to the window
	bitmap_layer_set_bitmap(s_cat_layer, s_cat_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_cat_layer));

	// Create the TextLayer with specific bounds
	s_time_layer = text_layer_create(GRect(2, 90, bounds.size.w, 70));

	// Watchface formatting
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorBlack);
	text_layer_set_text(s_time_layer, "00:00");

	// Apply custom font to TextLayer & align
	text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

	// Create the date TextLayer
	s_date_layer = text_layer_create(GRect(2, 65, bounds.size.w, 30));

	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_color(s_date_layer, GColorBlack);
	text_layer_set_font(s_date_layer, s_date_font);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

	// Create battery meter Layer
	s_battery_layer = layer_create(GRect(14, 98, 114, 2));
	layer_set_update_proc(s_battery_layer, battery_update_proc);

	// Add to Window
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	layer_add_child(window_get_root_layer(window), s_battery_layer);
	
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

static void update_time() {
	// Get a tm structure
	time_t temp = time(NULL);
	struct tm* tick_time = localtime(&temp);

	// Write the current hours and minutes into a buffer
	static char s_buffer[8];
	strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

	// Display this time on the TextLayer
	text_layer_set_text(s_time_layer, s_buffer);

	// Write the current date into a buffer
	static char s_date_buffer[16];
	strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %d", tick_time);

	// Display the date
	text_layer_set_text(s_date_layer, s_date_buffer);
}

static void tick_handler(struct tm* tick_time, TimeUnits units_changed)
{
	update_time();
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

	// Show the Window on the watch, with animated=true
	window_stack_push(s_main_window, true);

	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	// Register for battery level updates
	battery_state_service_subscribe(battery_callback);

	// Ensure battery level is displayed from the start
	battery_callback(battery_state_service_peek());

	// Register for Bluetooth connection updates
	connection_service_subscribe((ConnectionHandlers)
	{
		.pebble_app_connection_handler = bluetooth_callback
	});

	// Show the correct state of the BT connection from the start
	bluetooth_callback(connection_service_peek_pebble_app_connection());

	// 1. Load the saved setting (default to true)
	s_vibrate_on_disconnect = persist_exists(SETTINGS_KEY) ? persist_read_bool(SETTINGS_KEY) : true;

	// 2. Register the handler to "listen" for the phone
	app_message_register_inbox_received(prv_inbox_received_handler);
	app_message_open(128, 128);

	update_time();
}

static void deinit() 
{
	// Destroy Window
	window_destroy(s_main_window);
}

int main(void) 
{
	init();
	app_event_loop();
	deinit();
}