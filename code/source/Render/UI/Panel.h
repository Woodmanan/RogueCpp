#pragma once
#include "../Windows/Window.h"

class Panel : public Window
{
public:
	Panel(string id, Anchors anchors, Window* parent = nullptr) : Window(id, anchors, parent) {}

	virtual void OnSizeUpdated() override;
	virtual void OnDrawContent(float deltaTime) override;
};