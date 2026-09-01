// Adam Kavanagh - D00247069
#pragma once
#include "scene_node.hpp"
#include "network_protocol.hpp"
#include <queue>

// Sits in the scene graph purely so that gameplay code deep inside the world
// can queue something for the network layer without knowing about sockets.
// MultiplayerGameState drains the queue each frame and sends it on.
class NetworkNode : public SceneNode
{
public:
    NetworkNode();
    void NotifyGameAction(GameActions::Type type, uint8_t subject, uint8_t other, sf::Vector2f position);
    bool PollGameAction(GameActions::Action& out);
    virtual unsigned int GetCategory() const override;

private:
    std::queue<GameActions::Action> m_pending_actions;
};
