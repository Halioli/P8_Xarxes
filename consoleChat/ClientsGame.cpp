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
