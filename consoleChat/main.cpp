#pragma once
#include <SFML\Network.hpp>
#include <iostream> 
#include <string>
#include <thread>
#include "TCPSocketManager.h"
#include "UDPServer.h"
#include "UDPClient.h"
#include "game.h"
#include "ClientsGame.h"

const float PKT_LOSS_PROB = 0.25f;
const unsigned short PORT = 5000;
const sf::IpAddress IP = "127.0.0.1";
bool applicationRunning = true;

enum Mode
{
	SERVER,
	CLIENT,
	COUNT
};

std::string username;
bool hasLogedIn = false;

// Function to read from console (adapted for threads)
void GetLineFromCin(std::string* mssg) 
{
	while (applicationRunning) 
	{
		std::string line;
		std::getline(std::cin, line);
		mssg->assign(line);
	}
}

void OpenReceiveThread(UDPClient* _udpClient)
{
	sf::Packet receivePacket;
	sf::IpAddress _ip = IP;
	int _port = PORT;

	while (applicationRunning)
	{
		_udpClient->Receive(receivePacket, _ip, _port);
		receivePacket.clear();
	}
}

void OpenListener(UDPServer* _udpServer)
{
	sf::Packet receivePacket;
	sf::IpAddress _ip = IP;
	int _port = PORT;

	while (applicationRunning)
	{
		for (int i = 0; i < _udpServer->clients.size(); i++)
		{
			_udpServer->Receive(receivePacket, _udpServer->clients[i].ip, _udpServer->clients[i].port);
		}

		for (int i = 0; i < _udpServer->newConnections.size(); i++)
		{
			_udpServer->Receive(receivePacket, _udpServer->newConnections[i].ip, _udpServer->newConnections[i].port);
		}

		_udpServer->Receive(receivePacket, _ip, _port);
	}
}

void WaitForACK(UDPHandler* handler)
{
	handler->isACKThreadOpen = true;
	while (applicationRunning && handler->idsToMessageIDs.size() > 0)
	{
		handler->WaitForACK();
	}
	handler->isACKThreadOpen = false;
}

void OpenGame(UDPClient* udpClient)
{
	Game game;
	game.clientGame = new ClientsGame;

	game.clientGame->client = udpClient;
	game.clientGame->game = &game;

	udpClient->myClientGame->client = udpClient;
	udpClient->myClientGame->game = &game;

	game.Run();
}

void Server()
{
	std::cout << "Server mode running" << std::endl;

	UDPServer udpServer;

	sf::Packet infoPacket;
	std::string sendMessage, receiveMessage;

	// Logic for receiving
	std::thread tcpScoketListen(OpenListener, &udpServer);
	tcpScoketListen.detach();

	udpServer.BindSocket();

	std::thread getLines(GetLineFromCin, &sendMessage);
	getLines.detach();

	while (applicationRunning)
	{
		if (udpServer.openACKThread && !udpServer.isACKThreadOpen)
		{
			udpServer.openACKThread = false;
			std::thread serverWaitForACK(WaitForACK, &udpServer);
			serverWaitForACK.detach();
		}
	}
}

void Client()
{
	std::cout << "Client mode running" << std::endl;
	
	UDPClient udpClient;

	sf::Packet infoPacket;
	std::string sendMessage, receiveMessage;

	udpClient.BindSocket();

	// Set server values
	udpClient.serverIP = IP;
	udpClient.shortServerPort = PORT;
	udpClient.myClientGame = new ClientsGame;

	// Logic for receiving
	std::thread udpSocketReceive(OpenReceiveThread, &udpClient);
	udpSocketReceive.detach();

	std::thread getLines(GetLineFromCin, &sendMessage);
	getLines.detach();

	// Open Game
	std::thread openGame(OpenGame, &udpClient);
	openGame.detach();

	while (applicationRunning)
	{
		if (udpClient.openACKThread && !udpClient.isACKThreadOpen)
		{
			udpClient.openACKThread = false;
			std::thread clientWaitForACK(WaitForACK, &udpClient);
			clientWaitForACK.detach();
		}
		
		if (udpClient.GetForceQuit())
		{
			applicationRunning = false;
		}
	}
}

void main()
{
	int server_mode;
	std::string mode_str;
	std::cout << "Select a mode: (1) server, (2) cliente" << std::endl;
	std::cin >> mode_str;
	server_mode = std::stoi(mode_str);

	if (server_mode == 1) 
	{
		Server();
	}
	else if (server_mode == 2)
	{
		Client();
	}
}