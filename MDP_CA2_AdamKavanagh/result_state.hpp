// Adam Kavanagh - D00247069
#pragma once
#include "state.hpp"
#include "hud_panel.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

// Full-screen victory or defeat card. It holds the screen for
// kResultScreenSeconds and then drops the player back to the main menu, which
// is what ends a training run once the lone enemy Panzer has been knocked out.
class ResultState : public State
{
public:
	ResultState(StateStack& stack, Context context, bool victory);

	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	sf::Sprite	m_background_sprite;
	sf::Text	m_countdown_text;
	HudPanel	m_countdown_panel;
	sf::Time	m_elapsed_time;
	bool		m_victory;
};
