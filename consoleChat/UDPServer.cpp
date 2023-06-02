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
            if (newConnections[_id].lastMessageMode != MessageModes::CHALLENGE)
                ReceiveLogin(inPacket, remoteIP, shortPort);
            else
                SendAcknowledge(&socket, MessageModes::LOGIN, idValues, remoteIP, shortPort);
            break;

        case CHALLENGE:
            if (newConnections[_id].lastMessageMode != MessageModes::CHALLENGE_RESULT || clients[_id].lastMessageMode != MessageModes::CHALLENGE_RESULT)
                ReceiveChallengeResponse(_id, inPacket, remoteIP, shortPort);
            else
                SendAcknowledge(&socket, MessageModes::CHALLENGE_RESULT, idValues, remoteIP, shortPort);
            break;

        case MESSAGE:
            break;

        case ACK:
            ReceiveAcknowledge(_id, inPacket, remoteIP, shortPort);
            break;

        case JOIN_GAME:
            ReceiveJoinGame(_id, inPacket, remoteIP, shortPort);
            break;

        case CREATE_GAME:
            ReceiveCreateGame(_id, inPacket, remoteIP, shortPort);
            break;

        case UPDATE_CHARACTER:
            ReceiveUpdateCharacter(_id, inPacket, remoteIP, shortPort);
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
    float mssgRTT = (float)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - critMessages[idsToMessageIDs[id]].initialTimestamp).count();
    lastACKsTimeDifference.push_back(mssgRTT);
    mtx.unlock();

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
    case ENTER_GAME:
        std::cout << "ENTER_GAME ACK" << std::endl;
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

    // Get random challenge
    int challengeIndex = (rand() % challenges.size());

    // Create and save new connection
    NewConnection newConn;
    newConn.ip = remoteIP;
    newConn.port = _port;
    newConn.name = username;
    newConn.challenge = challenges[challengeIndex];
    newConn.solution = solutions[challengeIndex];
    newConn.lastMessageMode = MessageModes::CHALLENGE;

    newConnections[idValues] = newConn;
    
    // Packet and send challenge
    sf::Packet outPacket;
    outPacket << idValues << MessageModes::CHALLENGE << newConn.challenge;
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

    newConnections[id].lastMessageMode = MessageModes::CHALLENGE_RESULT;
    if (lowercaseMssg == newConnections[id].solution)
    {
        outPacket << true;
        
        Client newClient;
        newClient.clientID = id;
        newClient.ip = remoteIP;
        newClient.port = remotePort;
        newClient.name = newConnections[id].name;
        newClient.lastMessageMode = MessageModes::CHALLENGE_RESULT;
        newClient.clientCommandMessages.clear();

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

void UDPServer::ReceiveJoinGame(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort)
{
    SendAcknowledge(&socket, MessageModes::JOIN_GAME, id, remoteIP, remotePort);

    bool foundGame = true;
    sf::Packet outPacket;
    outPacket << id << MessageModes::ENTER_GAME;

    // Join/Create game (TODO)
    if (clientsMatches.size() < 0)
    {
        CreateGame(id);

        outPacket << !foundGame;
    }
    else
    {
        std::list<char> firstLetters;
        int diff = 100;
        int idToJoin = -1;

        // Find and join game
        for each (std::pair<int, Match> clientMatch in clientsMatches)
        {
            if (!clientMatch.second.isFull)
            {
                if (std::abs(clientMatch.second.creatorUsername[0] - clients[id].name[0]) < diff)
                {
                    idToJoin = clientMatch.first;
                }

                break;
            }
        }

        if (idToJoin != -1)
        {
            // Make client join a game
            clientsMatches[idToJoin].otherPlayerID = id;
            clientsMatches[idToJoin].isFull = true;

            clientsMatches[id].creatorUsername = clients[id].name;
            clientsMatches[id].game = clientsMatches[idToJoin].game;
            clientsMatches[id].otherPlayerID = idToJoin;
            clientsMatches[id].isFull = true;

            outPacket << foundGame;
        }
        else
        {
            std::cout << "No suitable game found" << std::endl;
            CreateGame(id);

            outPacket << !foundGame;
        }
    }

    // Send crit mssg to client
    Send(&socket, outPacket, remoteIP, remotePort);
    CriticalMessageSent(++lastMessageSentID, outPacket, &socket, remoteIP, remotePort);
}

void UDPServer::ReceiveCreateGame(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort)
{
    SendAcknowledge(&socket, MessageModes::CREATE_GAME, id, remoteIP, remotePort);

    sf::Packet outPacket;
    outPacket << id << MessageModes::ENTER_GAME << false;

    CreateGame(id);

    // Send crit mssg to client
    Send(&socket, outPacket, remoteIP, remotePort);
    CriticalMessageSent(++lastMessageSentID, outPacket, &socket, remoteIP, remotePort);
}

void UDPServer::ReceiveUpdateCharacter(int id, sf::Packet& inPacket, sf::IpAddress remoteIP, unsigned short& remotePort)
{
    int commandId;
    inPacket >> commandId;

    // Save client command to map
    mtx.lock();
    clients[id].clientCommandMessages[commandId] = inPacket;
    mtx.unlock();
}

void UDPServer::CalculateAverageRTT()
{
    while (isRunning)
    {
        float currentValue = 0.f;
        float result = 0.f;

        std::this_thread::sleep_for(std::chrono::seconds(2));

        mtx.lock();
        if (lastACKsTimeDifference.size() > 0)
        {
            // Clear old times & calculate avrg if needed
            while (lastACKsTimeDifference.size() > 10)
            {
                lastACKsTimeDifference.pop_front();
            }

            // Calculate avrg
            for each (float timeDifference in lastACKsTimeDifference)
            {
                currentValue += timeDifference;
            }
            result = currentValue / lastACKsTimeDifference.size();

            std::cout << "Current RTT: " << result << std::endl;
        }
        mtx.unlock();
    }
}

void UDPServer::CreateGame(int id)
{
    // Create game
    activeGames.push_back(new Game); // REMEMBER TO CLEAR :^)

    clientsMatches[id].creatorUsername = clients[id].name;
    clientsMatches[id].game = activeGames.back();
}

void UDPServer::ProcessReceivedCommands()
{
    float dirX, dirY;

    sf::Vector2f newPlayerPos;

    while (isRunning)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Check every received command in every client
        mtx.lock();
        for each (std::pair<int, Client> client in clients)
        {
            // Check if client is in match
            if (clientsMatches.find(client.first) != clientsMatches.end())
            {
                for each (std::pair<int, sf::Packet> cmndMssg in client.second.clientCommandMessages)
                {
                    cmndMssg.second >> dirX >> dirY;
                    
                    // Validate move
                    newPlayerPos = clientsMatches[client.first].game->GetPlayerPosition();
                }
                client.second.clientCommandMessages.clear();
            }
        }
        mtx.unlock();
    }
}

bool UDPServer::GetIsRunning()
{
    return isRunning;
}
