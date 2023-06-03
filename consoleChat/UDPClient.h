#pragma once
#include <SFML\Network.hpp>
#include <string>
#include <iostream>
#include <chrono>
#include "MessageModes.h"
#include "UDPHandler.h"
class ClientsGame;

class UDPClient : public UDPHandler
{
private:
	int ID = -1;
	sf::UdpSocket socket;
	int port = 5001;

	bool forceQuit = false;

	int lastMessageCommandId = 0;
	std::vector<Command> executedCommands; // All of the client's commands
	std::vector<Command> pendingCommands;  // New commands yet to send

public:
	bool isSendCommandsThreatOpen = false;
	int serverPort;
	unsigned short shortServerPort = 5000;
	sf::IpAddress serverIP;
	MessageModes inputMode = MessageModes::LOGIN;
	ClientsGame* myClientGame;

	void BindSocket();
	bool SendMessage(std::string* message);
	void SendLogin(std::string* name);
	void SendChallengeResponse(std::string* response);
	void SendSelectedGameOption(std::string* gameOption);
	void Receive(sf::Packet &packet, sf::IpAddress &remoteIP, int &remotePort);
	void ReceiveChallenge(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort);
	void ReceiveChallengeResult(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort);
	void ReceiveAcknowledge(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort);
	void ReceiveEnterGame(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort);
	void ReceiveUpdateLocalGame(sf::Packet& packet, sf::IpAddress& remoteIP, int& remotePort);

	void ClientSendAcknoledge(MessageModes messageMode, sf::IpAddress& remoteIP, int& remotePort);
	void SaveNewCommand(CommandType commandType, sf::Vector2f position);

	// threats
	void SendCommands();

	// get/set
	bool GetForceQuit();
	void SetForceQuit(bool _forceQuit);
};

