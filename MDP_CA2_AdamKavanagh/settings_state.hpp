// Adam Kavanagh - D00247069
#pragma once
#include "statestack.hpp"
#include "action.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include "container.hpp"
#include "button.hpp"
#include "label.hpp"
#include <array>

// Controls screen. One column of rebindable keys - the tank game has a single
// local crew, so the second column the plane version used for local co-op is
// gone - plus a short briefing on how the match is won.
class SettingsState : public State
{
public:
	SettingsState(StateStack& stack, Context context);

	virtual void Draw() override;
	virtual bool Update(sf::Time dt) override;
	virtual bool HandleEvent(const sf::Event& event) override;

private:
	void UpdateLabels();
	void AddButtonLabel(Action action, std::size_t row, const std::string& text, Context context);

private:
	sf::Sprite		m_background_sprite;
	gui::Container	m_gui_container;
	std::array<gui::Button::Ptr, static_cast<int>(Action::kActionCount)> m_binding_buttons;
	std::array<gui::Label::Ptr, static_cast<int>(Action::kActionCount)> m_binding_labels;
};
