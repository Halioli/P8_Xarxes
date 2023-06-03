#include "UDPClient.h"
#include "ClientsGame.h"

void UDPClient::BindSocket()
{
	while (socket.bind(port) != sf::Socket::Done)
	{
		++port;
	}

	//std::cout << port << std::endl;
}

bool UDPClient::SendMessage(std::string* message)
{
	if (*message == "exit")
	{
		// Desconection
		message->clear();
		return false;
	}
	else
	{
		sf::Packet mssgPacket;
		mssgPacket << ID << MessageModes::MESSAGE << &message;
		Send(&socket, mssgPacket, serverIP, shortServerPort);

		message->clear();
	}

	return true;
}

void UDPClient::SendLogin(std::string* name)
{
	sf::Packet packet;
	packet << ID << MessageModes::LOGIN << port << *name;

	Send(&socket, packet, serverIP, shortServerPort);
	CriticalMessageSent(++lastMessageSentID, packet, &socket, serverIP, shortServerPort);

	name->clear();
}

void UDPClient::SendChallengeResponse(std::string* response)
{
	sf::Packet packet;
	packet << ID << MessageModes::CHALLENGE << *response;

	Send(&socket, packet, serverIP, shortServerPort);
	CriticalMessageSent(++lastMessageSentID, packet, &socket, serverIP, shortServerPort);

	response->clear();
}

void UDPClient::SendSelectedGameOption(std::string* gameOption)
{
	if (gameOption[0] == "1")
	{
		// Join existing game
		sf::Packet outPacket;
		outPacket << ID << MessageModes::JOIN_GAME;

		Send(&socket, outPacket, serverIP, shortServerPort);
		CriticalMessageSent(++lastMessageSentID, outPacket, &socket, serverIP, serverPort);
	}
	else if (gameOption[0] == "2")
	{
		// Create new game
		sf::Packet outPacket;
		outPacket << ID << MessageModes::CREATE_GAME;

		Send(&socket, outPacket, serverIP, shortServerPort);
		CriticalMessageSent(++lastMessageSentID, outPacket, &socket, serverIP, serverPort);
	}
	else
	{
		// Clear and ask again
		myClientGame->UpdateShownMessage("1 - Join Game || 2 - Create Game\n\n\nIncorrect input");
	}
}

void UDPClient::Receive(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort)
{
    unsigned short shortPort = remotePort;

    if (socket.receive(packet, remoteIP, shortPort) == sf::Socket::Done)
    {
		std::cout << "Received packet from " << remoteIP << " on port " << remotePort << std::endl;

		int _id;
		int _mssgMode;
		packet >> _id >> _mssgMode;

		if (_id != ID && _mssgMode != MessageModes::CHALLENGE && _mssgMode != MessageModes::ACK)
		{
			std::cout << _mssgMode << std::endl;
			return;
		}

		switch (_mssgMode)
		{
		case CHALLENGE:
			ID = _id;
			if (inputMode != MessageModes::CHALLENGE)
				ReceiveChallenge(packet, remoteIP, remotePort);
			else
			{
				ClientSendAcknoledge(MessageModes::CHALLENGE, remoteIP, remotePort);
			}
			break;

		case CHALLENGE_RESULT:
			if (!inputMode != MessageModes::GAME_SELECTION)
				ReceiveChallengeResult(packet, remoteIP, remotePort);
			else
				ClientSendAcknoledge(MessageModes::GAME_SELECTION, remoteIP, remotePort);
			break;

		case MESSAGE:
			break;

		case ACK:
			ReceiveAcknowledge(packet, remoteIP, remotePort);
			break;

		case ENTER_GAME:
			if (inputMode != MessageModes::ENTER_GAME)
				ReceiveEnterGame(packet, remoteIP, remotePort);
			else
			{
				ClientSendAcknoledge(MessageModes::ENTER_GAME, remoteIP, remotePort);
			}
			break;

		case UPDATE_LOCAL_GAME:
			ReceiveUpdateLocalGame(packet, remoteIP, remotePort);
			break;

		case UPDATE_REMOTE_GAME:
			break;

		case DISCONNECT:
			break;

		default:
			break;
		}
	}
}

void UDPClient::ReceiveChallenge(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort)
{
	ClientSendAcknoledge(MessageModes::CHALLENGE, remoteIP, remotePort);

	std::string challenge;
	packet >> challenge;

	myClientGame->UpdateShownMessage(challenge);

	std::cout << challenge << std::endl;
	inputMode = MessageModes::CHALLENGE;
}

