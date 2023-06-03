#pragma once
#include "UDPClient.h"
#include "game.h"
#include "MessageModes.h"

class ClientsGame
{
public:
	ClientsGame();

	UDPClient* client;
	Game* game;

	void UpdateShownMessage(std::string message);
	void EnterKeyPressed(sf::String* message);
	void ShowGamesMenu();
	void PlayerMovedCharacter(CommandType cmndType, sf::Vector2f direction);

	void SetIsPlaying(bool isPlaying);
	bool GetIsPlaying();
	void SetOnlyOneConnectedPlayer(bool _onlyOneConnectedPlayer);
	void SetPlayerCharacter(bool isPlayerOne);
};

