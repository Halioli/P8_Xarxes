#include "UDPHandler.h"

void UDPHandler::Send(sf::UdpSocket* socket, sf::Packet packet, sf::IpAddress IP, unsigned short& port)
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
    mtx.lock();
    idsToMessageIDs[targetID] = mssgID;
    critMessages[mssgID] = { std::chrono::system_clock::now(), packet, socket, IP, port };
    mtx.unlock();
}

void UDPHandler::SendAcknowledge(sf::UdpSocket* socket, int ackMssgType, int _id, sf::IpAddress remoteIP, unsigned short& remotePort)
{
    sf::Packet packet;
    packet << _id << MessageModes::ACK << ackMssgType;

    Send(socket, packet, remoteIP, remotePort);
}

void UDPHandler::WaitForACK()
{
    mtx.lock();
    for each (std::pair<int, CriticalMessage> critMssg in critMessages)
    {
        // if one second passed
        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - critMssg.second.timestamp).count() >= 0.5f)
        {
            // resend mssg
            std::cout << "Resend Packet" << std::endl;
            critMssg.second.timestamp = std::chrono::system_clock::now();

            unsigned short _port = critMssg.second.port;
            Send(critMssg.second.socket, critMssg.second.mssgPacket, critMssg.second.IP, _port);
        }
    }
    mtx.unlock();
}