void UDPClient::ReceiveChallengeResult(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort)
{
	ClientSendAcknoledge(MessageModes::CHALLENGE_RESULT, remoteIP, remotePort);

	bool reslut;
	packet >> reslut;

	if (!reslut)
	{
		forceQuit = true;
		inputMode = MessageModes::DISCONNECT;
	}
	else
	{
		inputMode = MessageModes::GAME_SELECTION;
		myClientGame->ShowGamesMenu();
	}
}

void UDPClient::ReceiveAcknowledge(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort)
{
	mtx.lock();
	critMessages.erase(idsToMessageIDs[ID]);
	idsToMessageIDs.erase(ID);
	mtx.unlock();

	int ack;
	packet >> ack;

	switch (ack)
	{
	case LOGIN:
		std::cout << "LOGIN ACK" << std::endl;
		break;

	case CHALLENGE:
		std::cout << "CHALLENGE ACK" << std::endl;
		break;

	case CHALLENGE_RESULT:
		std::cout << "CHALLENGE_RESULT ACK" << std::endl;
		break;

	case JOIN_GAME:
	case CREATE_GAME:
		std::cout << "JOIN/CREATE_GAME ACK" << std::endl;
		break;

	case DISCONNECT:
		break;
	default:
		break;
	}
}

void UDPClient::ReceiveEnterGame(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort)
{
	ClientSendAcknoledge(MessageModes::ENTER_GAME, remoteIP, remotePort);
	bool joiningGame;
	packet >> joiningGame;

	if (!joiningGame)
	{
		// Wait for new player
		myClientGame->SetOnlyOneConnectedPlayer(true);
	}
	else
	{
		// Play
		myClientGame->SetOnlyOneConnectedPlayer(false);
		myClientGame->SetPlayerCharacter(false);
	}

	myClientGame->SetIsPlaying(true);
}

void UDPClient::ReceiveUpdateLocalGame(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort)
{
	int lastValidCommandId, lastServerReceivedCommandId, firstServerReceivedCommandId;
	packet >> lastValidCommandId >> lastServerReceivedCommandId >> firstServerReceivedCommandId;

	if (lastValidCommandId == -1)
	{
		mtx.lock();
		if (firstServerReceivedCommandId != 0)
			myClientGame->game->SetPlayerPosition(executedCommands[firstServerReceivedCommandId - 1].newPos);
		else
			myClientGame->game->SetPlayerPosition(executedCommands[0].newPos);
		mtx.unlock();
	}
	else if (lastValidCommandId < lastServerReceivedCommandId)
	{
		// Some command was invalid
		mtx.lock();
		myClientGame->game->SetPlayerPosition(executedCommands[lastValidCommandId].newPos);
		mtx.unlock();
	}
}

void UDPClient::ClientSendAcknoledge(MessageModes messageMode, sf::IpAddress& remoteIP, int& remotePort)
{
	unsigned short shorPort = remotePort;
	SendAcknowledge(&socket, (int)messageMode, ID, remoteIP, shorPort);
}

void UDPClient::SaveNewCommand(CommandType commandType, sf::Vector2f position)
{
	Command newCommand;
	newCommand.cmndId = ++lastMessageCommandId;
	newCommand.cmndType = commandType;
	newCommand.newPos = position;

	mtx.lock();
	pendingCommands.push_back(newCommand);
	mtx.unlock();
}

void UDPClient::SendCommands()
{
	sf::Packet outPacket;
	int pendingCommandsSize;

	while (!forceQuit)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(80));

		mtx.lock();
		if (pendingCommands.size() > 0)
		{
			pendingCommandsSize = pendingCommands.size();
			outPacket << ID << MessageModes::UPDATE_CHARACTER << pendingCommandsSize;
			for (int i = 0; i < pendingCommands.size(); i++)
			{
				outPacket << pendingCommands[i].cmndId << pendingCommands[i].cmndType << pendingCommands[i].newPos.x << pendingCommands[i].newPos.y;
				executedCommands.push_back(pendingCommands[i]);
			}
			pendingCommands.clear();

			Send(&socket, outPacket, serverIP, shortServerPort);
			outPacket.clear();
		}
		mtx.unlock();
	}
}

bool UDPClient::GetForceQuit()
{
	return forceQuit;
}

void UDPClient::SetForceQuit(bool _forceQuit)
{
	forceQuit = _forceQuit;
}
