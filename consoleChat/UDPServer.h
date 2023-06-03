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
#include "game.h"

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
		std::map<int, Command> clientCommands;
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
		"Name this painting:\n\n\n\n          ____  \n        o8%8888,    \n      o88%8888888.  \n     8'-    -:8888b   \n    8'         8888  \n   d8.-=. ,==-.:888b  \n   >8 `~` :`~' d8888   \n   88         ,88888   \n   88b. `-~  ':88888  \n   888b ~==~ .:88888 \n   88888o--:':::8888      \n   `88888| :::' 8888b  \n   8888^^'       8888b  \n  d888           ,%888b.   \n d88%            %%%8--'-.  \n/88:.__ ,       _%-' ---  -  \n    '''::===..-'   =  --. \n",
		//          ____  
		//        o8%8888,    
		//      o88%8888888.  
		//     8'-    -:8888b   
		//    8'         8888  
		//   d8.-=. ,==-.:888b  
		//   >8 `~` :`~' d8888   
		//   88         ,88888   
		//   88b. `-~  ':88888  
		//   888b ~==~ .:88888 
		//   88888o--:':::8888      
		//   `88888| :::' 8888b  
		//   8888^^'       8888b  
		//  d888           ,%888b.   
		// d88%            %%%8--'-.  
		///88:.__ ,       _%-' ---  -  
		//    '''::===..-'   =  --. 
	};
	std::vector<std::string> solutions =
	{
		"dog",
		"monalisa"
	};

	struct Match
	{
		bool isFull = false;
		std::string creatorUsername;
		int otherPlayerID = -1;
		Game* game;
	};
	std::map<int, Match> clientsMatches;
	std::list<Game*> activeGames;

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
	void ReceiveUpdateCharacter(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort);
	
	void CalculateAverageRTT();
	void CreateGame(int id);
	void ProcessReceivedCommands();
	void IterateAndValidateCommandMessages(std::map<int, Client>::iterator it);

	bool GetIsRunning();
};

