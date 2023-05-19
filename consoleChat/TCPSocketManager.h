#pragma once 
#include <SFML\Network.hpp>
#include <string>
#include <list>
#include <iostream>
#include <map>
#include <algorithm>

class TCPSocketManager
{
    std::list<sf::TcpSocket*> sockets;
    sf::TcpListener listener;
    sf::SocketSelector selector;
    std::vector<std::string> usernames;
    std::map<std::string, sf::TcpSocket*> users;

public:
    enum MessageTypes
    {
        LOGIN,
        MESSAGE,
        DISCONNECT,
        COUNT
    };

    sf::Socket::Status Listen(unsigned short port, sf::IpAddress ip);
    void ServerSend(std::string mssg, sf::TcpSocket& senderSocket);
    void ServerSendAll(std::string message);
    void ClientSend(sf::Packet infoPack);
    void ServerReceive(sf::Packet receivedPacket, sf::TcpSocket& senderSocket);
    void ClientReceive(std::string* mssg);
    sf::Socket::Status Connect(unsigned short port, sf::IpAddress ip);
    void ClientDisconected(std::string username, sf::TcpSocket& clientSocket);
    void Disconnect();
    void AddListener(unsigned short port);
    sf::TcpSocket* GetSocket();
};
