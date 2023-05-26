#pragma once
#include "UDPClient.h"
#include "game.h"

class ClientsGame
{
public:
	UDPClient* client;
	Game* game;

	void SendUsername();
	void UpdateShownMessage(std::string message);
	void EnterKeyPressed(sf::String* message);
};

