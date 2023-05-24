#include "UDPClient.h"

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
			ReceiveChallenge(packet, remoteIP, remotePort);
			break;

		case CHALLENGE_RESULT:
			ReceiveChallengeResult(packet, remoteIP, remotePort);
			break;

		case MESSAGE:
			break;

		case ACK:
			ReceiveAcknowledge(packet, remoteIP, remotePort);
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
	unsigned short shorPort = remotePort;
	SendAcknowledge(&socket, (int)MessageModes::CHALLENGE, ID, remoteIP, shorPort);

	std::string challenge;
	packet >> challenge;

	std::cout << challenge << std::endl;
	inputMode = MessageModes::CHALLENGE;
}

void UDPClient::ReceiveChallengeResult(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort)
{
	unsigned short shorPort = remotePort;
	SendAcknowledge(&socket, (int)MessageModes::CHALLENGE_RESULT, ID, remoteIP, shorPort);

	bool reslut;
	packet >> reslut;

	if (!reslut)
	{
		forceQuit = true;
		inputMode = MessageModes::DISCONNECT;
	}
	else
	{
		inputMode = MessageModes::MESSAGE;
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
	case DISCONNECT:
		break;
	default:
		break;
	}
}

bool UDPClient::GetForceQuit()
{
	return forceQuit;
}
