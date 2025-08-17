#include <Render/VulkanTerminal/VulkanTest.h>
#include <vector>

#ifdef VULKAN_RENDER

static VulkanTerminal* terminal = nullptr;
static std::vector<int>* keys = nullptr;

int terminal_open();
void terminal_close();
void terminal_refresh();
void terminal_clear();
void terminal_color(Color color);
void terminal_bkcolor(Color color);
void terminal_layer(int index);
bool terminal_should_close();

void terminal_put(int x, int y, int code);
void terminal_clear_area(int x, int y, int w, int h);
int terminal_read_str(int x, int y, char* buffer, int max);

int terminal_width();
int terminal_height();

void terminal_print(int x, int y, const char* s);
void terminal_print_ext(int x, int y, int width, int height, int align, const char* s);

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
int terminal_has_input();
int terminal_read();
int terminal_peek();

bool terminal_get_key(int keycode);

/*int terminal_set8(const int8_t* value);
int terminal_set16(const int16_t* value);
int terminal_set32(const int32_t* value);
void terminal_refresh();
void terminal_clear();
void terminal_clear_area(int x, int y, int w, int h);
void terminal_crop(int x, int y, int w, int h);
void terminal_composition(int mode);
void terminal_font8(const int8_t* name);
void terminal_font16(const int16_t* name);
void terminal_font32(const int32_t* name);
void terminal_put_ext(int x, int y, int dx, int dy, int code, color_t* corners);
int terminal_pick(int x, int y, int index);
color_t terminal_pick_color(int x, int y, int index);
color_t terminal_pick_bkcolor(int x, int y);
void terminal_print_ext8(int x, int y, int w, int h, int align, const int8_t* s, int* out_w, int* out_h);
void terminal_print_ext16(int x, int y, int w, int h, int align, const int16_t* s, int* out_w, int* out_h);
void terminal_print_ext32(int x, int y, int w, int h, int align, const int32_t* s, int* out_w, int* out_h);
void terminal_measure_ext8(int w, int h, const int8_t* s, int* out_w, int* out_h);
void terminal_measure_ext16(int w, int h, const int16_t* s, int* out_w, int* out_h);
void terminal_measure_ext32(int w, int h, const int32_t* s, int* out_w, int* out_h);

int terminal_state(int code);
int terminal_read_str8(int x, int y, int8_t* buffer, int max);
int terminal_read_str16(int x, int y, int16_t* buffer, int max);
int terminal_read_str32(int x, int y, int32_t* buffer, int max);
void terminal_delay(int period);
const int8_t* terminal_get8(const int8_t* key, const int8_t* default_);
const int16_t* terminal_get16(const int16_t* key, const int16_t* default_);
const int32_t* terminal_get32(const int32_t* key, const int32_t* default_);
color_t color_from_name8(const int8_t* name);
color_t color_from_name16(const int16_t* name);
color_t color_from_name32(const int32_t* name);
int terminal_put_array(int x, int y, int w, int h, const uint8_t* data, int row_stride, int column_stride, const void* layout, int char_size);*/
#endif