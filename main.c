#include "raylib.h"
#include "game.h"

int main(void)
{
    const int screen_width = 1280;
    const int screen_height = 720;

    InitWindow(screen_width, screen_height, "Pong Game");
    InitAudioDevice();

    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    Game game;
    GameInit(&game, screen_width, screen_height);

    while (!WindowShouldClose())
    {
        GameUpdate(&game);

        BeginDrawing();
        ClearBackground(BLACK);
        GameDraw(&game);
        EndDrawing();
    }

    GameUnload(&game);

    CloseAudioDevice();
    CloseWindow();

    return 0;
}

/*
cc main.c game.c pcg32.c -o game $(pkg-config --cflags --libs raylib) -lm
./game
*/