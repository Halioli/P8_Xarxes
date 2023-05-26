#include "ClientsGame.h"

void ClientsGame::UpdateShownMessage(std::string message)
{
	game->SetMessage(message);
}

void ClientsGame::EnterKeyPressed(sf::String* message)
{
	if (message->getSize() > 0)
	{
		switch (client->inputMode)
		{
		case LOGIN:
			client->SendLogin(&message->toAnsiString());
			break;
		case CHALLENGE:
			client->SendChallengeResponse(&message->toAnsiString());
			break;
		case MESSAGE:
			if (!client->SendMessage(&message->toAnsiString()))
			{
				client->SetForceQuit(true);
			}
			break;

		case GAME_SELECTION:
			client->SendSelectedGameOption(&message->toAnsiString());
			break;

		case DISCONNECT:
			client->SetForceQuit(true);
			break;
		default:
			break;
		}
		message->clear();
	}
}

void ClientsGame::ShowGamesMenu()
{
	std::string newShownMessage = "1 - Join Game || 2 - Create Game";
	UpdateShownMessage(newShownMessage);
}

void ClientsGame::SetIsPlaying(bool isPlaying)
{
	game->SetPlaying(isPlaying);
}
