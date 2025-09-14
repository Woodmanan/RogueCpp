#include "VulkanTerminal.h"
#ifdef VULKAN_RENDER
int `erminal_open()
{
	terminal = new VulkanTerminal();
	keys = new std::vector<int>();
	terminal->Initialize();

	glfwSetKeyCallback(terminal->GetWindow(), key_callback);

	return 1;
}

void terminal_close()
{
	terminal->Shutdown();
	delete keys;
	delete terminal;
}

void terminal_refresh()
{
	terminal->RenderFrame();
}

void terminal_clear()
{
	terminal->Clear();
}

void terminal_color(Color color)
{
	terminal->SetFGColor(color);
}

void terminal_bkcolor(Color color)
{
	terminal->SetBGColor(color);
}

void terminal_layer(int index)
{
	terminal->SetDepth(index);
}

bool terminal_should_close()
{
	return glfwWindowShouldClose(terminal->GetWindow());
}

void terminal_put(int x, int y, int code)
{
	terminal->SetCharacter(x, y, code);
}

void terminal_clear_area(int x, int y, int w, int h)
{
	terminal->ClearArea(x, y, w, h);
}

int terminal_read_str(int x, int y, char* buffer, int max)
{
	return 0;
}

int terminal_width()
{
	return VulkanTerminal::TERMINAL_WIDTH;
}

int terminal_height()
{
	return VulkanTerminal::TERMINAL_HEIGHT;
}

void terminal_print(int x, int y, const char* s)
{
	int strlen = strnlen(s, VulkanTerminal::TERMINAL_WIDTH - x);

	for (int i = 0; i < strlen; i++)
	{
		terminal_put(x + i, y, s[i]);
	}
}

void terminal_print_ext(int x, int y, int width, int height, int align, const char* s)
{
	int strlen = strnlen(s, width * height);

	int c = 0;
	for (int i = x; i < (x + width); i++)
	{
		for (int j = y; j < (y + height); j++)
		{
			terminal_put(i, j, s[c]);
			c++;
		}
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		keys->push_back(key);
	}
}

int terminal_has_input()
{
	return keys->size() > 0;
}

int terminal_read()
{
	while (keys->size() == 0)
	{
		glfwWaitEvents();
	}
	
	int value = keys->at(0);
	keys->erase(keys->begin());
	return value;
}

int terminal_peek()
{
	if (terminal_has_input()) { return keys->at(0); }

	return 0;
}

bool terminal_get(int keycode)
{
	return glfwGetKey(terminal->GetWindow(), keycode) == GLFW_PRESS;
}
#endif