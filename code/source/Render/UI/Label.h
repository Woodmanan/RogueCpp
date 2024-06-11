#pragma once
#include "../Windows/Window.h"

class Label : public Window
{
public:
	Label(string id, Anchors anchors, Window* parent = nullptr) : Window(id, anchors, parent) {}

	virtual void OnSizeUpdated() override;
	virtual void OnDrawContent(float deltaTime) override;

	void SetString(std::string value) { m_value = value; }
	void SetAlignment(int alignment) { m_alignment = alignment; }

private:
	std::string m_value;
	int m_alignment;
};