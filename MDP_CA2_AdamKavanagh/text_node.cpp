// Adam Kavanagh - D00247069
#include "text_node.hpp"
#include "utility.hpp"

TextNode::TextNode(const FontHolder& fonts, std::string& text)
	:m_text(fonts.Get(FontID::kMain), text, 20)
{
}

void TextNode::SetColour(sf::Color colour)
{
	m_text.setFillColor(colour);
}

void TextNode::SetString(const std::string& text)
{
	m_text.setString(text);
	Utility::CentreOrigin(m_text);
}

//unsigned int TextNode::GetCategory() const
//{
//	return static_cast<int>(ReceiverCategories::kNone);
//}

void TextNode::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_text, states);
}
