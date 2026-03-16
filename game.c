#include "game.h"
#include "pcg32.h"
#include <math.h>

static void GameResetBallDirection(Game *game)
{
    float angle = (float)pcg_range_int(90) - 45.0f;
    angle *= DEG2RAD;

    game->ball_dir.x = cosf(angle);
    game->ball_dir.y = sinf(angle);

    if (pcg_range_int(2) == 0)
    {
        game->ball_dir.x *= -1;
    }
}

static void GameResetBallAndRoundState(Game *game)
{
    game->ball_active = false;
    game->blink_timer = 0.0f;
    game->wait_start_timer = 0.0f;
    game->blink_count = 0;
    game->start_delay = 0.0f;
    game->ball_visible = true;

    game->ball_pos = (Vector2)
    {
        game->screen_width / 2.0f,
        game->screen_height / 2.0f
    };

    game->ball_speed = game->ball_base_speed;

    GameResetBallDirection(game);
}

static void GameResetFullMatch(Game *game)
{
    game->left_paddle_lives = 3;
    game->right_paddle_lives = 3;
    game->right_paddle_won = false;

    game->right_paddle.y = game->screen_height / 2.0f - game->paddle_height / 2.0f;
    game->left_paddle.y = game->screen_height / 2.0f - game->paddle_height / 2.0f;

    game->left_paddle_target = game->screen_height / 2.0f - game->left_paddle.height / 2.0f;
    game->left_paddle_reaction_timer = 0.0f;
    game->left_paddle_reaction_delay = 0.0f;
    game->left_paddle_has_prediction = false;

    GameResetBallAndRoundState(game);
}

static void ClampPaddles(Game *game)
{
    if (game->right_paddle.y < 0)
        game->right_paddle.y = 0;
    else if (game->right_paddle.y > game->screen_height - game->right_paddle.height)
        game->right_paddle.y = game->screen_height - game->right_paddle.height;

    if (game->left_paddle.y < 0)
        game->left_paddle.y = 0;
    else if (game->left_paddle.y > game->screen_height - game->left_paddle.height)
        game->left_paddle.y = game->screen_height - game->left_paddle.height;
}

static void UpdateBallStartSequence(Game *game, float dt)
{
    if (!game->ball_active)
    {
        if (game->start_delay < 1.0f)
        {
            game->start_delay += dt;
        }
        else
        {
            game->blink_timer += dt;

            if (game->blink_count < 6)
            {
                if (game->blink_timer > game->blink_duration)
                {
                    game->ball_visible = !game->ball_visible;
                    game->blink_timer = 0.0f;
                    game->blink_count++;
                }
            }
            else
            {
                game->wait_start_timer += dt;

                if (game->wait_start_timer > game->wait_start_duration)
                {
                    game->ball_active = true;
                    game->ball_visible = true;
                }
            }
        }
    }
}

static void UpdatePlayerInput(Game *game, float dt)
{
    if (game->start_delay > 1.0f || game->left_paddle_lives < 3 || game->right_paddle_lives < 3)
    {
        if (game->solo_mode)
        {
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
                game->right_paddle.y -= game->paddle_speed * dt;

            if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
                game->right_paddle.y += game->paddle_speed * dt;
        }
        else
        {
            if (IsKeyDown(KEY_UP))
                game->right_paddle.y -= game->paddle_speed * dt;

            if (IsKeyDown(KEY_DOWN))
                game->right_paddle.y += game->paddle_speed * dt;
        }
    }
}

static void UpdateEnemyInputTwoPlayers(Game *game, float dt)
{
    if (game->start_delay > 1.0f || game->left_paddle_lives < 3 || game->right_paddle_lives < 3)
    {
        if (IsKeyDown(KEY_W))
            game->left_paddle.y -= game->paddle_speed * dt;
        else if (IsKeyDown(KEY_S))
            game->left_paddle.y += game->paddle_speed * dt;
    }
}

