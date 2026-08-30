#pragma once
enum class ReceiverCategories
{
	kNone = 0,
	kScene = 1 << 0,
	kPlayerAircraft = 1 << 1,
	kAlliedAircraft = 1 << 2,
	kEnemyAircraft = 1 << 3, 
	kAlliedProjectile = 1 << 4,
	kEnemyProjectile = 1 << 5,
	kPickup = 1 << 6,
	kParticleSystem = 1 << 7,
	kSoundEffect = 1 << 8,
	kNetwork = 1 << 9,

    kObstacle = 1 << 10,

    // Tank conversion: team-tagged categories so collision/damage logic can
    // tell friend from foe.
    kAxisTeamTank = 1 << 11,
    kAlliesTeamTank = 1 << 12,
    kAxisTeamProjectile = 1 << 13,
    kAlliesTeamProjectile = 1 << 14,

	kAircraft = kPlayerAircraft | kAlliedAircraft | kEnemyAircraft,
	kProjectile = kAlliedProjectile | kEnemyProjectile
    kAnyTank = kAxisTeamTank | kAlliesTeamTank,
    kAnyTankProjectile = kAxisTeamProjectile | kAlliesTeamProjectile
};

//A message that would be sent to all aircraft would be
//unsigned int all_aircraft = ReceiverCategories::kPlayer | ReceiverCategories::kAlloedAircraft | ReceiverCategories::kEnemyAircraft