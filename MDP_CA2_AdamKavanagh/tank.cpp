#include "tank.hpp"
#include "data_tables.hpp"
#include "texture_id.hpp"
#include "utility.hpp"

namespace
{
	const std::vector<TankData> Table = InitializeTankData();
}

Tank::Tank(TankType type, const TextureHolder& textures, const FontHolder& fonts)
	: Entity(Table[static_cast<int>(type)].m_hitpoints)
	, m_type(type)
	, m_identifier(0)
	, m_hull_sprite(textures.Get(Table[static_cast<int>(type)].m_texture), Table[static_cast<int>(type)].m_hull_rect)
	, m_turret(nullptr)
	, m_health_display(nullptr)
	, m_wants_to_fire(false)
	, m_is_firing(false)
	, m_fire_countdown(sf::Time::Zero)
	, m_show_wreck(true)
{
	Utility::CentreOrigin(m_hull_sprite);

	std::unique_ptr<TurretNode> turret(new TurretNode(
		textures.Get(Table[static_cast<int>(type)].m_texture),
		Table[static_cast<int>(type)].m_turret_rect,
		Table[static_cast<int>(type)].m_turret_pivot));
	m_turret = turret.get();
	AttachChild(std::move(turret));
	// No further setup needed: the turret already defaults to "aligned with
	// hull" (see TurretNode's constructor) and will only move if
	// RotateTurretBy() is called from arrow-key input.

	std::string health_text = "";
	std::unique_ptr<TextNode> health_display(new TextNode(fonts, health_text));
	health_display->setPosition(sf::Vector2f(0.f, 70.f));
	m_health_display = health_display.get();
	AttachChild(std::move(health_display));

	UpdateTexts();
}

unsigned int Tank::GetCategory() const
{
	switch (Table[static_cast<int>(m_type)].m_team)
	{
	case TeamID::kAxis:
		return static_cast<unsigned int>(ReceiverCategories::kAxisTeamTank);
	case TeamID::kAllies:
		return static_cast<unsigned int>(ReceiverCategories::kAlliesTeamTank);
	default:
		return static_cast<unsigned int>(ReceiverCategories::kNone);
	}
}

TeamID Tank::GetTeam() const
{
	return Table[static_cast<int>(m_type)].m_team;
}

uint8_t Tank::GetIdentifier() const
{
	return m_identifier;
}

void Tank::SetIdentifier(uint8_t identifier)
{
	m_identifier = identifier;
}

float Tank::GetMaxSpeed() const
{
	return Table[static_cast<int>(m_type)].m_speed;
}

void Tank::RotateTurretBy(float delta_degrees)
{
	m_turret->RotateBy(delta_degrees);
}

float Tank::GetTurretLocalRotationDegrees() const
{
	return m_turret->GetLocalRotationDegrees();
}

void Tank::SetTurretLocalRotationDegrees(float degrees)
{
	m_turret->SetLocalRotationDegrees(degrees);
}

sf::Vector2f Tank::GetMuzzleWorldPosition() const
{
	return m_turret->GetMuzzleWorldPosition();
}

void Tank::Fire()
{
	m_wants_to_fire = true;
}

bool Tank::IsFiring() const
{
	return m_is_firing;
}

void Tank::UpdateTexts()
{
	if (IsDestroyed())
	{
		m_health_display->SetString("");
	}
	else
	{
		m_health_display->SetString(std::to_string(GetHitPoints()) + "HP");
	}
	// Keep health text upright in world coordinates
	m_health_display->setRotation(sf::degrees(-getRotation().asDegrees()));
}

// Re-added (see tank.hpp comment) - needed for accessibility, not
// behavior; SceneNode's own default already does the right thing.
bool Tank::IsMarkedForRemoval() const
{
	return IsDestroyed();
}

sf::FloatRect Tank::GetBoundingRect() const
{
	return GetWorldTransform().transformRect(m_hull_sprite.getGlobalBounds());
}

void Tank::Remove()
{
	Entity::Remove();
	m_show_wreck = false;
}

void Tank::DrawCurrent(sf::RenderTarget& target, sf::RenderStates states) const
{
	target.draw(m_hull_sprite, states);
}

void Tank::UpdateCurrent(sf::Time dt, CommandQueue& commands)
{
	if (IsDestroyed())
	{
		return;
	}

	Entity::UpdateCurrent(dt, commands);
	UpdateTexts();

	// Deliberately nothing turret-related here: the turret is a normal
	// child SceneNode, so it already rotates along with the hull for free.
	// Its rotation only changes when RotateTurretBy() is called elsewhere
	// (arrow-key input in Player, or SetTurretLocalRotationDegrees() when
	// applying network state).

	// Fire-rate cooldown: Fire() (called every frame the fire key is held,
	// via Player's realtime input) only sets m_wants_to_fire - the actual
	// shot is gated here so holding the key doesn't spawn a shell every
	// tick. IsFiring() is true for exactly the one frame a shot is
	// released; World should spawn a shell then, on that frame only.
	if (m_fire_countdown > sf::Time::Zero)
		m_fire_countdown -= dt;

	m_is_firing = false;
	if (m_wants_to_fire && m_fire_countdown <= sf::Time::Zero)
	{
		m_is_firing = true;
		m_fire_countdown += Table[static_cast<int>(m_type)].m_fire_interval;
	}
	m_wants_to_fire = false;
}
