// Adam Kavanagh - D00247069
#include "game_state.hpp"
#include "tutorial_config.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

GameState::GameState(StateStack& stack, Context context)
	: State(stack, context)
	, m_world(*context.window, *context.fonts, *context.sound, false)
	// No socket: nothing here is networked, so the Player only drives the
	// local tank and never tries to report anything.
	, m_player(nullptr, TutorialConfig::kPlayerIdentifier, context.keys)
	, m_objective_text(context.fonts->Get(FontID::kMain))
	, m_result_pushed(false)
{
	m_objective_text.setCharacterSize(22);
	m_objective_text.setFillColor(sf::Color::White);
	m_objective_text.setPosition(sf::Vector2f(20.f, 20.f));
	m_objective_text.setString("TRAINING\nWASD drives, arrow keys traverse the turret, Space fires\nObjective: destroy the enemy Panzer");

	context.music->Play(MusicThemes::kMissionTheme);
}

void GameState::Draw()
{
	m_world.Draw();
	DrawObjectiveText();
}

void GameState::DrawObjectiveText()
{
	sf::RenderWindow& window = *GetContext().window;
	window.setView(window.getDefaultView());
	window.draw(m_objective_text);
}

bool GameState::Update(sf::Time dt)
{
	m_world.Update(dt);

	if (!m_result_pushed)
	{
		if (m_world.IsTutorialComplete())
		{
			// The victory card holds for kResultScreenSeconds and then clears
			// the stack back to the menu by itself.
			m_result_pushed = true;
			RequestStackPush(StateID::kMissionSuccess);
		}
		else if (m_world.IsTutorialPlayerDestroyed())
		{
			m_result_pushed = true;
			RequestStackPush(StateID::kGameOver);
		}
	}

	CommandQueue& commands = m_world.GetCommandQueue();
	m_player.HandleRealtimeInput(commands);
	return true;
}

bool GameState::HandleEvent(const sf::Event& event)
{
	CommandQueue& commands = m_world.GetCommandQueue();
	m_player.HandleEvent(event, commands);

	const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
	if (key_pressed && key_pressed->scancode == sf::Keyboard::Scancode::Escape)
	{
		RequestStackPush(StateID::kPause);
	}
	return true;
}
