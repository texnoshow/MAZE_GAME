#include <gb/gb.h>
#include <rand.h>
#include <stdio.h>

#define MAP_WIDTH 10
#define MAP_HEIGHT 9

// Карта лабиринта (1 - стена, 0 - пустая дорога)
const UBYTE MAZE[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,0,1,0,1},
    {1,0,0,0,0,1,0,0,0,1},
    {1,1,1,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
};

// Текстуры лабиринта
const unsigned char wall_tile[] = {
    0xFF,0x00,0x80,0x00,0x83,0x00,0x83,0x00,
    0xFF,0x00,0x20,0x00,0x20,0x00,0xFF,0x00
};
const unsigned char empty_tile[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// Спрайт Игрока (Круглая тарелка)
const unsigned char player_sprite[] = {
    0x3C,0x3C,0x42,0x7E,0x99,0xE7,0xBD,0xFF,
    0x81,0xFF,0x99,0xE7,0x42,0x7E,0x3C,0x3C
};

// Спрайт Врага (Пришелец)
const unsigned char enemy_sprite[] = {
    0x81,0x81,0x42,0x42,0x3C,0x3C,0x5A,0x5A,
    0xFF,0xFF,0xBD,0xBD,0xA5,0xA5,0x42,0x42
};

// ВЕРТИКАЛЬНЫЙ лазер
const unsigned char laser_vert_sprite[] = {
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
    0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18
};

// ГОРИЗОНТАЛЬНЫЙ лазер
const unsigned char laser_horiz_sprite[] = {
    0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// Структура сущностей
struct Entity {
    UBYTE x;
    UBYTE y;
    BYTE dir_x;
    BYTE dir_y;
    UBYTE alive;
};

struct Entity player = {32, 40, 0, -1, 1}; 
struct Entity bullet = {0, 0, 0, 0, 0};   

#define MAX_BOTS 3
struct Entity bots[MAX_BOTS] = {
    {120, 40, 1, 0, 1},
    {32, 104, 0, 1, 1},
    {120, 104, -1, 0, 1}
};

// Проверка одной точки
UBYTE check_point(UBYTE px, UBYTE py) {
    UBYTE tile_x = (px - 8) / 16;
    UBYTE tile_y = (py - 16) / 16;
    if (tile_x >= MAP_WIDTH || tile_y >= MAP_HEIGHT) return 0;
    return (MAZE[tile_y][tile_x] == 0);
}

// Проверка габаритов
UBYTE is_walkable(UBYTE new_x, UBYTE new_y) {
    if (!check_point(new_x, new_y)) return 0;          
    if (!check_point(new_x + 7, new_y)) return 0;      
    if (!check_point(new_x, new_y + 7)) return 0;      
    if (!check_point(new_x + 7, new_y + 7)) return 0;  
    return 1; 
}

void draw_maze_background() {
    UBYTE x, y;
    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {
            UBYTE tile_index = (MAZE[y][x] == 1) ? 1 : 0;
            set_bkg_tiles(x * 2,     y * 2,     1, 1, &tile_index);
            set_bkg_tiles(x * 2 + 1, y * 2,     1, 1, &tile_index);
            set_bkg_tiles(x * 2,     y * 2 + 1, 1, 1, &tile_index);
            set_bkg_tiles(x * 2 + 1, y * 2 + 1, 1, 1, &tile_index);
        }
    }
}

void init_sound() {
    NR52_REG = 0x80; NR50_REG = 0x77; NR51_REG = 0xFF; 
}
void play_laser_sound() {
    NR41_REG = 0x01; NR42_REG = 0xF1; NR43_REG = 0x22; NR44_REG = 0x80; 
}
void play_explosion_sound() {
    NR41_REG = 0x1F; NR42_REG = 0xF3; NR43_REG = 0x77; NR44_REG = 0x80; 
}

void main() {
    UBYTE i, j; // Переменные циклов
    UBYTE remaining_bots = MAX_BOTS;
    
    initrand(DIV_REG);
    init_sound();

    set_bkg_data(0, 1, empty_tile);
    set_bkg_data(1, 1, wall_tile);
    
    set_sprite_data(0, 1, player_sprite);     
    set_sprite_data(1, 1, laser_vert_sprite);  
    set_sprite_data(2, 1, enemy_sprite);       
    set_sprite_data(3, 1, laser_horiz_sprite); 
    
    set_sprite_tile(0, 0); 
    set_sprite_tile(1, 1); 
    for(i = 0; i < MAX_BOTS; i++) {
        set_sprite_tile(2 + i, 2); 
    }

    draw_maze_background();

    SHOW_BKG;
    SHOW_SPRITES;

    while(1) {
        
        // --- ПРОВЕРКА ПОБЕДЫ ---
        if (remaining_bots == 0) {
            move_sprite(0, 0, 0);
            move_sprite(1, 0, 0);
            for(i = 0; i < MAX_BOTS; i++) move_sprite(2 + i, 0, 0);
            printf("\n\n\n\n   === VICTORY ===\n\n   SPACE IS CLEANED!\n\n   GAME OVER.");
            while(1) { wait_vbl_done(); }
        }

        // --- УПРАВЛЕНИЕ ИГРОКОМ ---
        if (joypad() & J_LEFT) {
            player.dir_x = -1; player.dir_y = 0;
            if (is_walkable(player.x - 1, player.y)) player.x--;
        }
        else if (joypad() & J_RIGHT) {
            player.dir_x = 1; player.dir_y = 0;
            if (is_walkable(player.x + 1, player.y)) player.x++;
        }
        else if (joypad() & J_UP) {
            player.dir_x = 0; player.dir_y = -1;
            if (is_walkable(player.x, player.y - 1)) player.y--;
        }
        else if (joypad() & J_DOWN) {
            player.dir_x = 0; player.dir_y = 1;
            if (is_walkable(player.x, player.y + 1)) player.y++;
        }

        // --- ВЫСТРЕЛ ---
        if ((joypad() & J_A) && !bullet.alive) {
            bullet.x = player.x;
            bullet.y = player.y;
            bullet.dir_x = player.dir_x;
            bullet.dir_y = player.dir_y;
            bullet.alive = 1;

            if (bullet.dir_x != 0) {
                set_sprite_tile(1, 3);
            } else {
                set_sprite_tile(1, 1);
            }
            play_laser_sound(); 
        }

        // --- ЛОГИКА ПУЛИ ---
        if (bullet.alive) {
            bullet.x += bullet.dir_x * 2;
            bullet.y += bullet.dir_y * 2;
            
            if (!check_point(bullet.x + 3, bullet.y + 3)) {
                bullet.alive = 0;
            }
        }

        // --- БЕШЕНАЯ ЛОГИКА БОТОВ (СКОРОСТЬ 3X + АНТИ-СЛИПАНИЕ) ---
        for(i = 0; i < MAX_BOTS; i++) {
            if (!bots[i].alive) continue;

            // ЗНАЧИТЕЛЬНОЕ УСКОРЕНИЕ: Шаг равен 3 пикселям за кадр
            UBYTE speed = 3; 
            UBYTE panicked = 0;
            UBYTE blocked = 0;
            
            UBYTE next_x, next_y;

            // ПАТРУЛИРОВАНИЕ И ТАКТИЧЕСКИЙ ПОБЕГ (ИНВЕРСИЯ К КУРСУ ИГРОКА)
            if (player.y >= bots[i].y - 10 && player.y <= bots[i].y + 10) {
                bots[i].dir_x = (player.x > bots[i].x) ? -1 : 1; 
                bots[i].dir_y = 0;
                panicked = 1;
            }
            else if (player.x >= bots[i].x - 10 && player.x <= bots[i].x + 10) {
                bots[i].dir_x = 0;
                bots[i].dir_y = (player.y > bots[i].y) ? -1 : 1;
                panicked = 1;
            }

            // Рассчитываем предполагаемые новые координаты
            next_x = bots[i].x + (bots[i].dir_x * speed);
            next_y = bots[i].y + (bots[i].dir_y * speed);

            // КЛЮЧЕВАЯ ПРОВЕРКА НА СЛИПАНИЕ И СТЕНЫ В ОДИН КАДР
            // Проверяем, не врежемся ли мы в другого бота
            for(j = 0; j < MAX_BOTS; j++) {
                if (i == j || !bots[j].alive) continue;
                
                // Дистанция безопасности увеличена до 8 пикселей, чтобы на скорости 3x они не заскакивали друг в друга
                if (next_x >= bots[j].x - 8 && next_x <= bots[j].x + 8 &&
                    next_y >= bots[j].y - 8 && next_y <= bots[j].y + 8) {
                    blocked = 1;
                    break;
                }
            }

            // Если впереди стена, затор из ботов или случайный фактор (раз в 15 кадров для хаоса)
            if (!is_walkable(next_x, next_y) || blocked || (!panicked && (rand() % 15 == 0))) {
                
                // Жесткий цикл перебора направлений: бот ТУТ ЖЕ находит свободный путь, не допуская остановок
                UBYTE attempts = 0;
                while (attempts < 8) {
                    UBYTE rand_dir = rand() % 4;
                    if (rand_dir == 0)      { bots[i].dir_x = 1;  bots[i].dir_y = 0;  }
                    else if (rand_dir == 1) { bots[i].dir_x = -1; bots[i].dir_y = 0;  }
                    else if (rand_dir == 2) { bots[i].dir_x = 0;  bots[i].dir_y = 1;  }
                    else                    { bots[i].dir_x = 0;  bots[i].dir_y = -1; }

                    next_x = bots[i].x + (bots[i].dir_x * speed);
                    next_y = bots[i].y + (bots[i].dir_y * speed);

                    // Проверяем выбранный путь на стены
                    if (is_walkable(next_x, next_y)) {
                        // Проверяем выбранный путь на других ботов
                        UBYTE space_clean = 1;
                        for(j = 0; j < MAX_BOTS; j++) {
                            if (i == j || !bots[j].alive) continue;
                            if (next_x >= bots[j].x - 8 && next_x <= bots[j].x + 8 &&
                                next_y >= bots[j].y - 8 && next_y <= bots[j].y + 8) {
                                space_clean = 0;
                                break;
                            }
                        }
                        if (space_clean) break; // Свободный путь найден!
                    }
                    attempts++;
                }
            }

            // Окончательное перемещение (если путь полностью чист)
            if (is_walkable(next_x, next_y)) {
                bots[i].x = next_x;
                bots[i].y = next_y;
            }
            
            // Столкновение пули и пришельца
            if (bullet.alive && 
                bullet.x >= bots[i].x - 6 && bullet.x <= bots[i].x + 6 &&
                bullet.y >= bots[i].y - 6 && bullet.y <= bots[i].y + 6) {
                
                bots[i].alive = 0;   
                bullet.alive = 0;    
                remaining_bots--;    
                play_explosion_sound(); 
                move_sprite(2 + i, 0, 0); 
            }
        }

        // --- ОТРИСОВКА КАДРА ---
        move_sprite(0, player.x, player.y);
        
        if (bullet.alive) {
            move_sprite(1, bullet.x, bullet.y);
        } else {
            move_sprite(1, 0, 0); 
        }

        for(i = 0; i < MAX_BOTS; i++) {
            if (bots[i].alive) {
                move_sprite(2 + i, bots[i].x, bots[i].y);
            }
        }

        wait_vbl_done();
    }
}
