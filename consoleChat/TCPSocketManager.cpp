#include "TCPSocketManager.h"

sf::Socket::Status TCPSocketManager::Listen(unsigned short port, sf::IpAddress ip)
{
    // Make the selector wait for data on any socket
    if (selector.wait())
    {
        // Test the listener
        if (selector.isReady(listener))
        {
            // The listener is ready: there is a pending connection
            sf::TcpSocket* client = new sf::TcpSocket;
            if (listener.accept(*client) == sf::Socket::Done)
            {
                // Add the new client to the clients list
                sockets.push_back(client);

                // Add the new client to the selector so that we will
                // be notified when he sends something
                selector.add(*client);

                std::cout << "Added client" << std::endl;
            }
            else
            {
                // Error, we won't get a new connection, delete the socket
                delete client;
            }
        }
        else
        {
            // The listener socket is not ready, test all other sockets (the clients)
            for (std::list<sf::TcpSocket*>::iterator it = sockets.begin(); it != sockets.end(); ++it)
            {
                sf::TcpSocket& client = **it;
                if (selector.isReady(client))
                {
                    // The client has sent some data, we can receive it
                    sf::Packet packet;
                    if (client.receive(packet) == sf::Socket::Done)
                    {
                        ServerReceive(packet, client);
                    }
                }
            }
        }
    }

    return sf::Socket::Done;
}


void TCPSocketManager::ServerSend(std::string message, sf::TcpSocket& senderSocket)
{
    sf::Packet packet;
    packet << message;

    for each (sf::TcpSocket* targetSocket in sockets)
    {
        if (targetSocket != &senderSocket)
        {
            sf::Socket::Status status = targetSocket->send(packet);
            std::cout << "Message sent: " << message << std::endl;
            if (status != sf::Socket::Status::Done)
            {
                // Error when sending data
                std::cout << "Error sending message" << std::endl;
                return;
            }
        }
    }

    packet.clear();
}

void TCPSocketManager::ServerSendAll(std::string message)
{
    sf::Packet packet;
    packet << message;

    if (message == "exit")
    {
        ServerSendAll("DISCONNECT");
        Disconnect();
    }

    for each (sf::TcpSocket * targetSocket in sockets)
    {
        sf::Socket::Status status = targetSocket->send(packet);
        std::cout << "Message sent: " << message << std::endl;
        if (status != sf::Socket::Status::Done)
        {
            // Error when sending data
            std::cout << "Error sending message" << std::endl;
            return;
        }
    }

    packet.clear();
}

void TCPSocketManager::ClientSend(sf::Packet infoPack)
{
    for each (sf::TcpSocket* targetSocket in sockets)
    {
        sf::Socket::Status status = targetSocket->send(infoPack);
        if (status != sf::Socket::Status::Done)
        {
            // Error when sending data
            std::cout << "Error sending message" << std::endl;
            return;
        }
    }

    infoPack.clear();
}

void TCPSocketManager::ServerReceive(sf::Packet receivedPacket, sf::TcpSocket& senderSocket)
{
    //auto findResult;
    int tempMode;
    std::string tempUsername;
    std::string tempMssg;
    receivedPacket >> tempMode >> tempUsername >> tempMssg;

    switch (tempMode)
    {
    case TCPSocketManager::LOGIN:
        usernames.push_back(tempUsername);
        std::cout << "New username: " << tempUsername << std::endl;
        break;
    case TCPSocketManager::MESSAGE:
        if (tempMssg.size() > 0)
        {
            if (tempMssg == "exit")
            {
                ClientDisconected(tempUsername, senderSocket);
                return;
            }
            std::cout << "Received message: " << tempMssg << std::endl;

            // Send received message to all clients
            ServerSend(tempUsername + " - " + tempMssg, senderSocket);
        }
        break;
    case TCPSocketManager::DISCONNECT:
        ClientDisconected(tempUsername, senderSocket);
        break;
    default:
        break;
    }
}

void TCPSocketManager::ClientReceive(std::string* mssg)
{
    sf::Packet packet;
    sf::TcpSocket* socket = *sockets.begin();

    sf::Socket::Status status = socket->receive(packet);
    if (status != sf::Socket::Status::Done)
    {
        std::cout << "Error receiving message" << std::endl;
        return;
    }

    std::string masaje;
    packet >> masaje;

    // Se procesaelmensaje
    if (masaje.size() > 0)
    {
        if (masaje == "DISCONNECT")
        {
            // Manages the desconection
            Disconnect();
        }
        std::cout << "Received message: " << masaje << std::endl;
        mssg->assign(masaje);
    }
}

sf::Socket::Status TCPSocketManager::Connect(unsigned short port, sf::IpAddress ip)
{
    sf::TcpSocket* server = new sf::TcpSocket;
    sockets.push_back(server);

    sf::Socket::Status status = (*sockets.begin())->connect(ip, port, sf::seconds(5.f));
    if (status != sf::Socket::Done)
    {
        //No se ha podido conectar
        std::cout << "Error al conectarse con el servidor" << std::endl;
    }

    return status;
}

void TCPSocketManager::ClientDisconected(std::string username, sf::TcpSocket& clientSocket)
{
    auto findUsername = std::find(usernames.begin(), usernames.end(), username);
    if (findUsername != usernames.end())
    {
        usernames.erase(findUsername);
    }

    sockets.remove(&clientSocket); // <-- Peta

    std::cout << username << " disconected" << std::endl;
}

void TCPSocketManager::Disconnect()
{
    listener.close();

    for each (sf::TcpSocket* socket in sockets)
    {
        socket->disconnect();
    }
}

void TCPSocketManager::AddListener(unsigned short port)
{
    listener.listen(port);
    selector.add(listener);
}

sf::TcpSocket* TCPSocketManager::GetSocket()
{
    return *sockets.begin();
}
