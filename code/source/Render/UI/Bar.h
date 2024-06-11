#pragma once
#include "../Windows/Window.h"

class Bar : public Window
{
public:
	Bar(string id, Anchors anchors, Window* parent = nullptr) : Window(id, anchors, parent) {}

	virtual void OnSizeUpdated() override;
	virtual void OnDrawContent(float deltaTime) override;

	void SetFill(float fill, float lerpSpeed);
	void SetColor(Color color) { m_color = color; }

private:
	float m_currentFill = 0.0f;
	float m_goalFill = 0.0f;

	float m_lerpAmount = 1.0f;

	Color m_color;
};