static void UpdateEnemyAI(Game *game, float dt)
{
    if (game->ball_dir.x > 0 && game->left_paddle_has_prediction)
    {
        game->left_paddle_has_prediction = false;
        game->left_paddle_target = game->screen_height / 2.0f - game->left_paddle.height / 2.0f;

        game->left_paddle_reaction_delay = (float)(pcg_range_int(200) + 80) / 1000.0f;
        game->left_paddle_reaction_timer = 0.0f;
    }

    if (game->ball_dir.x < 0 && !game->left_paddle_has_prediction && game->ball_pos.x < game->screen_width / 2.0f)
    {
        game->left_paddle_reaction_timer += dt;

        if (game->left_paddle_reaction_timer >= game->left_paddle_reaction_delay)
        {
            float vx = game->ball_dir.x * game->ball_speed;

            if (fabsf(vx) > 0.01f)
            {
                float time = (game->left_paddle.x - game->ball_pos.x) / vx;

                if (time > 0.0f)
                {
                    float predicted_y = game->ball_pos.y + game->ball_dir.y * game->ball_speed * time;

                    while (predicted_y < 0 || predicted_y > game->screen_height)
                    {
                        if (predicted_y < 0)
                            predicted_y = -predicted_y;

                        if (predicted_y > game->screen_height)
                            predicted_y = 2.0f * game->screen_height - predicted_y;
                    }

                    game->left_paddle_target = predicted_y - game->left_paddle.height / 2.0f;

                    game->left_paddle_target += (float)(pcg_range_int(200) - 100);

                    if (game->left_paddle_target < 0)
                        game->left_paddle_target = 0;

                    if (game->left_paddle_target > game->screen_height - game->left_paddle.height)
                        game->left_paddle_target = game->screen_height - game->left_paddle.height;

                    game->left_paddle_has_prediction = true;
                }
            }
        }
    }

    float move = game->paddle_speed * dt;
    float dead_zone = 10.0f;

    if (game->left_paddle.y < game->left_paddle_target - dead_zone)
    {
        game->left_paddle.y += move;
    }
    else if (game->left_paddle.y > game->left_paddle_target + dead_zone)
    {
        game->left_paddle.y -= move;
    }
}

static void UpdateBallMovement(Game *game, float dt)
{
    if (game->ball_active)
    {
        game->ball_pos.x += game->ball_dir.x * game->ball_speed * dt;
        game->ball_pos.y += game->ball_dir.y * game->ball_speed * dt;
    }
}

static void HandleBallWallCollisions(Game *game)
{
    if (game->ball_pos.y - game->ball_radius <= 0)
    {
        game->ball_pos.y = game->ball_radius;
        game->ball_dir.y *= -1;
    }

    if (game->ball_pos.y + game->ball_radius >= game->screen_height)
    {
        game->ball_pos.y = game->screen_height - game->ball_radius;
        game->ball_dir.y *= -1;
    }
}

static void HandlePaddleCollisionPlayer(Game *game)
{
    if (game->ball_dir.x > 0 && CheckCollisionCircleRec(game->ball_pos, game->ball_radius, game->right_paddle))
    {
        PlaySound(game->sound_hit);

        float hit_pos = game->ball_pos.y - game->right_paddle.y;
        float normalized = (hit_pos / game->right_paddle.height) * 2.0f - 1.0f;
        float max_angle = 60.0f * DEG2RAD;
        float angle = normalized * max_angle;

        game->ball_dir.x = -cosf(angle);
        game->ball_dir.y = sinf(angle);

        game->ball_pos.x = game->right_paddle.x - game->ball_radius;

        game->ball_speed += game->ball_inc_speed;
        if (game->ball_speed > game->ball_max_speed)
            game->ball_speed = game->ball_max_speed;
    }
}

static void HandlePaddleCollisionEnemy(Game *game)
{
    if (game->ball_dir.x < 0 && CheckCollisionCircleRec(game->ball_pos, game->ball_radius, game->left_paddle))
    {
        PlaySound(game->sound_hit);

        float hit_pos = game->ball_pos.y - game->left_paddle.y;
        float normalized = (hit_pos / game->left_paddle.height) * 2.0f - 1.0f;
        float max_angle = 60.0f * DEG2RAD;
        float angle = normalized * max_angle;

        game->ball_dir.x = cosf(angle);
        game->ball_dir.y = sinf(angle);

        game->ball_pos.x = game->left_paddle.x + game->left_paddle.width + game->ball_radius;

        game->ball_speed += game->ball_inc_speed;
        if (game->ball_speed > game->ball_max_speed)
            game->ball_speed = game->ball_max_speed;
    }
}

