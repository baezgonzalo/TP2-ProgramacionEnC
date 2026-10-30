// Uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

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

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS DE DATOS =========

// 1. Nodo para la fila del tablero (Lista Enlazada)
typedef struct RowNode {
    int cells[GRID_COLS]; 
    struct RowNode* next; 
} RowNode;

// 2. Nodo para la cola de próximas piezas (Queue)
typedef struct PieceQueueNode {
    int type; 
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. Nodo para el sistema de partículas 
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;      
    int rotation;  
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;             
    PieceQueueNode* queue_head;     
    PieceQueueNode* queue_tail;     
    ParticleNode* first_particle;   
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// ========= VARIABLES GLOBALES =========
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ========= EJERCICIO 1: STRINGS (NO USAR string.h) =========

char* strDuplicate(char* src) {
    int len = 0;
    while (src[len] != '\0') {
        len++;
    }   
    char* copia = (char*) malloc (sizeof (char) * len + 1);
    if (copia == NULL) {
        return NULL;
    }
    for (int i = 0; i < len; i++) {
        copia [i] = src[i];
    }
    copia[len] = '\0';
    return copia;
}

int strCompare(char* s1, char* s2) {
    int len1 = 0;
    int len2 = 0;
    int i = 0;
    while (s1[len1] != '\0') {
        len1++;
    }
    while (s2[len2] != '\0') {
        len2++;
    }
    while (i < len1 || i < len2) {
        if (s1[i] < s2[i]) {
            return 1;
        }
        if (s1[i] > s2[i]) {
            return -1;
        }
        if (s1[i] == s2[i]) {
            i++;
        }
    }
    return 0;
}

char* strConcatenate(char* src1, char* src2) {
    int len1 = 0;
    int len2 = 0;
    while (src1[len1] != '\0') {
        len1++;
    }
    while (src2[len2] != '\0') {
        len2++;
    }
    char* nuevo = (char*) malloc (sizeof(char) * (len1 + len2 + 1));
    if (nuevo == NULL) {
        free(src1);
        free(src2);
        return NULL;
    }
    int j = 0;
    for (int i = 0; i < len1; i++) {
        nuevo[i] = src1[i];
        j++;
    }
    for (int i = j; i < (len1 + len2); i++) {
        nuevo[i] = src2[i - j];
    }
    nuevo[len1 + len2] = '\0';
    free(src1);
    free(src2);
    return nuevo;
}

// ========= EJERCICIO 2: FUNCIONES A IMPLEMENTAR (EL TP) =========

GameBoard* gameBoardNew() {
    GameBoard* board = (GameBoard*) malloc (sizeof(GameBoard));
    if (board == NULL) {
        return NULL;
    }
    board->score = 0;
    board->tick_timer = 0;
    board->first_row = NULL;
    board->queue_head = NULL;
    board->queue_tail = NULL;
    board->first_particle = NULL;
    board->current_piece.col = 0;
    board->current_piece.row = 0;
    board->current_piece.type = 0;
    board->current_piece.rotation = 0;
    board->current_piece.active = 0;
    for (int i = 0; i < GRID_ROWS; i++) {
        RowNode* node = (RowNode*) malloc(sizeof(RowNode));

        if (node == NULL) {
            RowNode* curr = board->first_row;
            while (curr != NULL) {
                RowNode* next = curr->next;
                free(curr);
                curr = next;
            }
            free(board);
            return NULL;
        }

        for (int c = 0; c < GRID_COLS; c++) {
            node->cells[c] = 0;
        }

        node->next = board->first_row;
        board->first_row = node;
    }
    return board;
}

void gameBoardDelete(GameBoard* board) {
    if (board == NULL) {
        return NULL;
    }
    RowNode* curr = board->first_row;
    while (curr != NULL) {
        RowNode* next = curr->next;
        free(curr);
        curr = next;
    }
    PieceQueueNode* curr_piece = board->queue_head; 
    while (curr_piece != NULL) {
        PieceQueueNode* next_piece = curr_piece->next;
        free(curr_piece);
        curr_piece = next_piece;
    }
    ParticleNode* curr_piece2 =  board->first_particle;
    while (curr_piece2 != NULL) {
        ParticleNode* next_piece2 = curr_piece2->next;
        free(curr_piece2);
        curr_piece2 = next_piece2;
    }
    free(board);
}

void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    ParticleNode* node = (ParticleNode*) malloc (sizeof(ParticleNode));
    if (node == NULL) {
        return;
    }
    node->x = x;
    node->y = y;
    node->color = color;
    node->vel_x = (float)(rand() % 7 - 3);
    node->vel_y = -(float)(rand() % 4 + 1);
    node->life_timer = 30;
    node->next = board->first_particle;
    board->first_particle = node;
}

