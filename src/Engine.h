#pragma once
class Engine
{
public:
    static void setup();
    static void run();
    static void cleanup();
    static bool is_game_running();
    static void create_game();
};