static void HandleScoreAndRoundReset(Game *game)
{
    if (game->ball_pos.x <= game->ball_radius)
    {
        game->left_paddle_lives--;

        if (game->left_paddle_lives <= 0)
        {
            game->state = STATE_GAME_OVER;
            game->right_paddle_won = true;
        }

        GameResetBallAndRoundState(game);

        game->left_paddle_target = game->screen_height / 2.0f - game->left_paddle.height / 2.0f;
        game->left_paddle_reaction_timer = 0.0f;
        game->left_paddle_reaction_delay = 0.0f;
        game->left_paddle_has_prediction = false;
    }

    if (game->ball_pos.x >= game->screen_width - game->ball_radius)
    {
        game->right_paddle_lives--;

        if (game->right_paddle_lives <= 0)
        {
            game->state = STATE_GAME_OVER;
            game->right_paddle_won = false;
        }

        GameResetBallAndRoundState(game);

        game->left_paddle_target = game->screen_height / 2.0f - game->left_paddle.height / 2.0f;
        game->left_paddle_reaction_timer = 0.0f;
        game->left_paddle_reaction_delay = 0.0f;
        game->left_paddle_has_prediction = false;
    }
}

void GameInit(Game *game, int screen_width, int screen_height)
{
    pcg_init();

    game->screen_width = screen_width;
    game->screen_height = screen_height;

    game->paddle_speed = 700;
    game->paddle_width = 20;
    game->paddle_height = 140;

    game->ball_radius = 12.5f;
    game->ball_base_speed = 700.0f;
    game->ball_max_speed = 1500.0f;
    game->ball_inc_speed = 50.0f;

    game->font_margin = 200;
    game->font_normal_size = 30;
    game->font_large_size = 80;

    game->left_paddle_reaction_timer = 0.0f;
    game->left_paddle_reaction_delay = 0.0f;
    game->left_paddle_target = screen_height / 2.0f - game->paddle_height / 2.0f;
    game->left_paddle_has_prediction = false;

    game->ball_speed = game->ball_base_speed;
    game->blink_timer = 0.0f;
    game->blink_duration = 0.15f;
    game->wait_start_timer = 0.0f;
    game->wait_start_duration = 1.0f;
    game->start_delay = 0.0f;
    game->blink_count = 0;
    game->ball_active = false;
    game->ball_visible = true;

    game->left_paddle_lives = 3;
    game->right_paddle_lives = 3;
    game->right_paddle_won = false;
    game->solo_mode = false;

    game->music = LoadMusicStream("sounds/retro_music.wav");
    game->music_playing = false;
    game->sound_hit = LoadSound("sounds/hit_sound.wav");

    game->state = STATE_MENU;

    game->right_paddle = (Rectangle)
    {
        screen_width - game->paddle_width - 100,
        screen_height / 2.0f - game->paddle_height / 2.0f,
        (float)game->paddle_width,
        (float)game->paddle_height
    };

    game->left_paddle = (Rectangle)
    {
        100,
        screen_height / 2.0f - game->paddle_height / 2.0f,
        (float)game->paddle_width,
        (float)game->paddle_height
    };

    game->ball_pos = (Vector2)
    {
        screen_width / 2.0f,
        screen_height / 2.0f
    };

    GameResetBallDirection(game);
}

