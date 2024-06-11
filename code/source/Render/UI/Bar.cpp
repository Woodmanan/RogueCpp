#include "Bar.h"
#include "../RenderTools.h"

const wchar_t fillChars[8] = { L'\u258F', L'\u258E', L'\u258D', L'\u258C', L'\u258B', L'\u258A', L'\u2589', L'\u2588' };

void Bar::OnSizeUpdated()
{

}

void Bar::OnDrawContent(float deltaTime)
{
	m_currentFill = Lerp(m_currentFill, m_goalFill, m_lerpAmount);
	m_currentFill = Clamp(m_currentFill);

	MatchLayer();
	Clear();

	//0-7, 8-15, 16-23

	int step = ((m_rect.w * 8 - 1) * m_currentFill);
	int boundary = step / 8;

	for (int x = 0; x < boundary; x++)
	{
		for (int y = 0; y < m_rect.h; y++)
		{
			Put(x, y, fillChars[7], m_color, Color(0, 0, 0, 0));
		}
	}

	step = step % 8;
	if (step != 0)
	{
		for (int y = 0; y < m_rect.h; y++)
		{
			Put(boundary, y, fillChars[step], m_color, Color(0, 0, 0, 0));
		}
	}
}

void Bar::SetFill(float fill, float lerpSpeed)
{
	m_goalFill = fill;
	m_lerpAmount = lerpSpeed;
}