#pragma once
#include <SFML\Network.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <list>
#include <queue>
#include <thread>
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
		int lastMessageMode;
	};

	struct NewConnection
	{
		sf::IpAddress ip;
		int port;
		std::string name;
		int challengeTimeout;
		std::string challenge;
		std::string solution;
		int lastMessageMode;
	};

	sf::UdpSocket socket;
	unsigned int idValues = 0;
	bool isRunning = true;

	std::list<float> lastACKsTimeDifference;

	std::vector<std::string> challenges =
	{
		"Name this animal:\n\n\n\n^..^       /\n/_/ \\_____/\n    /\\  / \\\n   /  \\/   \\",
		//     ^..^       /
		//     /_/ \_____/
		//         /\  / \
	    //        /  \/   \ 

	};
	std::vector<std::string> solutions =
	{
		"dog"
	};

public:
	std::map<int, Client> clients;
	std::map<int, NewConnection> newConnections;

	void BindSocket();
	void Receive(sf::Packet & inPacket, sf::IpAddress remoteIP, int &remotePort);
	void ReceiveAcknowledge(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort);
	void ReceiveLogin(sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort);
	void ReceiveChallengeResponse(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort);
	void ReceiveMessage(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort);
	void ReceiveJoinGame(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort);
	void ReceiveCreateGame(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort);
	void CalculateAverageRTT();

	bool GetIsRunning();
};

