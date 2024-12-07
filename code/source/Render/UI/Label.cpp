#include "Label.h"
#include "../RenderTools.h"
#include "Render/Terminal.h"

void Label::OnSizeUpdated()
{

}

void Label::OnDrawContent(float deltaTime)
{
	MatchLayer();
	Clear();

	terminal_color(Color(255, 255, 255));

	terminal_print_ext(m_rect.x, m_rect.y, m_rect.w, m_rect.h, m_alignment, m_value.c_str());
}