void GameUpdate(Game *game)
{
    float dt = GetFrameTime();

    switch (game->state)
    {
        case STATE_MENU:
        {
            if (IsKeyPressed(KEY_X))
            {
                game->solo_mode = true;
                GameResetFullMatch(game);
                game->state = STATE_GAME;
            }
            else if (IsKeyPressed(KEY_Y))
            {
                game->solo_mode = false;
                GameResetFullMatch(game);
                game->state = STATE_GAME;
            }

        } break;

        case STATE_GAME:
        {
            if (IsKeyPressed(KEY_ESCAPE))
            {
                GameResetFullMatch(game);

                StopMusicStream(game->music);
                game->music_playing = false;

                game->state = STATE_MENU;
                break;
            }

            if (!game->music_playing)
            {
                PlayMusicStream(game->music);
                game->music_playing = true;
            }

            UpdateMusicStream(game->music);

            UpdatePlayerInput(game, dt);

            if (game->solo_mode)
                UpdateEnemyAI(game, dt);
            else
                UpdateEnemyInputTwoPlayers(game, dt);

            ClampPaddles(game);

            UpdateBallStartSequence(game, dt);
            UpdateBallMovement(game, dt);

            HandlePaddleCollisionPlayer(game);
            HandlePaddleCollisionEnemy(game);

            HandleBallWallCollisions(game);
            HandleScoreAndRoundReset(game);

        } break;

        case STATE_GAME_OVER:
        {
            StopMusicStream(game->music);
            game->music_playing = false;

            if (IsKeyPressed(KEY_X))
            {
                GameResetFullMatch(game);
                game->state = STATE_GAME;
            }

            if (IsKeyPressed(KEY_Y))
            {
                GameResetFullMatch(game);
                game->state = STATE_MENU;
            }

        } break;
    }
}

void GameDraw(Game *game)
{
    switch (game->state)
    {
        case STATE_MENU:
        {
            const char *text_solo = "PRESS X FOR SOLO";
            int text_solo_width = MeasureText(text_solo, game->font_normal_size);

            DrawText(text_solo,
                     game->screen_width / 2 - text_solo_width / 2,
                     game->screen_height / 2 - 30,
                     game->font_normal_size,
                     RAYWHITE);

            const char *text_multi = "PRESS Y FOR TWO PLAYERS";
            int text_multi_width = MeasureText(text_multi, game->font_normal_size);

            DrawText(text_multi,
                     game->screen_width / 2 - text_multi_width / 2,
                     game->screen_height / 2 + 30,
                     game->font_normal_size,
                     RAYWHITE);

        } break;

        case STATE_GAME:
        {
            DrawRectangleRec(game->right_paddle, RAYWHITE);
            DrawRectangleRec(game->left_paddle, RAYWHITE);

            if (game->ball_visible)
            {
                DrawCircleV(game->ball_pos, game->ball_radius, RAYWHITE);
            }

            const char *enemy_text = TextFormat("LIVES: %d", game->left_paddle_lives);
            DrawText(enemy_text, game->font_margin, 30, game->font_normal_size, RAYWHITE);

            const char *pl_text = TextFormat("LIVES: %d", game->right_paddle_lives);
            int pl_width = MeasureText(pl_text, game->font_normal_size);
            int pl_x = game->screen_width - pl_width - game->font_margin;

            DrawText(pl_text, pl_x, 30, game->font_normal_size, RAYWHITE);

        } break;

        case STATE_GAME_OVER:
        {
            Color result_color;
            const char *result_text;

            if (game->solo_mode)
            {
                if (game->right_paddle_won)
                {
                    result_text = "YOU WIN";
                    result_color = GREEN;
                }
                else
                {
                    result_text = "GAME OVER";
                    result_color = RED;
                }
            }
            else
            {
                if (game->right_paddle_won)
                {
                    result_text = "PLAYER 2 WON";
                    result_color = GREEN;
                }
                else
                {
                    result_text = "PLAYER 1 WON";
                    result_color = GREEN;
                }
            }

            int result_text_width = MeasureText(result_text, game->font_large_size);

            DrawText(result_text,
                     game->screen_width / 2 - result_text_width / 2,
                     game->screen_height / 2 - 150,
                     game->font_large_size,
                     result_color);

            DrawText("PRESS X TO RESTART",
                     game->screen_width / 2 - MeasureText("PRESS X TO RESTART", 30) / 2,
                     game->screen_height / 2 + 60,
                     30,
                     RAYWHITE);

            DrawText("PRESS Y FOR MENU",
                     game->screen_width / 2 - MeasureText("PRESS Y FOR MENU", 30) / 2,
                     game->screen_height / 2 + 120,
                     30,
                     RAYWHITE);

        } break;
    }
}

void GameUnload(Game *game)
{
    UnloadMusicStream(game->music);
    UnloadSound(game->sound_hit);
}