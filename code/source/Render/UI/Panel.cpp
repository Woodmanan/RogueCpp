#include "Panel.h"
#include "../RenderTools.h"

void Panel::OnSizeUpdated()
{

}

void Panel::OnDrawContent(float deltaTime)
{
	MatchLayer();
	Clear();
	DrawRect({ 0,0,m_rect.w, m_rect.h }, Color(255, 0, 0));
	RenderTools::DrawRect(m_rect);
}