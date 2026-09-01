// Adam Kavanagh - D00247069
#include "title_state.hpp"
#include "fontID.hpp"
#include "utility.hpp"
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

TitleState::TitleState(StateStack& stack, Context context) : State(stack, context), m_show_text(true), m_text_effect_time(sf::Time::Zero), m_background_sprite(context.textures->Get(TextureID::kTitleScreen)), m_text(context.fonts->Get(FontID::kMain))
{
    m_text.setString("Press any key to continue");
    Utility::CentreOrigin(m_text);
    m_text.setPosition(context.window->getView().getSize() / 2.f);
}

void TitleState::Draw()
{
    sf::RenderWindow& window = *GetContext().window;
    // Scale title background to fill the current window size
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

    if (m_show_text)
    {
        window.draw(m_text);
    }
}

bool TitleState::Update(sf::Time dt)
{
    m_text_effect_time += dt;
    if (m_text_effect_time >= sf::seconds(0.5))
    {
        m_show_text = !m_show_text;
        m_text_effect_time = sf::Time::Zero;
    }
    return true;
}

bool TitleState::HandleEvent(const sf::Event& event)
{
    const auto* key_pressed = event.getIf<sf::Event::KeyPressed>();
    if (key_pressed)
    {
        RequestStackPop();
        RequestStackPush(StateID::kMenu);
    }
    return true;
}
