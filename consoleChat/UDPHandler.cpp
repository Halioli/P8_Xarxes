#include "UDPHandler.h"

void UDPHandler::Send(sf::UdpSocket* socket, sf::Packet packet, sf::IpAddress IP, int port)
{
    if (socket->send(packet, IP, port) != sf::Socket::Done)
    {
        std::cout << "Error sending packet to " << IP << std::endl;
    }
}


int UDPHandler::GetLastMessageSent()
{
    return lastMessageSentID;
}

void UDPHandler::CriticalMessageSent(int mssgID, sf::Packet packet, sf::UdpSocket* socket, sf::IpAddress IP, int port)
{
    sf::Packet _packet;
    int targetID;
    _packet = packet;
    _packet >> targetID;
    _packet.clear();

    openACKThread = true;
    idsToMessageIDs[targetID] = mssgID;
    critMessages[mssgID] = { std::chrono::system_clock::now(), packet, socket, IP, port };
}

void UDPHandler::SendAcknowledge(sf::UdpSocket* socket, int ackMssgType, int _id, sf::IpAddress& remoteIP, unsigned short& remotePort)
{
    sf::Packet packet;
    packet << _id << MessageModes::ACK << ackMssgType;

    Send(socket, packet, remoteIP, remotePort);
}

void UDPHandler::WaitForACK(int mssgID)
{
    for each (std::pair<int, CriticalMessage> critMssg in critMessages)
    {
        // if one second passed
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - critMessages[mssgID].timestamp).count() >= 1)
        {
            // resend mssg
            std::cout << "Resend Packet" << std::endl;
            critMessages[mssgID].timestamp = std::chrono::system_clock::now();

            Send(critMessages[mssgID].socket, critMessages[mssgID].mssgPacket, critMessages[mssgID].IP, critMessages[mssgID].port);
        }
    }
}
