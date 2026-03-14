#include "raylib.h"
#include "pcg32.h"
#include <math.h>

int main(void)
{
    pcg_init();

    // screen
    const int screen_width = 1280;
    const int screen_height = 720;

    // paddles
    const int speed_paddle = 700;
    const int paddle_width = 20;
    const int paddle_height = 140;
    float enemy_reaction_timer = 0.0f;
    float enemy_reaction_delay = 0.0f;
    float enemy_target = screen_height / 2 - paddle_height / 2;
    bool enemy_has_prediction = false;

    // ball
    const float ball_radius = 12.5f;
    const float base_ball_speed = 700.0f;
    const float max_ball_speed = 1500.0f;
    float speed_ball = base_ball_speed;
    float blink_timer = 0.0f;
    float wait_timer = 0.0f;
    float start_delay = 0.0f;
    int blink_count = 0;
    bool ball_active = false;
    bool ball_visible = true;

    // font
    const int font_margin = 180;
    const int font_size = 30;

    // lives
    int enemy_lives = 3;
    int player_lives = 3;
    bool player_won = false;

    InitWindow(screen_width, screen_height, "Pong Game");
    InitAudioDevice();

    Music music = LoadMusicStream("sounds/retro_music.wav");
    bool music_playing = false;
    Sound sound_hit = LoadSound("sounds/hit_sound.wav");
    Sound sound_game_over = LoadSound("sounds/game_over_sound.wav");
    bool game_over_sound_played = false;
    
    SetTargetFPS(60);

    typedef enum
    {
        STATE_MENU,
        STATE_GAME,
        STATE_GAME_OVER,
    } game_state;

    game_state state = STATE_MENU;

    Rectangle player = 
    {
        screen_width - paddle_width - 100,
        screen_height / 2 - paddle_height / 2,
        paddle_width,
        paddle_height
    };

    Rectangle enemy = 
    {
        100,
        screen_height / 2 - paddle_height / 2,
        paddle_width,
        paddle_height
    };

    Vector2 ball_pos =
    {
        screen_width / 2,
        screen_height / 2
    };

    Vector2 ball_dir;
    float angle = (float)pcg_range_int(90) - 45.0f;

    angle *= DEG2RAD;

    ball_dir.x = cosf(angle);
    ball_dir.y = sinf(angle);

    if (pcg_range_int(2) == 0)
    {
        ball_dir.x *= -1;
    }

    while (!WindowShouldClose())
    {
        switch (state)
        {
            case STATE_MENU:

                if (IsKeyPressed(KEY_H))
                {
                    state = STATE_GAME;
                }

                BeginDrawing();

                    ClearBackground(BLACK);

                    const char *start_text = TextFormat("PRESS H TO START");
                    int start_text_width = MeasureText(start_text, font_size);
                    DrawText(start_text, screen_width / 2 - start_text_width / 2, screen_height / 2, font_size, RAYWHITE);

                EndDrawing();
                
                break;
            
            case STATE_GAME:
                
                if (!music_playing)
                {
                    PlayMusicStream(music);
                    music_playing = true;
                }

                UpdateMusicStream(music);

                if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
                    player.y -= speed_paddle * GetFrameTime();
                else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
                    player.y += speed_paddle * GetFrameTime();

                if (player.y < 0)
                    player.y = 0;
                else if (player.y > screen_height - player.height)
                    player.y = screen_height - player.height;

                if (!ball_active)
                {
                    if (start_delay < 1.0f)
                    {
                        start_delay += GetFrameTime();
                    }
                    else
                    {
                        blink_timer += GetFrameTime();

                        if (blink_count < 6)
                        {
                            if (blink_timer > 0.15f)
                            {
                                ball_visible = !ball_visible;
                                blink_timer = 0.0f;
                                blink_count++;
                            }
                        }
                        else
                        {
                            wait_timer += GetFrameTime();

                            if (wait_timer > 1.0f)
                            {
                                ball_active = true;
                                ball_visible = true;
                            }
                        }
                    }
                }

                if (ball_active)
                {
                    ball_pos.x += ball_dir.x * speed_ball * GetFrameTime();
                    ball_pos.y += ball_dir.y * speed_ball * GetFrameTime();
                }   
                
                if (ball_dir.x > 0 && CheckCollisionCircleRec(ball_pos, ball_radius, player))
                {
                    PlaySound(sound_hit);
                    float hit_pos = ball_pos.y - player.y;

                    float normalized = (hit_pos / player.height) * 2.0f - 1.0f;
                    float max_angle = 60.f * DEG2RAD;
                    float angle = normalized * max_angle;

                    ball_dir.x = -cosf(angle);
                    ball_dir.y = sinf(angle);

                    ball_pos.x = player.x - ball_radius;

                    speed_ball = fminf(speed_ball + 50.0f, max_ball_speed);
                }

                if (ball_dir.x < 0 && CheckCollisionCircleRec(ball_pos, ball_radius, enemy))
                {
                    PlaySound(sound_hit);
                    float hit_pos = ball_pos.y - enemy.y;

                    float normalized = (hit_pos / enemy.height) * 2.0f - 1.0f;
                    float max_angle = 60.0f * DEG2RAD;
                    float angle = normalized * max_angle;

                    ball_dir.x = cosf(angle);
                    ball_dir.y = sinf(angle);

                    ball_pos.x = enemy.x + enemy.width + ball_radius;

                    speed_ball = fminf(speed_ball + 50.0f, max_ball_speed);
                }

                if (ball_pos.y - ball_radius <= 0)
                {
                    ball_pos.y = ball_radius;
                    ball_dir.y *= -1;
                }

                if (ball_pos.y + ball_radius >= screen_height)
                {
                    ball_pos.y = screen_height - ball_radius;
                    ball_dir.y *= -1;
                }

                if (ball_pos.x <= 0 + ball_radius)
                {
                    enemy_lives--;

                    if (enemy_lives <= 0)
                    {
                        state = STATE_GAME_OVER;
                        player_won = true;
                    }

                    ball_active = false;
                    blink_timer = 0.0f;
                    wait_timer = 0.0f;
                    blink_count = 0;
                    start_delay = 0.0f;
                    ball_visible = true;
                    ball_pos = (Vector2){screen_width / 2, screen_height / 2};
                    angle = (float)pcg_range_int(90) - 45.0f;
                    angle *= DEG2RAD;
                    ball_dir.x = cosf(angle);
                    ball_dir.y = sinf(angle);

                    if (pcg_range_int(2) == 0)
                    {
                        ball_dir.x *= -1;
                    }

                    speed_ball = base_ball_speed;
                }

                if (ball_pos.x >= screen_width - ball_radius)
                {
                    player_lives--;

                    if (player_lives <= 0)
                    {
                        state = STATE_GAME_OVER;
                        player_won = false;
                    }

                    ball_active = false;
                    blink_timer = 0.0f;
                    wait_timer = 0.0f;
                    start_delay = 0.0f;
                    blink_count = 0;
                    ball_visible = true;
                    ball_pos = (Vector2){screen_width / 2, screen_height / 2};
                    angle = (float)pcg_range_int(90) - 45.0f;
                    angle *= DEG2RAD;
                    ball_dir.x = cosf(angle);
                    ball_dir.y = sinf(angle);

                    if (pcg_range_int(2) == 0)
                    {
                        ball_dir.x *= -1;
                    }

                    speed_ball = base_ball_speed;
                }

                if (ball_dir.x > 0 && enemy_has_prediction)
                {
                    enemy_has_prediction = false;
                    enemy_target = screen_height / 2 - enemy.height / 2;

                    enemy_reaction_delay = (float)(pcg_range_int(200) + 80) / 1000.0f;

                    enemy_reaction_timer = 0.0f;
                }

                if (ball_dir.x < 0 && !enemy_has_prediction && ball_pos.x < screen_width / 2)
                {
                    enemy_reaction_timer += GetFrameTime();

                    if (enemy_reaction_timer >= enemy_reaction_delay)
                    {
                        float vx = ball_dir.x * speed_ball;

                        if (fabsf(vx) > 0.01f)
                        {
                            float time = (enemy.x - ball_pos.x) / vx;

                            if (time > 0.0f)
                            {
                                float predicted_y = ball_pos.y + ball_dir.y * speed_ball * time;

                                while (predicted_y < 0 || predicted_y > screen_height)
                                {
                                    if (predicted_y < 0)
                                        predicted_y = -predicted_y;

                                    if (predicted_y > screen_height)
                                        predicted_y = 2 * screen_height - predicted_y;
                                }

                                enemy_target = predicted_y - enemy.height / 2;
                                enemy_target += pcg_range_int(200) - 100;

                                if (enemy_target < 0)
                                    enemy_target = 0;

                                if (enemy_target > screen_height - enemy.height)
                                    enemy_target = screen_height - enemy.height;

                                enemy_has_prediction = true;
                            }
                        }
                    }
                }

                float move = speed_paddle * GetFrameTime();
                float dead_zone = 10.0f;

                if (enemy.y < enemy_target - dead_zone)
                {
                    enemy.y += move;
                }
                else if (enemy.y > enemy_target + dead_zone)
                {
                    enemy.y -= move;
                }

                if (enemy.y < 0)
                    enemy.y = 0;
                else if (enemy.y > screen_height - enemy.height)
                    enemy.y = screen_height - enemy.height;
                    
                BeginDrawing();

                    ClearBackground(BLACK);

                    DrawRectangleRec(player, RAYWHITE);
                    DrawRectangleRec(enemy, RAYWHITE);

                    if (ball_visible)
                    {
                        DrawCircleV(ball_pos, ball_radius, RAYWHITE);
                    }
                    
                    const char *enemy_text = TextFormat("LIVES: %d", enemy_lives);
                    DrawText(enemy_text, font_margin, 30, font_size, RAYWHITE);

                    const char *pl_text = TextFormat("LIVES: %d", player_lives);
                    int pl_width = MeasureText(pl_text, 30);
                    int plX = screen_width - pl_width - font_margin;
                    DrawText(pl_text, plX, 30, font_size, RAYWHITE);

                    //DrawText(TextFormat("SPEED: %.2f", speed_ball), screen_width / 2, 30, font_size, GREEN);

                EndDrawing();

                break;

            case STATE_GAME_OVER:

                StopMusicStream(music);
                music_playing = false;

                if (!game_over_sound_played)
                {
                    PlaySound(sound_game_over);
                    game_over_sound_played = true;
                }

                if (IsKeyPressed(KEY_R))
                {
                    game_over_sound_played = false;

                    enemy_lives = 3;
                    player_lives = 3;

                    player_won = false;

                    ball_pos = (Vector2){screen_width /2, screen_height / 2};
                    speed_ball = base_ball_speed;

                    player.y = screen_height / 2 - paddle_height / 2;
                    enemy.y = screen_height / 2 - paddle_height / 2;

                    ball_active = false;
                    blink_timer = 0.0f;
                    wait_timer = 0.0f;
                    blink_count = 0;
                    start_delay = 0.0f;
                    ball_visible = true;

                    state = STATE_GAME;
                }

                if (IsKeyPressed(KEY_P))
                {
                    game_over_sound_played = false;

                    enemy_lives = 3;
                    player_lives = 3;

                    ball_active = false;
                    blink_timer = 0;
                    wait_timer = 0;
                    blink_count = 0;
                    start_delay = 0;

                    state = STATE_MENU;
                }

                BeginDrawing();

                    ClearBackground(BLACK);

                    Color result_color = player_won ? GREEN : RED;

                    const char *result_text = player_won ? "YOU WIN" : "YOU LOSE";
                    int result_text_width = MeasureText(result_text, 80);

                    DrawText(result_text,
                            screen_width/2 - result_text_width/2,
                            screen_height/2 - 150,
                            80,
                            result_color);

                    DrawText("PRESS R TO RESTART",
                            screen_width/2 - MeasureText("PRESS R TO RESTART", 30)/2,
                            screen_height/2 + 50,
                            30,
                            RAYWHITE);

                    DrawText("PRESS P FOR MENU",
                            screen_width/2 - MeasureText("PRESS P FOR MENU", 30)/2,
                            screen_height/2 + 100,
                            30,
                            RAYWHITE);

                EndDrawing();

            break;
        }
    }

    UnloadMusicStream(music);
    UnloadSound(sound_hit);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
