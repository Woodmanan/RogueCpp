#pragma once
#include "Data/Resources.h"
#define BEARLIB_TERMINAL

#ifdef BEARLIB_TERMINAL
#include "Include/C/BearLibTerminal.h"

enum EKey
{
	A = TK_A,
	B = TK_B,
	C = TK_C,
	D = TK_D,
	E = TK_E,
	F = TK_F,
	G = TK_G,
	H = TK_H,
	I = TK_I,
	J = TK_J,
	K = TK_K,
	L = TK_L,
	M = TK_M,
	N = TK_N,
	O = TK_O,
	P = TK_P,
	Q = TK_Q,
	R = TK_R,
	S = TK_S,
	T = TK_T,
	U = TK_U,
	V = TK_V,
	W = TK_W,
	X = TK_X,
	Y = TK_Y,
	Z = TK_Z,
	UP = TK_UP,
	DOWN = TK_DOWN,
	RIGHT = TK_RIGHT,
	LEFT = TK_LEFT,
	LEFT_SHIFT = TK_SHIFT,
	RIGHT_SHIFT = TK_SHIFT,
	LEFT_CONTROL = TK_CONTROL,
	SPACE = TK_SPACE,
	TAB = TK_TAB,
	EQUAL = TK_EQUALS,
	MINUS = TK_MINUS,
	ENTER = TK_ENTER,
	PAGE_UP = TK_PAGEUP,
	PAGE_DOWN = TK_PAGEDOWN,
	NUM_0 = TK_0,
	NUM_1 = TK_1,
	NUM_2 = TK_2,
	NUM_3 = TK_3,
	NUM_4 = TK_4,
	NUM_5 = TK_5,
	NUM_6 = TK_6,
	NUM_7 = TK_7,
	NUM_8 = TK_8,
	NUM_9 = TK_9,
};

inline void terminal_color(Color color)
{
	color_t termColor = color_from_argb(color.a, color.r, color.g, color.b);
	terminal_color(termColor);
}

inline void terminal_bkcolor(Color color)
{
	color_t termColor = color_from_argb(color.a, color.r, color.g, color.b);
	terminal_bkcolor(termColor);
}

inline int terminal_get_key(EKey key)
{
	return terminal_state(key);
}

inline void terminal_update()
{

}

inline bool terminal_should_close()
{
	return terminal_check(TK_CLOSE);
}

inline int terminal_width()
{
	return terminal_state(TK_WIDTH);
}

inline int terminal_height()
{
	return terminal_state(TK_HEIGHT);
}

inline void terminal_set_font(string name)
{
	TResourcePointer<std::vector<unsigned char>> fontBuffer = ResourceManager::Get()->LoadSynchronous("Font", name);
	terminal_setf("font: %p:%d, size=8x16, codepage=437", fontBuffer->data(), fontBuffer->size());
}

inline void terminal_custom_init()
{
	std::cout << "Here!" << std::endl;
	if (!terminal_open())
	{
		std::cout << "Terminal failed to init!" << std::endl;
	}
	std::cout << "Here!" << std::endl;
	terminal_set("window: title='Unnamed Roguelike', size=80x25, resizable=true, fullscreen=false;");
}

inline void terminal_fullscreen()
{
	if (terminal_check(TK_FULLSCREEN))
	{
		//terminal_set("window.fullscreen = false");
	}
	else
	{
		//terminal_set("window.fullscreen = true");
	}
}

#else
#define VULKAN_RENDER
#include <Render/VulkanTerminal/VulkanTerminal.h>
#include <GLFW/glfw3.h>
#endif // BEARLIB_TERMINAL

#ifdef BEARLIB_TERMINAL

#else
enum EKey
{
	A = GLFW_KEY_A,
	B = GLFW_KEY_B,
	C = GLFW_KEY_C,
	D = GLFW_KEY_D,
	E = GLFW_KEY_E,
	F = GLFW_KEY_F,
	G = GLFW_KEY_G,
	H = GLFW_KEY_H,
	I = GLFW_KEY_I,
	J = GLFW_KEY_J,
	K = GLFW_KEY_K,
	L = GLFW_KEY_L,
	M = GLFW_KEY_M,
	N = GLFW_KEY_N,
	O = GLFW_KEY_O,
	P = GLFW_KEY_P,
	Q = GLFW_KEY_Q,
	R = GLFW_KEY_R,
	S = GLFW_KEY_S,
	T = GLFW_KEY_T,
	U = GLFW_KEY_U,
	V = GLFW_KEY_V,
	W = GLFW_KEY_W,
	X = GLFW_KEY_X,
	Y = GLFW_KEY_Y,
	Z = GLFW_KEY_Z,
	UP = GLFW_KEY_UP,
	DOWN = GLFW_KEY_DOWN,
	RIGHT = GLFW_KEY_RIGHT,
	LEFT = GLFW_KEY_LEFT,
	LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
	RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT,
	LEFT_CONTROL = GLFW_KEY_LEFT_CONTROL,
	SPACE = GLFW_KEY_SPACE,
	EQUAL = GLFW_KEY_EQUAL,
	MINUS = GLFW_KEY_MINUS,
	ENTER = GLFW_KEY_ENTER,
	PAGE_UP = GLFW_KEY_PAGE_UP,
	PAGE_DOWN = GLFW_KEY_PAGE_DOWN,
	NUM_0 = GLFW_KEY_0,
	NUM_1 = GLFW_KEY_1,
	NUM_2 = GLFW_KEY_2,
	NUM_3 = GLFW_KEY_3,
	NUM_4 = GLFW_KEY_4,
	NUM_5 = GLFW_KEY_5,
	NUM_6 = GLFW_KEY_6,
	NUM_7 = GLFW_KEY_7,
	NUM_8 = GLFW_KEY_8,
	NUM_9 = GLFW_KEY_9,
};

inline void terminal_update()
{
	glfwPollEvents();
}
#endif