int gameBoardCheckAndClearLines(GameBoard* board) {
    if (board == NULL) {
        return 0;
    }

    int eliminadas = 0;
    int r = 0;
    RowNode* prev = NULL;
    RowNode* curr = board->first_row;

    while (curr != NULL) {
        RowNode* next = curr->next;
        int completa = 1;

        for (int c = 0; c < GRID_COLS; c++) {
            if (curr->cells[c] == 0) {
                completa = 0;
                break;
            }
        }

        if (completa) {
            RowNode* nueva = (RowNode*) malloc(sizeof(RowNode));

            if (nueva == NULL) {
                prev = curr;
            }
            else {
                for (int c = 0; c < GRID_COLS; c++) {
                    nueva->cells[c] = 0;
                    gameBoardAddParticle(
                        board,
                        OFFSET_X + c * CELL_SIZE,
                        OFFSET_Y + r * CELL_SIZE,
                        curr->cells[c]
                    );
                }

                if (prev == NULL) {
                    board->first_row = next;
                }
                else {
                    prev->next = next;
                }
                free(curr);

                nueva->next = board->first_row;
                board->first_row = nueva;

                if (prev == NULL) {
                    prev = nueva;
                }
                eliminadas++;
            }
        } else {
            prev = curr;
        }

        curr = next;
        r++;
    }

    return eliminadas;
}

void gameBoardEnqueuePiece(GameBoard* board, int type) {
    PieceQueueNode* node = (PieceQueueNode*) malloc(sizeof(PieceQueueNode));
    if (node == NULL) {
        return;
    }
    node->type = type;
    node->next = NULL;
    if (board->queue_tail == NULL) {
        board->queue_head = node;
    } else {
        board->queue_tail->next = node;
    }
    board->queue_tail = node;
}

int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    if (board->queue_head == NULL) {
        return 0;
    }
    PieceQueueNode* node = board->queue_head;
    *out_type = node->type;
    board->queue_head = node->next;

    if (board->queue_head == NULL) {
        board->queue_tail = NULL;
    }
    free(node);
    return 1;
}

// ========= LOGICA DE DIBUJADO Y COLISIONES (NO MODIFICAR) =========

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// Devuelve 0 si la pieza nueva no entra (GAME OVER), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// Devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

// ========= TESTS (COMPLETAR) =========

