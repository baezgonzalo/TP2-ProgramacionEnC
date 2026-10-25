// juego_base_v2.c - Prototipo con estructuras estaticas (version corregida).
// Cambios respecto de juego_base.c:
//  - Se quita SDL_image.h (no se usaba).
//  - Alfa de particulas: se satura life_timer*8 a 255 (antes se truncaba a Uint8 y parpadeaba).
//  - GAME OVER sale del loop principal y libera SDL (antes hacia exit(0) directo).
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ========= CONSTANTES DEL JUEGO =========
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

// LIMITACIONES DEL PROTOTIPO A ARREGLAR EN EL TP
#define MAX_PARTICLES 15 
#define QUEUE_SIZE 3

const int TETROMINOS[7][4][4][4] = {
    // 0: I
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    // 1: J
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    // 2: L
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    // 3: O
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    // 4: S
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    // 5: T
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    // 6: Z
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS ESTÁTICAS (EL PROBLEMA A ARREGLAR) =========

typedef struct Particle {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    int active;
} Particle;

typedef struct ActivePiece {
    int col, row;
    int type;
    int rotation;
    int active;
} ActivePiece;

// Estado Global Estático
int grid[GRID_ROWS][GRID_COLS] = {0};
int piece_queue[QUEUE_SIZE] = {0};
Particle particles[MAX_PARTICLES] = {0};

ActivePiece current_piece = {0};
int score = 0;
int tick_timer = 0;
int game_over = 0;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// ========= FUNCIONES BASE =========

void inicializarJuegoBase() {
    for (int i = 0; i < QUEUE_SIZE; i++) {
        piece_queue[i] = rand() % 7;
    }
}

void agregarParticula(float x, float y, int color) {
    // BUG DE DISEÑO: Si el array está lleno, se ignoran los efectos visuales
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vel_x = ((rand() % 100) / 10.0f) - 5.0f;
            particles[i].vel_y = ((rand() % 50) / -10.0f) - 2.0f;
            particles[i].color = color;
            particles[i].life_timer = 30 + (rand() % 30);
            particles[i].active = 1;
            return;
        }
    }
    printf("ADVERTENCIA: Array de partículas lleno. Efecto descartado.\n");
}

void procesarFilasCompletas() {
    int lines_cleared = 0;

    for (int r = 0; r < GRID_ROWS; r++) {
        int is_full = 1;
        for (int c = 0; c < GRID_COLS; c++) {
            if (grid[r][c] == 0) {
                is_full = 0;
                break;
            }
        }

        if (is_full) {
            lines_cleared++;
            
            // Intentar generar partículas (muchas fallarán por el tamaño fijo del array)
            for (int c = 0; c < GRID_COLS; c++) {
                agregarParticula(OFFSET_X + c * CELL_SIZE + CELL_SIZE/2, 
                                 OFFSET_Y + r * CELL_SIZE + CELL_SIZE/2, 
                                 grid[r][c]);
            }

            // BUG DE DISEÑO: Desplazamiento de memoria ineficiente O(N)
            for (int r_shift = r; r_shift > 0; r_shift--) {
                for (int c = 0; c < GRID_COLS; c++) {
                    grid[r_shift][c] = grid[r_shift - 1][c];
                }
            }
            // Limpiar la primera fila
            for (int c = 0; c < GRID_COLS; c++) {
                grid[0][c] = 0;
            }
        }
    }
    score += lines_cleared * 100;
}

int puedeMover(int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0 && grid[board_row][board_col] != 0) return 0;
            }
        }
    }
    return 1;
}

void bloquearPieza() {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[current_piece.type][current_piece.rotation][r][c];
            if (val != 0) {
                grid[current_piece.row + r][current_piece.col + c] = val;
            }
        }
    }
    current_piece.active = 0;
    procesarFilasCompletas();
}

void generarNuevaPieza() {
    current_piece.type = piece_queue[0];
    current_piece.rotation = 0;
    current_piece.col = GRID_COLS / 2 - 2;
    current_piece.row = 0;
    current_piece.active = 1;
    
    // Desplazamiento manual del array (ineficiente)
    for (int i = 0; i < QUEUE_SIZE - 1; i++) {
        piece_queue[i] = piece_queue[i + 1];
    }
    piece_queue[QUEUE_SIZE - 1] = rand() % 7;
    
    if (!puedeMover(current_piece.col, current_piece.row, current_piece.rotation)) {
        printf("GAME OVER. Puntaje final: %d\n", score);
        current_piece.active = 0;
        game_over = 1;   // main sale del loop y libera SDL
    }
}

void actualizarEstado() {
    if (!current_piece.active) generarNuevaPieza();
    if (game_over) return;

    tick_timer++;
    if (tick_timer >= TICK_RATE) {
        tick_timer = 0;
        if (puedeMover(current_piece.col, current_piece.row + 1, current_piece.rotation)) {
            current_piece.row++;
        } else {
            bloquearPieza();
        }
    }

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].x += particles[i].vel_x;
            particles[i].y += particles[i].vel_y;
            particles[i].vel_y += 0.5f;
            particles[i].life_timer--;
            if (particles[i].life_timer <= 0) particles[i].active = 0;
        }
    }
}

void dibujar() {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (grid[r][c] > 0) {
                SDL_Color col = colors[grid[r][c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
    }

    if (current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int val = TETROMINOS[current_piece.type][current_piece.rotation][pr][pc];
                if (val != 0) {
                    SDL_Color col = colors[val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            SDL_Color col = colors[particles[i].color];
            int alpha = particles[i].life_timer * 8;
            if (alpha > 255) alpha = 255;
            SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
            SDL_Rect part_rect = {(int)particles[i].x, (int)particles[i].y, 6, 6};
            SDL_RenderFillRect(renderer, &part_rect);
        }
    }
    SDL_RenderPresent(renderer);
}

int inicializarSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base (Estructuras Estaticas)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

int main(int argc, char* args[]) {
    srand(time(NULL));
    if (!inicializarSDL()) return 1;
    inicializarJuegoBase();

    SDL_Event e;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_LEFT && puedeMover(current_piece.col - 1, current_piece.row, current_piece.rotation)) current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && puedeMover(current_piece.col + 1, current_piece.row, current_piece.rotation)) current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && puedeMover(current_piece.col, current_piece.row + 1, current_piece.rotation)) current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (current_piece.rotation + 1) % 4;
                    if (puedeMover(current_piece.col, current_piece.row, next_rot)) current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE && current_piece.active) {
                     while(puedeMover(current_piece.col, current_piece.row + 1, current_piece.rotation)) current_piece.row++;
                     bloquearPieza();
                }
            }
        }
        actualizarEstado();
        dibujar();
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
