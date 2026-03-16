#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>

typedef enum
{
    STATE_MENU,
    STATE_GAME,
    STATE_GAME_OVER
} GameState;

typedef struct
{
    // window
    int screen_width;
    int screen_height;

    // font
    int font_margin;
    int font_normal_size;
    int font_large_size;

    // paddle
    Rectangle right_paddle;
    Rectangle left_paddle;
    int paddle_speed;
    int paddle_width;
    int paddle_height;

    // IA
    float left_paddle_reaction_timer;
    float left_paddle_reaction_delay;
    float left_paddle_target;
    bool left_paddle_has_prediction;

    // ball
    Vector2 ball_pos;
    Vector2 ball_dir;
    float ball_radius;
    float ball_speed;
    float ball_base_speed;
    float ball_max_speed;
    float ball_inc_speed;

    // ball animation
    float blink_timer;
    float blink_duration;
    float wait_start_timer;
    float wait_start_duration;
    float start_delay;
    int blink_count;
    bool ball_active;
    bool ball_visible;

    // states
    int left_paddle_lives;
    int right_paddle_lives;
    bool right_paddle_won;
    bool solo_mode;

    // global state
    GameState state;

    // audio
    Music music;
    bool music_playing;
    Sound sound_hit;

} Game;

void GameInit(Game *game, int screen_width, int screen_height);
void GameUpdate(Game *game);
void GameDraw(Game *game);
void GameUnload(Game *game);

#endif