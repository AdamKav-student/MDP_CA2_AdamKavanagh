// Adam Kavanagh - D00247069
#include "hud_panel.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <algorithm>
#include <cmath>

namespace
{
	// Points used to round each corner. Eight is plenty at HUD sizes and keeps
	// the whole plate to a 32-point convex shape.
	constexpr unsigned int kPointsPerCorner = 8;

	const sf::Color kBodyColour(45, 50, 60, 235);
	const sf::Color kEdgeColour(255, 255, 255, 28);
	const sf::Color kShadowColour(0, 0, 0, 70);
	constexpr float kShadowOffset = 4.f;
}

HudPanel::HudPanel(float corner_radius, float padding)
	: m_corner_radius(corner_radius)
	, m_padding(padding)
	, m_visible(false)
{
	m_body.setFillColor(kBodyColour);
	m_body.setOutlineColor(kEdgeColour);
	m_body.setOutlineThickness(1.f);

	m_shadow.setFillColor(kShadowColour);
}

void HudPanel::BuildRoundedRect(sf::ConvexShape& shape, sf::Vector2f size, float radius)
{
	// Never let the corners meet in the middle of a short panel.
	radius = std::min(radius, std::min(size.x, size.y) * 0.5f);

	// Corner centres in the order the outline is traced: bottom-right,
	// bottom-left, top-left, top-right. Each contributes a quarter turn, so
	// the points come out in a consistent winding and the shape stays convex.
	const sf::Vector2f centres[4] =
	{
		{ size.x - radius, size.y - radius },
		{ radius,          size.y - radius },
		{ radius,          radius          },
		{ size.x - radius, radius          },
	};

	shape.setPointCount(kPointsPerCorner * 4);

	std::size_t index = 0;
	for (int corner = 0; corner < 4; ++corner)
	{
		for (unsigned int i = 0; i < kPointsPerCorner; ++i)
		{
			const float t = static_cast<float>(i) / static_cast<float>(kPointsPerCorner - 1);
			const float angle = (static_cast<float>(corner) + t) * 1.5707963f;	// quarter turn
			shape.setPoint(index++, sf::Vector2f(
				centres[corner].x + std::cos(angle) * radius,
				centres[corner].y + std::sin(angle) * radius));
		}
	}
}

void HudPanel::Rebuild(sf::Vector2f position, sf::Vector2f size)
{
	BuildRoundedRect(m_body, size, m_corner_radius);
	m_body.setPosition(position);

	// The shadow is the same plate, nudged down and out a little.
	BuildRoundedRect(m_shadow, size + sf::Vector2f(2.f, 2.f), m_corner_radius);
	m_shadow.setPosition(position + sf::Vector2f(-1.f, kShadowOffset));
}

void HudPanel::FitTo(const sf::Text& text)
{
	const sf::FloatRect bounds = text.getGlobalBounds();

	// An empty string has no bounds worth drawing a plate around.
	m_visible = bounds.size.x > 0.f && bounds.size.y > 0.f;
	if (!m_visible)
	{
		return;
	}

	Rebuild(sf::Vector2f(bounds.position.x - m_padding, bounds.position.y - m_padding),
		sf::Vector2f(bounds.size.x + m_padding * 2.f, bounds.size.y + m_padding * 2.f));
}

void HudPanel::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (!m_visible)
	{
		return;
	}

	target.draw(m_shadow, states);
	target.draw(m_body, states);
}
