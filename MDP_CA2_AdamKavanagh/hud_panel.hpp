// Adam Kavanagh - D00247069
#pragma once
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Text.hpp>

// A dark rounded plate drawn behind HUD text. The battlefield art is a mix of
// pale sand and dark hedgerow, so plain text over it is legible in some places
// and invisible in others; a panel gives every readout the same background
// whatever it happens to be sitting on.
//
// The shape is generated rather than loaded from an image so it fits itself to
// whatever the text currently says, at any window size, without a nine-slice
// or a fixed-size asset.
class HudPanel : public sf::Drawable
{
public:
	explicit HudPanel(float corner_radius = 14.f, float padding = 14.f);

	// Size and position the plate around a piece of text. Call again whenever
	// the string changes, since the bounds change with it.
	void FitTo(const sf::Text& text);

private:
	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	void Rebuild(sf::Vector2f position, sf::Vector2f size);
	static void BuildRoundedRect(sf::ConvexShape& shape, sf::Vector2f size, float radius);

private:
	sf::ConvexShape	m_shadow;
	sf::ConvexShape	m_body;
	float			m_corner_radius;
	float			m_padding;
	bool			m_visible;
};
