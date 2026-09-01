// Adam Kavanagh - D00247069
#pragma once
#include "state.hpp"
#include "world.hpp"
#include "player.hpp"

// The offline training mission: one Sherman under the player's control and one
// stationary Panzer to knock out. Destroying it shows the victory card, which
// returns to the main menu on its own.
class GameState : public State
{
public:
	GameState(StateStack& stack, Context context);

	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	void DrawObjectiveText();

private:
	World		m_world;
	Player		m_player;
	sf::Text	m_objective_text;
	bool		m_result_pushed;
};
