// Adam Kavanagh - D00247069
#include "settings_state.hpp"
#include "key_binding.hpp"
#include "utility.hpp"
#include <functional>
#include <memory>
#include <string>

SettingsState::SettingsState(StateStack& stack, Context context)
	: State(stack, context)
	, m_background_sprite(context.textures->Get(TextureID::kSettingsScreen))
	, m_gui_container()
{
	AddButtonLabel(Action::kMoveForward, 0, "Drive forward", context);
	AddButtonLabel(Action::kMoveBackward, 1, "Reverse", context);
	AddButtonLabel(Action::kRotateHullLeft, 2, "Turn hull left", context);
	AddButtonLabel(Action::kRotateHullRight, 3, "Turn hull right", context);
	AddButtonLabel(Action::kTurretLeft, 4, "Traverse turret left", context);
	AddButtonLabel(Action::kTurretRight, 5, "Traverse turret right", context);
	AddButtonLabel(Action::kFire, 6, "Fire main gun", context);

	UpdateLabels();

	auto back_button = std::make_shared<gui::Button>(context);
	back_button->setPosition(sf::Vector2f(80.f, 660.f));
	back_button->SetText("Back");
	back_button->SetCallback(std::bind(&SettingsState::RequestStackPop, this));
	m_gui_container.Pack(back_button);

	auto brief_1 = std::make_shared<gui::Label>("Multiplayer: first team to 20 knock-outs wins", *context.fonts);
	brief_1->setPosition(sf::Vector2f(850.f, 300.f));
	m_gui_container.Pack(brief_1);

	auto brief_2 = std::make_shared<gui::Label>("A match lasts 15 minutes; destroyed tanks respawn", *context.fonts);
	brief_2->setPosition(sf::Vector2f(850.f, 350.f));
	m_gui_container.Pack(brief_2);

	auto brief_3 = std::make_shared<gui::Label>("Odd player numbers crew Shermans (Allies),", *context.fonts);
	brief_3->setPosition(sf::Vector2f(850.f, 420.f));
	m_gui_container.Pack(brief_3);

	auto brief_4 = std::make_shared<gui::Label>("even numbers crew Panzers (Axis)", *context.fonts);
	brief_4->setPosition(sf::Vector2f(850.f, 460.f));
	m_gui_container.Pack(brief_4);

	auto brief_5 = std::make_shared<gui::Label>("Training: destroy the lone Panzer to complete the mission", *context.fonts);
	brief_5->setPosition(sf::Vector2f(850.f, 530.f));
	m_gui_container.Pack(brief_5);
}

void SettingsState::Draw()
{
	sf::RenderWindow& window = *GetContext().window;
	window.setView(window.getDefaultView());

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
	window.draw(m_gui_container);
}

bool SettingsState::Update(sf::Time dt)
{
	return true;
}

bool SettingsState::HandleEvent(const sf::Event& event)
{
	bool is_key_binding = false;

	// A button left "active" is waiting for the next key press to become the
	// new binding for its action.
	for (std::size_t i = 0; i < m_binding_buttons.size(); ++i)
	{
		if (!m_binding_buttons[i]->IsActive())
		{
			continue;
		}

		is_key_binding = true;
		if (const auto* key_event = event.getIf<sf::Event::KeyReleased>())
		{
			GetContext().keys->AssignKey(static_cast<Action>(i), key_event->scancode);
			m_binding_buttons[i]->Deactivate();
		}
		break;
	}

	if (is_key_binding)
	{
		UpdateLabels();
	}
	else
	{
		m_gui_container.HandleEvent(event);
	}

	return false;
}

void SettingsState::UpdateLabels()
{
	for (std::size_t i = 0; i < m_binding_labels.size(); ++i)
	{
		const sf::Keyboard::Scancode key = GetContext().keys->GetAssignedKey(static_cast<Action>(i));
		m_binding_labels[i]->SetText(Utility::toString(key));
	}
}

void SettingsState::AddButtonLabel(Action action, std::size_t row, const std::string& text, Context context)
{
	const std::size_t index = static_cast<std::size_t>(action);

	m_binding_buttons[index] = std::make_shared<gui::Button>(context);
	m_binding_buttons[index]->setPosition(sf::Vector2f(80.f, 50.f * static_cast<float>(row) + 300.f));
	m_binding_buttons[index]->SetText(text);
	m_binding_buttons[index]->SetToggle(true);

	m_binding_labels[index] = std::make_shared<gui::Label>("", *context.fonts);
	m_binding_labels[index]->setPosition(sf::Vector2f(320.f, 50.f * static_cast<float>(row) + 315.f));

	m_gui_container.Pack(m_binding_buttons[index]);
	m_gui_container.Pack(m_binding_labels[index]);
}
