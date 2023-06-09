#pragma once

enum MessageModes
{
    LOGIN,
    CHALLENGE,
    CHALLENGE_RESULT,
    MESSAGE,
    ACK,
    GAME_SELECTION,
    JOIN_GAME, 
    CREATE_GAME,
    ENTER_GAME,
    UPDATE_CHARACTER,
    UPDATE_LOCAL_GAME,
    UPDATE_REMOTE_GAME,
    DISCONNECT
};

enum CommandType
{
    MOVE_UP,
    MOVE_RIGHT,
    MOVE_DOWN,
    MOVE_LEFT,
    SHOOT
};