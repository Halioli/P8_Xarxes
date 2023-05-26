#include "ClientsGame.h"

void ClientsGame::SendUsername()
{
	std::string username = game->GetNameText();
	client->SendLogin(&username);
}

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
		case DISCONNECT:
			client->SetForceQuit(true);
			break;
		default:
			break;
		}
		message->clear();
	}
}
