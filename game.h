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
    int font_size;

    // paddle
    Rectangle player;
    Rectangle enemy;

    int speed_paddle;
    int paddle_width;
    int paddle_height;

    // IA
    float enemy_reaction_timer;
    float enemy_reaction_delay;
    float enemy_target;
    bool enemy_has_prediction;

    // ball
    Vector2 ball_pos;
    Vector2 ball_dir;

    float ball_radius;
    float speed_ball;
    float base_ball_speed;
    float max_ball_speed;

    float blink_timer;
    float wait_timer;
    float start_delay;
    int blink_count;
    bool ball_active;
    bool ball_visible;

    // states
    int enemy_lives;
    int player_lives;
    bool player_won;
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