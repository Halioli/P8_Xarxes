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
        newClient.clientCommands.clear();

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
    Command tempCommand;
    int commandSize;
    inPacket >> commandSize;

    int commandId;
    int commandType;
    sf::Vector2f newPos;

    // Save client command to map
    for (int i = 0; i < commandSize; i++)
    {
        inPacket >> commandId >> commandType >> newPos.x >> newPos.y;
        
        tempCommand.cmndId = commandId;
        tempCommand.cmndType = commandType;
        tempCommand.newPos = newPos;

        mtx.lock();
        clients[id].clientCommands[commandId] = tempCommand;
        mtx.unlock();
    }
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

    mtx.lock();
    clientsMatches[id].creatorUsername = clients[id].name;
    clientsMatches[id].game = activeGames.back();
    mtx.unlock();
}

void UDPServer::ProcessReceivedCommands()
{
    while (isRunning)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Check every received command in every client
        for (auto it = clients.begin(); it != clients.end(); ++it)
        {
            mtx.lock();
            // Check if client is in match
            if (clientsMatches.find(it->first) != clientsMatches.end())
            {
                IterateAndValidateCommandMessages(it);
            }
            mtx.unlock();
        }
    }
}

void UDPServer::IterateAndValidateCommandMessages(std::map<int, Client>::iterator it)
{
    int commandType;
    sf::Vector2f newPlayerPos;
    sf::Vector2f currentServerPos;
    sf::Vector2f moveDirection;

    sf::Packet outPacket;
    int lastValidCommandId = -1;

    if (it->second.clientCommands.size() <= 0)
    {
        return;
    }

    for each (std::pair<int, Command> cmndMssg in it->second.clientCommands)
    {
        commandType = cmndMssg.second.cmndType;
        newPlayerPos = cmndMssg.second.newPos;

        currentServerPos = clientsMatches[it->first].game->GetPlayerPosition();

        switch (commandType)
        {
        case MOVE_UP:
            moveDirection = sf::Vector2f(0, -1);
            break;
        case MOVE_RIGHT:
            moveDirection = sf::Vector2f(1, 0);
            break;
        case MOVE_DOWN:
            moveDirection = sf::Vector2f(0, 1);
            break;
        case MOVE_LEFT:
            moveDirection = sf::Vector2f(-1, 0);
            break;
        case SHOOT:
            break;
        default:
            break;
        }
        currentServerPos += moveDirection;

        if (currentServerPos == newPlayerPos)
        {
            // Valid movement
            clientsMatches[it->first].game->SetPlayerPosition(newPlayerPos);
            lastValidCommandId = cmndMssg.first;
        }
    }

    // Update local client
    outPacket << it->second.clientID << MessageModes::UPDATE_LOCAL_GAME
        << lastValidCommandId << it->second.clientCommands[it->second.clientCommands.size()].cmndId << it->second.clientCommands[0].cmndId;
    unsigned short shortPort = it->second.port;
    Send(&socket, outPacket, it->second.ip, shortPort);
    
    // Clear
    outPacket.clear();

    // Update remote client if they exist
    if (clientsMatches[it->first].otherPlayerID != -1)
    {
        if (lastValidCommandId != -1)
        {
            // Update the entity interpolation logic with the last player's position
            outPacket << clientsMatches[it->first].otherPlayerID << MessageModes::UPDATE_REMOTE_GAME
                << it->second.clientCommands[lastValidCommandId].newPos.x << it->second.clientCommands[lastValidCommandId].newPos.y;
        }
        else
        {
            // If they receive -1 / -1 entity interpolation should stay still
            outPacket << clientsMatches[it->first].otherPlayerID << MessageModes::UPDATE_REMOTE_GAME << -1 << -1;
        }
        shortPort = clients[clientsMatches[it->first].otherPlayerID].port;
        Send(&socket, outPacket, clients[clientsMatches[it->first].otherPlayerID].ip, shortPort);
    }

    // Clear
    it->second.clientCommands.clear();
}

bool UDPServer::GetIsRunning()
{
    return isRunning;
}
