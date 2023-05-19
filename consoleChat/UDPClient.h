#pragma once
#include <SFML\Network.hpp>
#include <string>
#include <iostream>
#include <chrono>
#include "MessageModes.h"
#include "UDPHandler.h"

class UDPClient : public UDPHandler
{
private:
	int ID = -1;
	sf::UdpSocket socket;
	int port = 5001;

	bool forceQuit = false;

public:
	int serverPort;
	sf::IpAddress serverIP;
	MessageModes inputMode = MessageModes::LOGIN;

	void BindSocket();
	bool SendMessage(std::string* message);
	void SendLogin(std::string* name);
	void SendChallengeResponse(std::string* response);
	void Receive(sf::Packet &packet, sf::IpAddress &remoteIP, int &remotePort);
	void ReceiveChallenge(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort);
	void ReceiveChallengeResult(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort);
	void ReceiveAcknowledge(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort);

	bool GetForceQuit();
};