void correrTests() {
    /*strDuplicate*/
    char original_vacio[] = "";
    char* copia_vacio = strDuplicate(original_vacio);
    printf("El output de strDuplicate('%s') deberia ser: %s", original_vacio, copia_vacio;);
    free(copia_vacio);

    char original_x[] = "x";
    char* copia_x = strDuplicate(original_x);
    printf("El output de strDuplicate('%s') deberia ser: %s", original_x, copia_x;);
    free(copia_x);

    char original_hola[] = "Hola";
    char* copia_hola = strDuplicate(original_hola);
    printf("El output de strDuplicate('%s') deberia ser: %s", original_hola, copia_hola;);
    free(copia_hola);

    /*strCompare*/
    char s1_vacio[] = "";
    char s2_vacio[] = "";
    int comparacion_vacios = strCompare(s1_vacio, s2_vacio);
    printf("El output de strCompare('%s', '%s') deberia ser 0: %d\n", s1_vacio, s2_vacio, comparacion_vacios);

    char s1_igual[] = "a";
    char s2_igual[] = "a";
    int comparacion_iguales = strCompare(s1_igual, s2_igual);
    printf("El output de strCompare('%s', '%s') deberia ser 0: %d\n", s1_igual, s2_igual, comparacion_iguales);

    char s1_menor[] = "a";
    char s2_mayor[] = "b";
    int comparacion_menor = strCompare(s1_menor, s2_mayor);
    printf("El output de strCompare('%s', '%s') deberia ser 1: %d\n", s1_menor, s2_mayor, comparacion_menor);

    int comparacion_mayor = strCompare(s2_mayor, s1_menor);
    printf("El output de strCompare('%s', '%s') deberia ser -1: %d\n", s2_mayor, s1_menor, comparacion_mayor);

    /*strConcatenate*/
    char original_concat_vacio[] = "";
    char original_concat_abc[] = "abc";
    char* copia_concat_vacio = strDuplicate(original_concat_vacio);
    char* copia_concat_abc = strDuplicate(original_concat_abc);
    char* concatenado_1 = strConcatenate(copia_concat_vacio, copia_concat_abc);
    printf("El output de strConcatenate('%s', '%s') deberia ser: 'abc'. Output: '%s'\n", original_concat_vacio, original_concat_abc, concatenado_1);
    free(concatenado_1);

    char original_concat_hola[] = "hola ";
    char original_concat_mundo[] = "mundo";
    char* copia_concat_hola = strDuplicate(original_concat_hola);
    char* copia_concat_mundo = strDuplicate(original_concat_mundo);
    char* concatenado_2 = strConcatenate(copia_concat_hola, copia_concat_mundo);
    printf("El output de strConcatenate('%s', '%s') deberia ser: 'hola mundo'. Output: '%s'\n", original_concat_hola, original_concat_mundo, concatenado_2);
    free(concatenado_2);

    /*gameBoardEnqueuePiece y gameBoardDequeuePiece*/
    GameBoard* board = gameBoardNew();
    if (board == NULL) {
        printf("No se pudo crear el tablero para los tests\n");
        return;
    }

    for (int i = 0; i < 5; i++) {
        gameBoardEnqueuePiece(board, i);
    }

    for (int i = 0; i < 6; i++) {
        int out_type = -1;
        int extrajo = gameBoardDequeuePiece(board, &out_type);

        if (i < 5) {
            printf("El Pop %d deberia devolver 1 y type=%d. Output: %d y type=%d\n", i + 1, i, extrajo, out_type);
        } else {
            printf("El Pop 6 deberia devolver 0 y dejar type=-1. Output: %d y type=%d\n", extrajo, out_type);
        }
    }

    printf("queue_head y queue_tail deberian ser NULL (1 y 1). Output: %d y %d\n", board->queue_head == NULL, board->queue_tail == NULL);

    /*gameBoardEnqueuePiece una vez más*/
    gameBoardEnqueuePiece(board, 5);
    int out_type = -1;
    int extrajo = gameBoardDequeuePiece(board, &out_type);
    printf("El Push y Pop posterior deberia devolver 1 y type=5. Output: %d y type=%d\n", extrajo, out_type);

        RowNode* fila_19 = getRowAt(board, 19);
    for (int c = 0; c < GRID_COLS; c++) {
        fila_19->cells[c] = 1;
    }

    /*gameBoardCheckAndClearLines con fila 19*/
    RowNode* primera_antes = board->first_row;
    int eliminadas_19 = gameBoardCheckAndClearLines(board);

    int cantidad_filas = 0;
    RowNode* curr = board->first_row;
    while (curr != NULL) {
        cantidad_filas++;
        curr = curr->next;
    }

    int primera_nueva = board->first_row != primera_antes;
    int primera_vacia = 1;
    for (int c = 0; c < GRID_COLS; c++) {
        if (board->first_row->cells[c] != 0) {
            primera_vacia = 0;
        }
    }

    printf("Al llenar la fila 19, eliminadas deberia ser 1. Output: %d\n", eliminadas_19);
    printf("La primera fila deberia ser nueva y vacia (1 y 1). Output: %d y %d\n", primera_nueva, primera_vacia);
    printf("La cantidad de filas deberia ser 20. Output: %d\n", cantidad_filas);


    /*gameBoardCheckAndClearLines con filas 0 y 1*/
    RowNode* fila_0 = getRowAt(board, 0);
    RowNode* fila_1 = getRowAt(board, 1);
    for (int c = 0; c < GRID_COLS; c++) {
        fila_0->cells[c] = 1;
        fila_1->cells[c] = 2;
    }

    int eliminadas_01 = gameBoardCheckAndClearLines(board);

    cantidad_filas = 0;
    curr = board->first_row;
    while (curr != NULL) {
        cantidad_filas++;
        curr = curr->next;
    }

    printf("Al llenar las filas 0 y 1, eliminadas deberia ser 2. Output: %d\n", eliminadas_01);
    printf("La cantidad de filas deberia seguir siendo 20. Output: %d\n", cantidad_filas);

    gameBoardDelete(board);
}

// Compara contra "--test" sin depender de string.h ni del Ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // Los tests corren antes de SDL. Con --test termina aca, asi valgrind solo
    // reporta la memoria del TP (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
