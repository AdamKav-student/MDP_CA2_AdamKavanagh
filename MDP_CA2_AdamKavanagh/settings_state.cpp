#include "settings_state.hpp"
#include "Utility.hpp"
#include <type_traits>

namespace
{
    template<typename Sprite>
    const sf::Texture* GetTexturePtr(const Sprite& s)
    {
        if constexpr (std::is_pointer_v<decltype(s.getTexture())>)
        {
            return s.getTexture();
        }
        else
        {
            return &s.getTexture();
        }
    }
}

SettingsState::SettingsState(StateStack& stack, Context context)
    : State(stack, context)
    , m_gui_container()
    , m_background_sprite(context.textures->Get(TextureID::kSettingsScreen))
    , m_mission_goal_text1(nullptr)
    , m_mission_goal_text2(nullptr)

{
    //Build key binding buttons and labels
    for (std::size_t x = 0; x < 2; ++x)
    {
        AddButtonLabel(static_cast<int>(Action::kMoveLeft), x, 0, "Move Left", context);
        AddButtonLabel(static_cast<int>(Action::kMoveRight), x, 1, "Move Right", context);
        AddButtonLabel(static_cast<int>(Action::kMoveUp), x, 2, "Move Up", context);
        AddButtonLabel(static_cast<int>(Action::kMoveDown), x, 3, "Move Down", context);
        AddButtonLabel(static_cast<int>(Action::kBulletFire), x, 4, "Fire", context);
        AddButtonLabel(static_cast<int>(Action::kMissileFire), x, 5, "Missile", context);
    }

    UpdateLabels();

	auto back_button = std::make_shared<gui::Button>(context);
    back_button->setPosition(sf::Vector2f(80.f, 620.f));
    back_button->SetText("Back");
    back_button->SetCallback(std::bind(&SettingsState::RequestStackPop, this));
    m_gui_container.Pack(back_button);

    m_mission_goal_text1 = std::make_shared<gui::Label>("Victory conditions: Score 20 kill for your team to win", *context.fonts);
    m_mission_goal_text1->setPosition(sf::Vector2f(850.f,200.f));
    m_gui_container.Pack(m_mission_goal_text1);

    m_mission_goal_text2 = std::make_shared<gui::Label>("Mission Brief: Deplete the enemy forces armoured reseves to ensure victory ", *context.fonts);
    m_mission_goal_text2->setPosition(sf::Vector2f(850.f, 400.f));
    m_gui_container.Pack(m_mission_goal_text2);
}

void SettingsState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    // Scale settings background to fill the current window size
    if (const sf::Texture* tex = GetTexturePtr(m_background_sprite))
    {
        sf::Vector2u texSize = tex->getSize();
        if (texSize.x > 0 && texSize.y > 0)
        {
            sf::Vector2u winSize = window.getSize();
            float scaleX = static_cast<float>(winSize.x) / static_cast<float>(texSize.x);
            float scaleY = static_cast<float>(winSize.y) / static_cast<float>(texSize.y);
            m_background_sprite.setScale(sf::Vector2f(scaleX, scaleY));
            m_background_sprite.setPosition(sf::Vector2f(0.f, 0.f));
        }
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

    //Iterate through all of the key binding buttons to see if they are being pressed, waiting for input from the user
    for (std::size_t action = 0; action < 2*(static_cast<int>(Action::kActionCount)); ++action)
    {
        if (m_binding_buttons[action]->IsActive())
        {
            is_key_binding = true;
            const auto* key_event = event.getIf<sf::Event::KeyReleased>();
            if (key_event)
            {
                sf::Keyboard::Scancode pressed_key = key_event->scancode;
                // Player 1
                if (action < static_cast<int>(Action::kActionCount))
                    GetContext().keys1->AssignKey(static_cast<Action>(action), pressed_key);

                // Player 2
                else
                {
                    auto action_index = action - static_cast<int>(Action::kActionCount);
                    GetContext().keys2->AssignKey(static_cast<Action>(action_index), pressed_key);
                }
                m_binding_buttons[action]->Deactivate();
            }
            break;
        }
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
    for (std::size_t i = 0; i < static_cast<int>(Action::kActionCount); ++i)
    {
        auto action = static_cast<Action>(i);

        // Get keys of both players
        sf::Keyboard::Scancode key1 = GetContext().keys1->GetAssignedKey(action);
        sf::Keyboard::Scancode key2 = GetContext().keys2->GetAssignedKey(action);

        // Assign both key strings to labels
        m_binding_labels[i]->SetText(Utility::toString(key1));
        m_binding_labels[i + static_cast<int>(Action::kActionCount)]->SetText(Utility::toString(key2));
    }
}

void SettingsState::AddButtonLabel(std::size_t index, std::size_t x, std::size_t y, const std::string& text, Context context)
{
    // For x==0, start at index 0, otherwise start at half of array
    index += static_cast<int>(Action::kActionCount) * x;

    m_binding_buttons[index] = std::make_shared<gui::Button>(context);
    m_binding_buttons[index]->setPosition(sf::Vector2f(400.f * x + 80.f, 50.f * y + 300.f));
    m_binding_buttons[index]->SetText(text);
    m_binding_buttons[index]->SetToggle(true);

    m_binding_labels[index] = std::make_shared<gui::Label>("", *context.fonts);
    m_binding_labels[index]->setPosition(sf::Vector2f(400.f * x + 300.f, 50.f * y + 315.f));


    m_gui_container.Pack(m_binding_buttons[index]);
    m_gui_container.Pack(m_binding_labels[index]);
}
