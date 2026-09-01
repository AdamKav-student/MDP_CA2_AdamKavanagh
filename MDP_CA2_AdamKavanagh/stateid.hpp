// Adam Kavanagh - D00247069
#pragma once
enum class StateID
{
	kNone,
	kTitle,
	kMenu,
	kTraining,      // single-tank tutorial map
	kPause,
	kNetworkPause,
	kSettings,
	kGameOver,      // defeat screen
	kMissionSuccess,// victory screen
	kHostGame,
	kJoinGame
};
