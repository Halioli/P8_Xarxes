#pragma once
#include <SFML\Network.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <list>
#include "MessageModes.h"
#include "UDPHandler.h"

class UDPServer : public UDPHandler
{
private:
	const int PORT = 5000;
	struct Client 
	{
		sf::IpAddress ip;
		int port;
		std::string name;
		int clientID;
		int ts;
	};

	struct NewConnection
	{
		sf::IpAddress ip;
		int port;
		std::string name;
		int challengeTimeout;
		std::string challenge;
		std::string solution;
	};

	sf::UdpSocket socket;
	unsigned int idValues = 0;

public:
	std::map<int, Client> clients;
	std::map<int, NewConnection> newConnections;

	void BindSocket();
	void Receive(sf::Packet & inPacket, sf::IpAddress &remoteIP, int &remotePort);
	void ReceiveAcknowledge(int id, sf::Packet& inPacket, sf::IpAddress& remoteIP, unsigned short& remotePort);
	void ReceiveLogin(sf::Packet& inPacket, sf::IpAddress& remoteIP, unsigned short& remotePort);
	void ReceiveChallengeResponse(int id, sf::Packet& inPacket, sf::IpAddress& remoteIP, unsigned short& remotePort);
	void ReceiveMessage(int id, sf::Packet& inPacket, sf::IpAddress& remoteIP, unsigned short& remotePort);
};

