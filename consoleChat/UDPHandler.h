#pragma once
#include <SFML\Network.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <list>
#include <mutex>

#include "MessageModes.h"

class UDPHandler
{
protected:
    struct CriticalMessage
    {
        std::chrono::system_clock::time_point timestamp;
        sf::Packet mssgPacket;
        sf::UdpSocket* socket;
        sf::IpAddress IP;
        int port;
    };

	int lastMessageSentID = 0;
    std::mutex mtx;

public:
	void Send(sf::UdpSocket* socket, sf::Packet packet, sf::IpAddress IP, unsigned short& port);
	int GetLastMessageSent();

    float packetLostProbablity = 0.0f;
    //
    bool openACKThread;
    bool waitForACK;
    bool isACKThreadOpen;
    std::map<int, CriticalMessage> critMessages;
    std::map<int, int> idsToMessageIDs;

    void SendAcknowledge(sf::UdpSocket* socket, int ackMssgType, int _id, sf::IpAddress remoteIP, unsigned short& remotePort);
    void CriticalMessageSent(int mssgID, sf::Packet packet, sf::UdpSocket* socket, sf::IpAddress IP, int port);
    void WaitForACK();
};

