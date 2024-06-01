#pragma once
#include <vector>
#include <type_traits>
#include "../Windows/Window.h"

struct WindowSettings
{
	string id;
	Anchors anchors;
	string parentId;
};

class UIManager
{
public:
	UIManager();
	~UIManager();

	void LoadSettings();
	void SaveSettings();

	bool ApplySettingsToWindow(Window* window);
	void ApplySettingsToAllWindows();

	Window* GetWindowById(const string id);

	template <typename T, class... Args>
	T* CreateWindow(Args&&... args)
	{
		static_assert(std::is_base_of<Window, T>::value, "Type passed to UI manager does not derive from Window.");
		T* newWindow = new T(std::forward<Args>(args)...);
		ApplySettingsToWindow(newWindow);
		m_windows.push_back(newWindow);
		return newWindow;
	}

	bool DeleteWindow(Window* window);

	void CreateSettingsForAllWindows();
	void CreateSettingsForWindow(Window* window);

private:
	vector<Window*> m_windows;
	vector<WindowSettings> m_settings;
};

namespace RogueSaveManager
{
	void Serialize(WindowSettings& value);
	void Deserialize(WindowSettings& value);
}