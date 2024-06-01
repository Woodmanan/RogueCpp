#include "UIManager.h"

UIManager::UIManager()
{
    LoadSettings();
}

UIManager::~UIManager()
{

}

void UIManager::LoadSettings()
{
    if (RogueSaveManager::FileExists("UISettings.rsf"))
    {
        RogueSaveManager::OpenReadSaveFile("UISettings.rsf");
        RogueSaveManager::Read("Settings", m_settings);
        RogueSaveManager::CloseReadSaveFile();
    }
}

void UIManager::SaveSettings()
{
    RogueSaveManager::OpenWriteSaveFile("UISettings.rsf");
    RogueSaveManager::Write("Settings", m_settings);
    RogueSaveManager::CloseWriteSaveFile();
}

bool UIManager::ApplySettingsToWindow(Window* window)
{
    ASSERT(window != nullptr);
    for (const WindowSettings& setting : m_settings)
    {
        if (setting.id == window->m_id)
        {
            window->m_anchors = setting.anchors;
            Window* parentWindow = GetWindowById(setting.parentId);
            if (parentWindow != nullptr)
            {
                window->SetParent(parentWindow);
            }
            return true;
        }
    }

    return false;
}

void UIManager::ApplySettingsToAllWindows()
{
    for (int index = 0; index < (int)m_windows.size(); index++)
    {
        ApplySettingsToWindow(m_windows[index]);
    }
}

Window* UIManager::GetWindowById(const string id)
{
    for (Window* window : m_windows)
    {
        ASSERT(window != nullptr);
        if (window->m_id == id)
        {
            return window;
        }
    }

    return nullptr;
}

bool UIManager::DeleteWindow(Window* window)
{
    for (auto it = m_windows.begin(); it != m_windows.end(); it++)
    {
        if (*it == window)
        {
            m_windows.erase(it);
            return true;
        }
    }

    return false;
}

void UIManager::CreateSettingsForAllWindows()
{
    m_settings.clear();
    for (auto it = m_windows.begin(); it != m_windows.end(); it++)
    {
        CreateSettingsForWindow(*it);
    }
}

void UIManager::CreateSettingsForWindow(Window* window)
{
    m_settings.emplace_back(WindowSettings({ window->m_id, window->m_anchors, (window->m_parent == nullptr) ? std::string("") : window->m_parent->m_id }));
}

namespace RogueSaveManager
{
    void Serialize(WindowSettings& value)
    {
        AddOffset();
        Write("ID", value.id);
        Write("Anchors", value.anchors);
        Write("Parent", value.parentId);
        RemoveOffset();
    }

    void Deserialize(WindowSettings& value)
    {
        AddOffset();
        Read("ID", value.id);
        Read("Anchors", value.anchors);
        Read("Parent", value.parentId);
        RemoveOffset();
    }
}