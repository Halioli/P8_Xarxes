#pragma once
#include <thread>
#include <chrono>
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include "entities.h"
#include "utils.h"
#include "UDPHandler.h"
#include "UDPClient.h"
#include "UDPServer.h"
class ClientsGame;

#define SIZE 10.f

class Game
{
	// GUI VARIABLES
	sf::RenderWindow window;
	sf::Texture characterTex;
	sf::Texture bg;
	sf::Sprite sprite;
	sf::String message;
	sf::Text text;
	sf::Text nameText;
	sf::Font font;
	sf::Event event;
	sf::RectangleShape nameRectangle;
	sf::String input;

	// GAME VARIABLES
	bool playing = false;
	bool shooting = false;
	sf::Vector2f cDir;
	Character character = Character(sf::Vector2f(10, 10), 1);
	std::vector<Bullet> bullets;  // Bullet container to manage them
	Character character2 = Character(sf::Vector2f(40, 40), 2);
	std::vector<Bullet> bullets2; // Bullet container to manage them

public:
	ClientsGame* clientGame;

	void SetUp();      // Initializing GUI
	void Run(); // Application loop
	void UpdateGame(); // No implemented => Implement it in the Server side

	void SetMessage(std::string mssg);
	std::string GetNameText();
};
