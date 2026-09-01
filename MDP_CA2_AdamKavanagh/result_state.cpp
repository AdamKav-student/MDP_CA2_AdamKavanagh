// Adam Kavanagh - D00247069
#include "result_state.hpp"
#include "constants.hpp"
#include "utility.hpp"
#include <sstream>
#include <string>

ResultState::ResultState(StateStack& stack, Context context, bool victory)
	: State(stack, context)
	, m_background_sprite(context.textures->Get(victory ? TextureID::kVictoryScreen : TextureID::kDefeatScreen))
	, m_countdown_text(context.fonts->Get(FontID::kMain))
	, m_elapsed_time(sf::Time::Zero)
	, m_victory(victory)
{
	m_countdown_text.setCharacterSize(30);
	m_countdown_text.setFillColor(sf::Color::White);

	// The music stops so the card lands on silence rather than the battle
	// theme carrying on behind it.
	context.music->Stop();
}

void ResultState::Draw()
{
	sf::RenderWindow& window = *GetContext().window;
	window.setView(window.getDefaultView());

	// Dim whatever is underneath, then stretch the card over the whole window
	// whatever resolution the desktop happens to be.
	sf::RectangleShape dim;
	dim.setFillColor(sf::Color(0, 0, 0, 200));
	dim.setSize(window.getView().getSize());
	window.draw(dim);

	const sf::Vector2u texture_size = m_background_sprite.getTexture().getSize();
	if (texture_size.x > 0 && texture_size.y > 0)
	{
		const sf::Vector2u window_size = window.getSize();
		m_background_sprite.setScale(sf::Vector2f(
			static_cast<float>(window_size.x) / static_cast<float>(texture_size.x),
			static_cast<float>(window_size.y) / static_cast<float>(texture_size.y)));
		m_background_sprite.setPosition(sf::Vector2f(0.f, 0.f));
	}
	window.draw(m_background_sprite);
	window.draw(m_countdown_text);
}

bool ResultState::Update(sf::Time dt)
{
	m_elapsed_time += dt;

	const int seconds_left = static_cast<int>(kResultScreenSeconds - m_elapsed_time.asSeconds()) + 1;
	std::ostringstream stream;
	stream << (m_victory ? "Objective complete" : "Knocked out")
		<< " - returning to menu in " << (seconds_left > 0 ? seconds_left : 0);
	m_countdown_text.setString(stream.str());
	Utility::CentreOrigin(m_countdown_text);

	sf::Vector2f window_size(GetContext().window->getSize());
	m_countdown_text.setPosition(sf::Vector2f(window_size.x / 2.f, window_size.y * 0.9f));

	if (m_elapsed_time >= sf::seconds(kResultScreenSeconds))
	{
		RequestStackClear();
		RequestStackPush(StateID::kMenu);
	}

	// Nothing underneath this card keeps running while it is up.
	return false;
}

bool ResultState::HandleEvent(const sf::Event& event)
{
	return false;
}
