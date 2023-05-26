#include "UDPServer.h"

void UDPServer::BindSocket()
{
    if (socket.bind(PORT) != sf::Socket::Done)
    {
        std::cout << "Error binding socket" << std::endl;
    }
}

void UDPServer::Receive(sf::Packet& inPacket, sf::IpAddress remoteIP, int& remotePort)
{
    unsigned short shortPort = remotePort;
    int _id;
    int _mssgMode;

    if (socket.receive(inPacket, remoteIP, shortPort) != sf::Socket::Done)
    {
        std::cout << "Error receiving packet from: " << remoteIP << " on port " << shortPort << std::endl;
    }
    else
    {
        std::cout << "Received packet from " << remoteIP << " on port " << remotePort << std::endl;

        inPacket >> _id >> _mssgMode;

        if (_mssgMode != MessageModes::LOGIN && (clients.find(_id) == clients.end() && newConnections.find(_id) == newConnections.end()))
            return;

        switch (_mssgMode)
        {
        case LOGIN:
            ReceiveLogin(inPacket, remoteIP, shortPort);
            break;

        case CHALLENGE:
            ReceiveChallengeResponse(_id, inPacket, remoteIP, shortPort);
            break;

        case MESSAGE:
            break;

        case ACK:
            ReceiveAcknowledge(_id, inPacket, remoteIP, shortPort);
            break;

        case DISCONNECT:
            break;
        default:
            break;
        }
    }
}

void UDPServer::ReceiveAcknowledge(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort)
{
    mtx.lock();
    critMessages.erase(idsToMessageIDs[id]);
    idsToMessageIDs.erase(id);
    mtx.unlock();

    int _ack;
    inPacket >> _ack;

    switch (_ack)
    {
    case LOGIN:
        std::cout << "LOGIN ACK" << std::endl;
        break;
    case CHALLENGE:
        std::cout << "CHALLENGE ACK" << std::endl;
        break;
    case CHALLENGE_RESULT:
        std::cout << "CHALLENGE_RESULT ACK" << std::endl;
        break;
    case DISCONNECT:
        break;
    default:
        break;
    }
}

void UDPServer::ReceiveLogin(sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort)
{
    ++idValues;
    SendAcknowledge(&socket, MessageModes::LOGIN, idValues, remoteIP, remotePort);

    std::string username;
    int _port;
    inPacket >> _port >> username;

    // Create challenge
    std::string challenge = "Name this animal:\n\n\n\n^..^       /\n/_/ \\_____/\n    /\\  / \\\n   /  \\/   \\";
    //     ^..^       /
    //     /_/ \_____/
    //         /\  / \
    //        /  \/   \ 

    // Create and save new connection
    NewConnection newConn;
    newConn.ip = remoteIP;
    newConn.port = _port;
    newConn.name = username;
    newConn.challenge = challenge;
    newConn.solution = "dog";

    newConnections[idValues] = newConn;
    
    // Packet and send challenge
    sf::Packet outPacket;
    outPacket << idValues << MessageModes::CHALLENGE << challenge;
    Send(&socket, outPacket, remoteIP, remotePort);
    CriticalMessageSent(++lastMessageSentID, outPacket, &socket, remoteIP, remotePort);
}

void UDPServer::ReceiveChallengeResponse(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort)
{
    SendAcknowledge(&socket, MessageModes::CHALLENGE_RESULT, id, remoteIP, remotePort);

    std::string response;
    sf::Packet outPacket;
    outPacket << id << MessageModes::CHALLENGE_RESULT;

    inPacket >> response;

    std::string lowercaseMssg;
    for (int i = 0; i < response.length(); i++)
    {
        lowercaseMssg.append(1, std::tolower(response[i]));
    }

    if (lowercaseMssg == newConnections[id].solution)
    {
        outPacket << true;
        
        Client newClient;
        newClient.clientID = id;
        newClient.ip = remoteIP;
        newClient.port = remotePort;
        newClient.name = newConnections[id].name;

        clients[id] = newClient;
        newConnections.erase(id);
    }
    else
    {
        outPacket << false;

        newConnections.erase(id);
    }

    Send(&socket, outPacket, remoteIP, remotePort);
    CriticalMessageSent(++lastMessageSentID, outPacket, &socket, remoteIP, remotePort);
}

void UDPServer::ReceiveMessage(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort)
{
    std::string mssg;
    inPacket >> mssg;

    std::cout << clients[id].name << ": " << mssg << std::endl;
}
