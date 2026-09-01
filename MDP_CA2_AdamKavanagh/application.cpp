// Adam Kavanagh - D00247069
#include "application.hpp"
#include "constants.hpp"
#include "fontID.hpp"
#include "game_state.hpp"
#include "title_state.hpp"
#include "menu_state.hpp"
#include "pause_state.hpp"
#include "settings_state.hpp"
#include "result_state.hpp"
#include "multiplayer_gamestate.hpp"

Application::Application()
	: m_window(sf::VideoMode::getDesktopMode(), "Armoured Warfare: 1944", sf::Style::Close)
	, m_key_binding()
	, m_stack(State::Context(m_window, m_textures, m_fonts, m_music, m_sound, m_key_binding))
{
	m_window.setKeyRepeatEnabled(false);

	m_fonts.Load(FontID::kMain, "Media/Fonts/BOMBARD_.ttf");
	m_textures.Load(TextureID::kTitleScreen, "Media/Menu/Menu New.png");
	m_textures.Load(TextureID::kSettingsScreen, "Media/Menu/Settings New.png");
	m_textures.Load(TextureID::kVictoryScreen, "Media/Menu/Victory Image.png");
	m_textures.Load(TextureID::kDefeatScreen, "Media/Menu/Defeat Image.png");
	m_textures.Load(TextureID::kButtons, "Media/Textures/Buttons.png");

	RegisterStates();
	m_stack.PushState(StateID::kTitle);
}

void Application::Run()
{
	sf::Clock clock;
	sf::Time time_since_last_update = sf::Time::Zero;

	while (m_window.isOpen())
	{
		time_since_last_update += clock.restart();
		while (time_since_last_update.asSeconds() > kTimePerFrame)
		{
			time_since_last_update -= sf::seconds(kTimePerFrame);
			ProcessInput();
			Update(sf::seconds(kTimePerFrame));

			if (m_stack.IsEmpty())
			{
				m_window.close();
			}
		}
		Render();
	}
}

void Application::ProcessInput()
{
	while (const std::optional event = m_window.pollEvent())
	{
		m_stack.HandleEvent(*event);

		if (event->is<sf::Event::Closed>())
		{
			m_window.close();
		}
	}
}

void Application::Update(sf::Time dt)
{
	m_stack.Update(dt);
}

void Application::Render()
{
	m_window.clear();
	m_stack.Draw();
	m_window.display();
}

void Application::RegisterStates()
{
	m_stack.RegisterState<TitleState>(StateID::kTitle);
	m_stack.RegisterState<MenuState>(StateID::kMenu);
	m_stack.RegisterState<GameState>(StateID::kTraining);
	m_stack.RegisterState<MultiplayerGameState>(StateID::kHostGame, true);
	m_stack.RegisterState<MultiplayerGameState>(StateID::kJoinGame, false);
	m_stack.RegisterState<PauseState>(StateID::kPause);
	m_stack.RegisterState<PauseState>(StateID::kNetworkPause, true);
	m_stack.RegisterState<SettingsState>(StateID::kSettings);
	m_stack.RegisterState<ResultState>(StateID::kMissionSuccess, true);
	m_stack.RegisterState<ResultState>(StateID::kGameOver, false);
}
