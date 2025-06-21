#include <stdio.h>
#include <math.h>
#include <stdlib.h> // Para a função rand()
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>

// ==========================================================================
// 1. DEFINIÇÕES GLOBAIS (TIPOS E CONSTANTES)
// ==========================================================================

#define LARGURA 1680
#define ALTURA 1050
#define TAM_JOGADOR 50
#define PLAYER_ESCALA 1.0
#define ALTURA_CHAO 50
#define GRAVIDADE 0.3
#define PULO_FORCA -10.0
#define TAM_TIROS 10
#define TAM_INIMIGO 20
#define MAX_TIROS 20 // Aumentado para suportar rajadas
#define MAX_INIMIGOS 15
#define MAX_CUSPES 50
#define VELOCIDADE_ZUMBI 1.0
#define VELOCIDADE_BASE 2.0 // Aumentado um pouco a velocidade base
#define MAX_MUNICAO_ITENS 10
#define MAX_FRAMES_POR_ANIM 15

// --- Enums para Estados ---
typedef enum { FASE_NORMAL, BATALHA_CHEFE } EstadoDaFase;
typedef enum { ZUMBI_ANDARILHO, ZUMBI_CUSPIDOR } TipoInimigo;
typedef enum { NORMAL, CORRENDO, CANSADO, COOLDOWN } EstadoStamina;

// Struct para agrupar todos os recursos carregados
typedef struct {
    ALLEGRO_BITMAP* fundo;
    ALLEGRO_BITMAP* hp_sprite;
    ALLEGRO_BITMAP* caveira_sprite;
    ALLEGRO_BITMAP* player_sprite;
    ALLEGRO_FONT* font;

    // --- CAMPOS CORRIGIDOS E ADICIONADOS ---
    ALLEGRO_BITMAP* zumbi_cuspidor_sprite;
    ALLEGRO_BITMAP* zumbi_sprites[3]; // Um array para os 4 tipos de zumbis normais
} Assets;

// Struct para definir um único quadro de uma spritesheet
typedef struct {
    int x; // Posição X do quadro na imagem
    int y; // Posição Y do quadro na imagem
    int w; // Largura (width) do quadro
    int h; // Altura (height) do quadro
} Frame;

typedef struct {
    float x, y;
    float dy;
    bool no_chao, ativo;
    int direcao, hp, intangivel_timer, municao, timer_stamina;
    float velocidade;
    EstadoStamina estado_stamina; // Estado da stamina do jogador

    Frame* anim_sequencia_atual; // Ponteiro para o vetor da animação atual
    int    num_frames_na_anim;   // Quantos frames tem a animação atual
    int    anim_frame_atual;     // Índice do frame atual (0, 1, 2...)
    int    anim_timer;           // Timer para controlar a velocidade
    int    timer_anim_tiro;     // Timer para controlar a animação do tiro
    int    timer_anim_reload; // Timer para controlar a animação de recarga
    int    timer_anim_morte;       
} Jogador;

typedef struct {float x, y, dx, dy; bool ativo; } Tiro;

typedef struct {float x, y; bool ativo; int tempo_de_vida;} ItemMunicao;

typedef struct {float x, y, dx, dy; bool ativo; } Cuspe;

typedef struct {
    float x, y;
    float dx;                   // Velocidade de movimento horizontal do chefe
    int hp;
    int hp_max;
    bool ativo;
    int timer_ataque_principal; // Cooldown entre as rajadas de tiros
    int timer_rajada;           // Cooldown entre cada tiro de uma rajada
    int tiros_na_rajada;      // Quantos tiros ainda faltam na rajada atual
} Chefe;

typedef struct {
    float x, y, dx, dy;
    bool ativo, no_chao, morrendo;
    TipoInimigo tipo;
    int hp, hp_max, timer_ataque, timer_anim_dano;
    int direcao;

    // --- CAMPOS DE ANIMAÇÃO SIMPLIFICADOS ---
    int sprite_index;       // Qual dos 4 zumbis ele é (0 a 3)
    int anim_frame_atual;   // Em qual quadro da animação ele está
    int anim_timer;         // Timer de velocidade da animação
} Inimigo;

/*===========================*/
/*DADOS DA ANIMAÇÃO DO JOGADOR*/
/*============================*/


Frame anim_idle[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {48, 63, 41, 65},  // Frame 1
    {48 + 1 * 129 -1, 63, 41, 65}, // Frame 2
    {48 + 2  * 129 -2, 63, 41, 65}, // Frame 3 
    {48 + 3 * 129 -3, 63, 41, 65}, // Frame 4
    {48 + 4 * 129 -4, 63, 41, 65}, // Frame 5
    {48 + 5 * 129 -5, 63, 41, 65}  // Frame 6
};

Frame anim_andando[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {48, 190, 41, 65},  // Frame 1
    {48 + 1* 128, 190, 41, 65}, // Frame 2
    {48 + 2* 128, 190, 41, 65}, // Frame 3
    {48 + 3* 128, 190, 41, 65}, // Frame 4
    {48 + 4 * 128, 190, 41, 65}, // Frame 5
    {48 + 5 * 128, 190, 41, 65},  // Frame 6
    {48 + 6 * 128, 190, 41, 65}, // Frame 7
    {48 + 7 * 128 , 190, 41, 65}  // Frame 8
};

Frame anim_correndo[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {43, 317, 41, 65},  // Frame 1
    {43 + 1 * 125, 317, 41, 65}, // Frame 2
    {43 + 2 * 125, 317, 41, 65}, // Frame 3
    {43 + 3 * 125, 317, 41, 65}, // Frame 4
    {43 + 4 * 125, 317, 41, 65}, // Frame 5
    {43 + 5 * 125, 317, 41, 65}, // Frame 6
    {43 + 6 * 125, 317, 41, 65}, // Frame 7
    {43 + 7 * 125 ,317 ,41 ,65}   // Frame 8
};

Frame anim_pulo[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {53, 444, 41, 65},  // Frame 1
    {53 + 1 * 129, 444, 41, 65}, // Frame 2
    {53 + 2 * 129, 444, 41, 65}, // Frame 3
    {53 + 3 * 129, 444, 41, 65}, // Frame 4
    {53 + 4 * 129, 444, 41, 65}, // Frame 5
    {53 + 5 * 129, 444, 41, 65}, // Frame 6
    {53 + 6 * 129, 444, 41, 65}, // Frame 7
    {53 + 7 * 129 ,444 ,41 ,65}   // Frame 8
};

Frame anim_dano[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {43, 575, 41, 65},  // Frame 1
    {43 + 1 * 129, 575, 41, 65}, // Frame 2
};

Frame anim_tiro[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {21, 699, 41, 65},  // Frame 1
    {21 +1 * 127, 699, 50, 65}, // Frame 2
    {21 +2 * 127, 699, 104, 65}, // Frame 3
    {21 +3 * 127, 699, 104, 65}, // Frame 4
    {21 +4 * 127, 699, 104, 65}, // Frame 5
    {21 +5 * 127, 699, 104, 65},   // Frame 6
    {21 +6 * 127, 699, 41, 65}, // Frame 7
    {21 +7 * 127, 699, 41, 65}, // Frame 8
    {21 +8 * 127, 699, 41, 65}, // Frame 9
    {21 +9 * 127, 699, 41, 65},   // Frame 10
    {21 +10 * 127, 699, 41, 65},  // Frame 11
    {21 +12 * 127, 699, 41, 65} // Frame 12
};

Frame anim_reload[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {43, 828, 41, 65},  // Frame 1
    {43 + 1 * 128, 828, 41, 65}, // Frame 2
    {43 + 2 * 128, 828, 41, 65}, // Frame 3
    {43 + 3 * 128, 828, 41, 65}, // Frame 4
    {43 + 4 * 128, 828, 41, 65}, // Frame 5
    {43 + 5 * 128, 828, 41, 65}, // Frame 6
    {43 + 6 * 128, 828, 41, 65}, // Frame 7
    {43 + 7 * 128, 828, 41, 65}, // Frame 8
    {43 + 8 * 128, 828, 41, 65}, // Frame 9
    {43 + 9 * 128, 828, 41, 65}, // Frame 10
    {43 + 10 * 128, 828, 41, 65}, // Frame 11
    {43 + 11 * 128, 828, 41, 65} // Frame 12
};

Frame anim_morte[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {48, 1219, 48, 65},  // Frame 1
    {48 + 1 * 128, 1219, 52, 62}, // Frame 2
    {48 + 2 * 128, 1219, 41, 54}, // Frame 3
    {48 + 3 * 128, 1219, 82, 19} // Frame 4
};

int num_frames_idle = 6;
int num_frames_andando = 8;
int num_frames_correndo = 8;
int num_frames_pulo = 9;
int num_frames_dano = 2;
int num_frames_tiro = 12;
int num_frames_reload = 12;
int num_frames_morte = 4;

void atualiza_animacao_jogador(Jogador* jogador, bool esta_andando) {
    Frame* proxima_anim = anim_idle;
    int    proximo_num_frames = num_frames_idle;
    bool   animacao_loop = true; // A maioria das animações se repete

    // --- LÓGICA DE ANIMAÇÃO FINAL COM MORTE ---
    if (!jogador->ativo) {
        // 1. Prioridade máxima absoluta: Morte
        proxima_anim = anim_morte;
        proximo_num_frames = num_frames_morte;
        animacao_loop = false; // A animação de morte não se repete
    }
    else if (jogador->timer_anim_tiro > 0) {
        // 2. Tiro
        proxima_anim = anim_tiro;
        proximo_num_frames = num_frames_tiro;
    }
    else if (jogador->timer_anim_reload > 0) {
        // 3. Recarga
        proxima_anim = anim_reload;
        proximo_num_frames = num_frames_reload;
    }
    else if (!jogador->no_chao) {
        // 4. Pulo
        proxima_anim = anim_pulo;
        proximo_num_frames = num_frames_pulo;
    }
    else if (jogador->estado_stamina == CORRENDO && esta_andando) {
        // 5. Correndo
        proxima_anim = anim_correndo;
        proximo_num_frames = num_frames_correndo;
    }
    else if (esta_andando) {
        // 6. Andando
        proxima_anim = anim_andando;
        proximo_num_frames = num_frames_andando;
    }
    else {
        // 7. Parado
        proxima_anim = anim_idle;
        proximo_num_frames = num_frames_idle;
    }

    // Se a animação mudou, reinicia a contagem de frames
    if (jogador->anim_sequencia_atual != proxima_anim) {
        jogador->anim_sequencia_atual = proxima_anim;
        jogador->num_frames_na_anim = proximo_num_frames;
        jogador->anim_frame_atual = 0;
        jogador->anim_timer = 0;
    }

    // Avança o frame da animação baseado no timer
    int velocidade_anim = 8;
    if (jogador->anim_sequencia_atual == anim_tiro || jogador->anim_sequencia_atual == anim_reload) {
        velocidade_anim = 5;
    }

    jogador->anim_timer++;
    if (jogador->anim_timer >= velocidade_anim) {
        jogador->anim_timer = 0;
        // Se a animação pode dar loop, avança normalmente
        if (animacao_loop) {
            jogador->anim_frame_atual = (jogador->anim_frame_atual + 1) % jogador->num_frames_na_anim;
        } 
        // Se não pode dar loop, avança até o último quadro e para
        else if (jogador->anim_frame_atual < jogador->num_frames_na_anim - 1) {
            jogador->anim_frame_atual++;
        }
    }
}

/*==============*/
/*DADOS ANIMAÇÃO ZUMBI ANDARILHO*/
/*==============*/

// Struct para agrupar todas as animações de UM TIPO de zumbi
typedef struct {
    Frame andar[MAX_FRAMES_POR_ANIM];
    int num_frames_andar;

    Frame ataque[MAX_FRAMES_POR_ANIM];
    int num_frames_ataque;

    Frame dano[MAX_FRAMES_POR_ANIM];
    int num_frames_dano;

    Frame morte[MAX_FRAMES_POR_ANIM];
    int num_frames_morte;
} ZumbiAnimSet;

ZumbiAnimSet animacoes_zumbis[3];

void inicializa_dados_animacao_zumbis() {

    // --- ZUMBI 1 (sprites_z1.png) ---
    // Preencha com os dados exatos do primeiro zumbi
    animacoes_zumbis[0].num_frames_andar = 10;
    animacoes_zumbis[0].andar[0] = (Frame){47, 62, 46, 66};
    animacoes_zumbis[0].andar[1] = (Frame){177, 61, 46, 67};
    animacoes_zumbis[0].andar[2] = (Frame){302, 61, 46, 67};
    animacoes_zumbis[0].andar[3] = (Frame){426, 62, 46, 65};
    animacoes_zumbis[0].andar[4] = (Frame){554, 62, 46, 67};
    animacoes_zumbis[0].andar[5] = (Frame){686, 62, 46, 67};
    animacoes_zumbis[0].andar[6] = (Frame){813, 63, 46, 65};
    animacoes_zumbis[0].andar[7] = (Frame){945, 63, 46, 65};
    animacoes_zumbis[0].andar[8] = (Frame){1071, 63, 46, 65};
    animacoes_zumbis[0].andar[9] = (Frame){1197, 62, 46, 66};

    animacoes_zumbis[0].num_frames_ataque = 5;
    animacoes_zumbis[0].ataque[0] = (Frame){43, 189, 49, 67}; // Frame de ataque
    animacoes_zumbis[0].ataque[1] = (Frame){169, 189, 49, 67};
    animacoes_zumbis[0].ataque[2] = (Frame){299, 189, 49, 67};
    animacoes_zumbis[0].ataque[3] = (Frame){430, 189, 49, 65};
    animacoes_zumbis[0].ataque[4] = (Frame){559, 189, 49, 65};

    animacoes_zumbis[0].num_frames_dano = 4;
    animacoes_zumbis[0].dano[0] = (Frame){40, 317, 50, 67}; // Frame de dano
    animacoes_zumbis[0].dano[1] = (Frame){167, 317, 50, 67};
    animacoes_zumbis[0].dano[2] = (Frame){292, 317, 50, 67};
    animacoes_zumbis[0].dano[3] = (Frame){413, 317, 50, 67};

    animacoes_zumbis[0].num_frames_morte = 5;
    animacoes_zumbis[0].morte[0] = (Frame){50, 447, 64, 67}; // Frame de morte
    animacoes_zumbis[0].morte[1] = (Frame){180, 453, 64, 67};
    animacoes_zumbis[0].morte[2] = (Frame){302, 447, 64, 67};
    animacoes_zumbis[0].morte[3] = (Frame){437, 482, 64, 67};
    animacoes_zumbis[0].morte[4] = (Frame){565, 503, 64, 67};


    // --- ZUMBI 2 (sprites_z2.png) ---
    animacoes_zumbis[1].num_frames_andar = 12; // Este pode ter menos frames
    animacoes_zumbis[1].andar[0] = (Frame){39, 60, 45, 68}; // Posição e tamanho diferentes
    animacoes_zumbis[1].andar[1] = (Frame){165, 61, 46, 67};
    animacoes_zumbis[1].andar[2] = (Frame){297, 60, 44, 68};
    animacoes_zumbis[1].andar[3] = (Frame){428, 60, 40, 68};
    animacoes_zumbis[1].andar[4] = (Frame){557, 60, 40, 68};
    animacoes_zumbis[1].andar[5] = (Frame){680, 60, 48, 68};
    animacoes_zumbis[1].andar[6] = (Frame){806, 59, 52, 69};
    animacoes_zumbis[1].andar[7] = (Frame){936, 59, 49, 69};
    animacoes_zumbis[1].andar[8] = (Frame){1065, 59, 44, 69};
    animacoes_zumbis[1].andar[9] = (Frame){1191, 60, 47, 68};
    animacoes_zumbis[1].andar[10] = (Frame){1320, 60, 48, 69};
    animacoes_zumbis[1].andar[11] = (Frame){1447, 60, 49, 68};
    

    animacoes_zumbis[1].num_frames_ataque = 10;
    animacoes_zumbis[1].ataque[0] = (Frame){39, 190, 50, 77}; // Frame de ataque
    animacoes_zumbis[1].ataque[1] = (Frame){163, 189, 50, 77};
    animacoes_zumbis[1].ataque[2] = (Frame){287, 190, 50, 77};
    animacoes_zumbis[1].ataque[3] = (Frame){423, 179, 50, 77};
    animacoes_zumbis[1].ataque[4] = (Frame){549, 181, 50, 77};
    animacoes_zumbis[1].ataque[5] = (Frame){685, 184, 50, 72};
    animacoes_zumbis[1].ataque[6] = (Frame){812, 191, 51, 77};
    animacoes_zumbis[1].ataque[7] = (Frame){942, 190, 50, 77};
    animacoes_zumbis[1].ataque[8] = (Frame){1069, 191, 50, 77};
    animacoes_zumbis[1].ataque[9] = (Frame){1198, 191, 50, 77};
    
    
    animacoes_zumbis[1].num_frames_dano = 4;
    animacoes_zumbis[1].dano[0] = (Frame){48, 318, 38, 66}; // Frame de dano
    animacoes_zumbis[1].dano[1] = (Frame){175, 317, 38, 67};
    animacoes_zumbis[1].dano[2] = (Frame){303, 318, 38, 66};
    animacoes_zumbis[1].dano[3] = (Frame){431, 318, 41, 67};


    animacoes_zumbis[1].num_frames_morte = 5;
    animacoes_zumbis[1].morte[0] = (Frame){46, 451, 80, 61}; // Frame de morte
    animacoes_zumbis[1].morte[1] = (Frame){168, 457, 80, 61};
    animacoes_zumbis[1].morte[2] = (Frame){295, 472, 80, 61};
    animacoes_zumbis[1].morte[3] = (Frame){410, 492, 80, 61};
    animacoes_zumbis[1].morte[4] = (Frame){536, 494, 80, 61};



    // --- ZUMBI 3 (sprites_z3.png) ---

    animacoes_zumbis[2].num_frames_andar = 10;
    animacoes_zumbis[2].andar[0] = (Frame){44, 63, 60, 65}; // Posição e tamanho diferentes
    animacoes_zumbis[2].andar[1] = (Frame){173, 64, 60, 65};
    animacoes_zumbis[2].andar[2] = (Frame){295, 64, 60, 65};
    animacoes_zumbis[2].andar[3] = (Frame){422, 65, 60, 65};
    animacoes_zumbis[2].andar[4] = (Frame){554, 64, 60, 65};
    animacoes_zumbis[2].andar[5] = (Frame){687, 63, 60, 65};
    animacoes_zumbis[2].andar[6] = (Frame){814, 64, 60, 65};
    animacoes_zumbis[2].andar[7] = (Frame){934, 65, 60, 65};
    animacoes_zumbis[2].andar[8] = (Frame){1063, 65, 60, 65};
    animacoes_zumbis[2].andar[9] = (Frame){1193, 64, 60, 65};

    animacoes_zumbis[2].num_frames_ataque = 5;
    animacoes_zumbis[2].ataque[0] = (Frame){42, 188, 56, 74}; // Frame de ataque
    animacoes_zumbis[2].ataque[1] = (Frame){165, 185, 56, 74};
    animacoes_zumbis[2].ataque[2] = (Frame){296, 182, 56, 74};
    animacoes_zumbis[2].ataque[3] = (Frame){432, 187, 56, 74};
    animacoes_zumbis[2].ataque[4] = (Frame){560, 190, 56, 74};

    animacoes_zumbis[2].num_frames_dano = 4;
    animacoes_zumbis[2].dano[0] = (Frame){43, 316, 57, 70}; // Frame de dano
    animacoes_zumbis[2].dano[1] = (Frame){166, 315, 57, 70};
    animacoes_zumbis[2].dano[2] = (Frame){288, 316, 57, 70};
    animacoes_zumbis[2].dano[3] = (Frame){412, 316, 57, 70};

    animacoes_zumbis[2].num_frames_morte = 5;
    animacoes_zumbis[2].morte[0] = (Frame){36, 451, 55, 61}; // Frame de morte
    animacoes_zumbis[2].morte[1] = (Frame){161, 453, 55, 61};
    animacoes_zumbis[2].morte[2] = (Frame){279, 458, 55, 61};
    animacoes_zumbis[2].morte[3] = (Frame){410, 466, 55, 61};
    animacoes_zumbis[2].morte[4] = (Frame){529, 499, 55, 61};
}

void atualiza_animacao_inimigo(Inimigo* inimigo, Jogador* jogador) {
    // Pega o conjunto de animações correto para este tipo de zumbi
    ZumbiAnimSet* anim_set = &animacoes_zumbis[inimigo->sprite_index];
    
    // Ponteiros para os dados da animação atual
    Frame* sequencia_atual;
    int num_frames_atual;
    bool loop = true;

    // Lógica de prioridade para decidir qual animação usar
    if (inimigo->morrendo) {
        sequencia_atual = anim_set->morte;
        num_frames_atual = anim_set->num_frames_morte;
        loop = false;
    } else if (inimigo->timer_anim_dano > 0) {
        sequencia_atual = anim_set->dano;
        num_frames_atual = anim_set->num_frames_dano;
    } else if (inimigo->x == jogador->x && inimigo->y == jogador->y) {
        sequencia_atual = anim_set->ataque;
        num_frames_atual = anim_set->num_frames_ataque;
    }else {
        sequencia_atual = anim_set->andar;
        num_frames_atual = anim_set->num_frames_andar;
    }

    // Avança o frame
    if (++inimigo->anim_timer >= 10) {
        inimigo->anim_timer = 0;
        if (loop) {
            inimigo->anim_frame_atual = (inimigo->anim_frame_atual + 1) % num_frames_atual;
        } else if (inimigo->anim_frame_atual < num_frames_atual - 1) {
            inimigo->anim_frame_atual++;
        }
    }
}

/*======================*/
/*   FUNÇÕES DO MENU    */
/*======================*/


void desenhar_menu(Assets* assets, int item_selecionado) {
    if (assets->fundo) {
        al_draw_bitmap(assets->fundo, 0, 0, 0);
    } else {
        al_clear_to_color(al_map_rgb(0,0,0));
    }

    const char* itens[] = {"JOGAR", "CONFIGURACOES", "SAIR"};
    for (int i = 0; i < 3; i++) {
        ALLEGRO_COLOR cor = (i == item_selecionado) ? al_map_rgb(255, 255, 0) : al_map_rgb(255, 255, 255);
        al_draw_text(assets->font, cor, LARGURA / 2, ALTURA / 2 - 40 + i * 40, ALLEGRO_ALIGN_CENTER, itens[i]);
    }
    al_flip_display();
}

int loop_do_menu(ALLEGRO_EVENT_QUEUE *queue, Assets* assets) {
    int item_selecionado = 0;
    desenhar_menu(assets, item_selecionado);

    while (true) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            switch (ev.keyboard.keycode) {
                case ALLEGRO_KEY_UP:
                    item_selecionado = (item_selecionado == 0) ? 2 : item_selecionado - 1;
                    break;
                case ALLEGRO_KEY_DOWN:
                    item_selecionado = (item_selecionado + 1) % 3;
                    break;
                case ALLEGRO_KEY_ENTER:
                    return item_selecionado; // Retorna a escolha do usuário
                case ALLEGRO_KEY_ESCAPE:
                    return 2; // Trata ESC como "SAIR"
            }
        } else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            return 2; // Trata o 'X' da janela como "SAIR"
        }

        // Redesenha o menu somente se houver uma mudança de input
        desenhar_menu(assets, item_selecionado);
    }
}

void tela_game_over(ALLEGRO_FONT* font, int zumbis_mortos) {
    // Cria uma fila de eventos local apenas para esta tela
    ALLEGRO_EVENT_QUEUE* fila_game_over = al_create_event_queue();
    al_register_event_source(fila_game_over, al_get_keyboard_event_source());

    bool sair = false;
    while (!sair) {
        // --- Desenho da Tela ---
        al_clear_to_color(al_map_rgb(20, 0, 0)); 

        al_draw_text(font, al_map_rgb(200, 0, 0), LARGURA / 2, ALTURA / 3, ALLEGRO_ALIGN_CENTER, "GAME OVER");

        al_draw_textf(font, al_map_rgb(255, 255, 255), LARGURA / 2, ALTURA / 2, ALLEGRO_ALIGN_CENTER, "Zumbis derrotados: %d", zumbis_mortos);

        al_draw_text(font, al_map_rgb(150, 150, 150), LARGURA / 2, ALTURA - 100, ALLEGRO_ALIGN_CENTER, "Pressione ENTER para voltar ao menu");

        al_flip_display();

        // --- Lógica de Eventos ---
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila_game_over, &ev);

        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER || ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                
                sair = true;
            }
        }
    }

    // Limpa a fila de eventos local
    al_destroy_event_queue(fila_game_over);
}

void tela_vitoria(ALLEGRO_FONT* font) {
    al_clear_to_color(al_map_rgb(20, 40, 20));
    al_draw_text(font, al_map_rgb(0, 200, 0), LARGURA / 2, ALTURA / 3, ALLEGRO_ALIGN_CENTER, "VOCE VENCEU!");
    al_draw_text(font, al_map_rgb(150, 150, 150), LARGURA / 2, ALTURA - 100, ALLEGRO_ALIGN_CENTER, "Pressione ENTER para voltar ao menu");
    al_flip_display();
    
    ALLEGRO_EVENT_QUEUE* fila_vitoria = al_create_event_queue();
    al_register_event_source(fila_vitoria, al_get_keyboard_event_source());
    ALLEGRO_EVENT ev;
    while (1) {
        al_wait_for_event(fila_vitoria, &ev);
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN && (ev.keyboard.keycode == ALLEGRO_KEY_ENTER || ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
            break;
        }
    }
    al_destroy_event_queue(fila_vitoria);
}

void get_movement(bool *teclas, float *x, float *y) {
    if (teclas[ALLEGRO_KEY_W] && *y > 0) *y -= 4;
    if (teclas[ALLEGRO_KEY_S] && *y + TAM_JOGADOR < ALTURA - ALTURA_CHAO) *y += 4;
    if (teclas[ALLEGRO_KEY_A] && *x > 0) *x -= 4;
    if (teclas[ALLEGRO_KEY_D] && *x + TAM_JOGADOR < LARGURA) *x += 4;
}

void atualizar_velocidade_jogador(Jogador *jogador) {
    if (jogador->hp > 4) {
        jogador->velocidade = 1.0 + (jogador->hp - 4) * 0.25;
    } else if (jogador->hp > 0) {
        jogador->velocidade = 1.0 - (4 - jogador->hp) * 0.25;
    } else {
        jogador->velocidade = 0; // Jogador para se estiver sem vida
    }
}

void aplicar_dano_jogador(Jogador *jogador, Inimigo inimigos[], int max_inimigos) {
    // Se o jogador está intangível, ele não pode levar dano.
    if (jogador->intangivel_timer > 0) {
        return;
    }

    for (int i = 0; i < max_inimigos; i++) {
        if (inimigos[i].ativo) {
            // Verificação de colisão simples (quadrado com círculo aproximado como quadrado)
            bool colidiu = (jogador->x < inimigos[i].x + TAM_INIMIGO &&
                          jogador->x + TAM_JOGADOR > inimigos[i].x &&
                          jogador->y < inimigos[i].y + TAM_INIMIGO &&
                          jogador->y + TAM_JOGADOR > inimigos[i].y);

            if (colidiu) {
                jogador->hp--; // Perde 1 de vida
                jogador->intangivel_timer = 180; // Fica intangível por 3 segundos (180 frames / 60 FPS)

                // Aplica knockback (empurrão para trás)
                // Se o inimigo está à direita, empurra o jogador para a esquerda.
                if (inimigos[i].x > jogador->x) {
                    jogador->x -= 30;
                } else { // Se o inimigo está à esquerda, empurra para a direita.
                    jogador->x += 30;
                }

                // Atualiza a velocidade do jogador baseada na nova vida
                atualizar_velocidade_jogador(jogador);

                // Sai do loop para não receber múltiplos danos no mesmo frame.
                break;
            }
        }
    }
}

void disparar_cuspe(Cuspe cuspes[], float x, float y, float alvo_x, float alvo_y) {
    for (int i = 0; i < MAX_CUSPES; i++) {
        if (!cuspes[i].ativo) {
            cuspes[i].ativo = true;
            cuspes[i].x = x;
            cuspes[i].y = y;

            float dist_x = alvo_x - x;
            float dist_y = alvo_y - y;

            // Define quanto tempo (em segundos) o projétil levará para atingir o alvo
            const float tempo_de_voo_segundos = 1.2f;
            const float tempo_de_voo_frames = tempo_de_voo_segundos * 60.0f; // Converte para frames

            // Gravidade que afeta o cuspe (um pouco mais fraca que a do jogador)
            const float G_CUSPE = GRAVIDADE * 0.8f;

            // Calcula a velocidade horizontal necessária
            cuspes[i].dx = dist_x / tempo_de_voo_frames;

            // Calcula a velocidade vertical inicial necessária para compensar a gravidade
            // e atingir a altura do jogador no tempo certo.
            cuspes[i].dy = (dist_y / tempo_de_voo_frames) - (0.5f * G_CUSPE * (tempo_de_voo_frames - 1));

            break;
        }
    }
}

void update_cuspes(Cuspe cuspes[], int max, float camera_x) {
    for (int i = 0; i < max; i++) {
        if (cuspes[i].ativo) {
            // Aplica gravidade para criar o efeito de parábola
            cuspes[i].dy += GRAVIDADE * 0.8; // Um pouco menos de gravidade que o jogador
            cuspes[i].x += cuspes[i].dx;
            cuspes[i].y += cuspes[i].dy;

            // Desativa se sair da tela
            if (cuspes[i].x < camera_x - 20 || cuspes[i].x > camera_x + LARGURA + 20 || cuspes[i].y > ALTURA) {
                cuspes[i].ativo = false;
            }
        }
    }
}

void disparar_tiro(Tiro tiros[], float x, float y, float dx, float dy) {
    for (int i = 0; i < MAX_TIROS; i++) {
        if (!tiros[i].ativo) {
            tiros[i].x = x;
            tiros[i].y = y;
            tiros[i].dx = dx;
            tiros[i].dy = dy;
            tiros[i].ativo = true;
            break;
        }
    }
}

void update_tiros(Tiro tiros[], int max, float camera_x) {
    for (int i = 0; i < max; i++) {
        if (tiros[i].ativo) {
            tiros[i].x += tiros[i].dx;
            tiros[i].y += tiros[i].dy;
            if (tiros[i].x > camera_x + LARGURA + 20 || tiros[i].x < camera_x - 20 ||
                tiros[i].y < -20 || tiros[i].y > ALTURA + 20) {
                    tiros[i].ativo = false; // Desativa o tiro se sair da tela
                }
            }
        }
    }

void criar_item_municao(ItemMunicao itens[], float x, float y) {
    for (int i = 0; i < MAX_MUNICAO_ITENS; i++) {
        if (!itens[i].ativo) {
            itens[i].ativo = true;
            itens[i].x = x;
            itens[i].y = y + TAM_INIMIGO; // Dropa um pouco abaixo do centro do inimigo
            itens[i].tempo_de_vida = 600; // Item dura 10 segundos
            break;
        }
    }
}

void update_itens_municao(ItemMunicao itens[]) {
    for (int i = 0; i < MAX_MUNICAO_ITENS; i++) {
        if (itens[i].ativo) {
            itens[i].tempo_de_vida--;
            if (itens[i].tempo_de_vida <= 0) {
                itens[i].ativo = false;
            }
        }
    }
}

void update_stamina_jogador(Jogador *jogador){
if (!jogador->ativo) return;

    switch (jogador->estado_stamina) {
        case CORRENDO:
            jogador->timer_stamina--;
            if (jogador->timer_stamina <= 0) {
                jogador->estado_stamina = CANSADO;
                jogador->timer_stamina = 4 * 60; // 4 segundos cansado
            }
            break;
        case CANSADO:
            jogador->timer_stamina--;
            if (jogador->timer_stamina <= 0) {
                jogador->estado_stamina = COOLDOWN;
                jogador->timer_stamina = 8 * 60; // 8 segundos de cooldown
            }
            break;
        case COOLDOWN:
            jogador->timer_stamina--;
            if (jogador->timer_stamina <= 0) {
                jogador->estado_stamina = NORMAL;
            }
            break;
        case NORMAL:
            // Nenhum timer ativo, esperando o input
            break;
    }
}

void desenha_barra_stamina(Jogador* jogador) {
    float max_stamina_largura = 100;
    float stamina_ratio = 0;
    ALLEGRO_COLOR stamina_cor = al_map_rgb(0, 200, 200); // Ciano

    if (jogador->estado_stamina == CORRENDO) {
        stamina_ratio = (float)jogador->timer_stamina / (8.0 * 60.0);
    } else if (jogador->estado_stamina == CANSADO) {
        stamina_cor = al_map_rgb(200, 200, 0); // Amarelo
        stamina_ratio = 1.0;
    } else if (jogador->estado_stamina == COOLDOWN) {
        stamina_cor = al_map_rgb(100, 100, 100); // Cinza
        stamina_ratio = 1.0 - ((float)jogador->timer_stamina / (8.0 * 60.0));
    } else { // NORMAL
        stamina_ratio = 1.0;
    }

    // Desenha o preenchimento da barra
    al_draw_filled_rectangle(10, 35, 10 + (max_stamina_largura * stamina_ratio), 45, stamina_cor);
    // Desenha a borda da barra
    al_draw_rectangle(10, 35, 10 + max_stamina_largura, 45, al_map_rgb(255, 255, 255), 1);
}

void gerar_inimigo(Inimigo inimigos[], int max, float camera_x) {
    for (int i = 0; i < max; i++) {
        if (!inimigos[i].ativo) {
            if (rand() % 2 == 0) {
                inimigos[i].tipo = ZUMBI_ANDARILHO;
                inimigos[i].hp_max = 100;
                inimigos[i].hp = 100;
                inimigos[i].sprite_index = rand() % 3;
            } else {
                inimigos[i].tipo = ZUMBI_CUSPIDOR;
                inimigos[i].hp_max = 200;
                inimigos[i].hp = 200;
                inimigos[i].sprite_index = 0;
            }

            inimigos[i].anim_frame_atual = 0;
            inimigos[i].anim_timer = 0;
            inimigos[i].timer_anim_dano = 0;
            inimigos[i].morrendo = false;
            
            inimigos[i].x = camera_x + LARGURA + (rand() % 100);
            inimigos[i].y = ALTURA - ALTURA_CHAO - 65;
            inimigos[i].dy = 0;
            inimigos[i].dx = 0;
            inimigos[i].no_chao = true;
            inimigos[i].ativo = true;
            inimigos[i].timer_ataque = 60 + (rand() % 120);
            break;
        }
    }
}

int dist(float x1, float y1, float x2, float y2) {
    return (int)sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

void update_inimigos(Inimigo inimigos[], Tiro tiros[], Cuspe cuspes[], Jogador *jogador, int *zumbis_mortos, float camera_x, ItemMunicao itens_municao[]) {
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        if (inimigos[i].ativo) {
            // Se o inimigo está na animação de morte, ele não se move nem ataca.
            // Apenas a animação é atualizada.
            if (inimigos[i].morrendo) {
                ZumbiAnimSet* anim_set = &animacoes_zumbis[inimigos[i].sprite_index];
                if (inimigos[i].anim_frame_atual >= anim_set->num_frames_morte - 1) {
                    inimigos[i].ativo = false;
                }
                atualiza_animacao_inimigo(&inimigos[i], jogador); // <<-- CORREÇÃO: Passa o jogador
                continue;
            }

            // Decrementa o timer da animação de dano, se estiver ativo.
            if (inimigos[i].timer_anim_dano > 0) {
                inimigos[i].timer_anim_dano--;
            }

            // --- Lógica de IA e Movimento (sem alterações) ---
            switch (inimigos[i].tipo) {
                case ZUMBI_ANDARILHO:
                    // --- CORREÇÃO: Define a direção do zumbi ---
                    if (inimigos[i].x < jogador->x) {
                        inimigos[i].x += VELOCIDADE_ZUMBI;
                        inimigos[i].direcao = 1; // Virado para a direita
                    } else if (inimigos[i].x > jogador->x) {
                        inimigos[i].x -= VELOCIDADE_ZUMBI;
                        inimigos[i].direcao = -1; // Virado para a esquerda
                    }
                    break;                case ZUMBI_CUSPIDOR:
                // Calcula a distância horizontal até o jogador
                    float dist_para_jogador_x = jogador->x - inimigos[i].x;
                    float dist_abs = fabs(dist_para_jogador_x);

                    // Define a "zona de conforto" para atirar (em pixels)
                    const float DIST_MIN = 250.0;
                    const float DIST_MAX = 400.0;

                    // --- Lógica de Movimento ---
                    if (dist_abs > DIST_MAX) {
                        // Se está muito longe, se aproxima 
                        if (dist_para_jogador_x > 0) inimigos[i].x += VELOCIDADE_ZUMBI * 0.75;
                        else inimigos[i].x -= VELOCIDADE_ZUMBI;
                    } 
                    else if (dist_abs < DIST_MIN) {
                        // Se está muito perto, se afasta lentamente
                        if (dist_para_jogador_x > 0) inimigos[i].x -= VELOCIDADE_ZUMBI * 0.5;
                        else inimigos[i].x += VELOCIDADE_ZUMBI;
                    } 
                    else {
                        // --- Lógica de Ataque (se estiver na distância ideal) ---
                        if (inimigos[i].timer_ataque > 0) {
                            inimigos[i].timer_ataque--;
                        } else {
                            // Chama a função de cuspir passando a posição X e Y do jogador
                            disparar_cuspe(cuspes, inimigos[i].x, inimigos[i].y, jogador->x, jogador->y);
                            inimigos[i].timer_ataque = 180; // Reinicia o cooldown para 3 segundos
                        }
                    }
                    break;
            }

            // --- Lógica de Dano (Tiros do Jogador) ---
            for (int j = 0; j < MAX_TIROS; j++) {
                if (tiros[j].ativo) {
                    // CONDIÇÃO DE COLISÃO: Verifica se o centro do tiro está dentro da caixa do inimigo
                    bool colidiu = (tiros[j].x > inimigos[i].x && tiros[j].x < inimigos[i].x + 41 && // 41 é a largura da sprite
                                  tiros[j].y > inimigos[i].y && tiros[j].y < inimigos[i].y + 65);  // 65 é a altura

                    if (colidiu) {
                        tiros[j].ativo = false; // O tiro some
                        inimigos[i].hp -= 20;   // Inimigo perde vida
                        inimigos[i].timer_anim_dano = 15; // Ativa a animação de dano por 1/4s

                        if (inimigos[i].hp <= 0) {
                            inimigos[i].morrendo = true; // Inicia o processo de morte
                            (*zumbis_mortos)++;
                            if (rand() % 2 == 0) {
                                criar_item_municao(itens_municao, inimigos[i].x, inimigos[i].y);
                            }
                        }
                        break; // Um tiro só pode acertar um inimigo
                    }
                }
            }

            // --- Física e Animação ---
            inimigos[i].dy += GRAVIDADE;
            inimigos[i].y += inimigos[i].dy;

            float chao_y = ALTURA - ALTURA_CHAO - 65; // Usa a altura da sprite
            if (inimigos[i].y > chao_y) {
                inimigos[i].y = chao_y;
                inimigos[i].no_chao = true;
                inimigos[i].dy = 0;
            }

            atualiza_animacao_inimigo(&inimigos[i], jogador);
        }
    }
}

void update_jogador(Jogador *jogador) {
    // A física de gravidade continua a mesma
    jogador->dy += GRAVIDADE;
    jogador->y += jogador->dy;

    // --- CORREÇÃO: Usa a altura redimensionada para calcular a posição do chão ---
    // Pega a altura do frame atual da animação
    float altura_original_frame = jogador->anim_sequencia_atual[jogador->anim_frame_atual].h;
    // Multiplica pela escala para obter a altura final na tela
    float altura_final_sprite = altura_original_frame * PLAYER_ESCALA;
    
    // Calcula a posição exata do chão para esta sprite
    float chao_y = ALTURA - ALTURA_CHAO - altura_final_sprite;

    if (jogador->y > chao_y) {
        jogador->y = chao_y; // Garante que o jogador não passe do chão
        jogador->no_chao = true;
        jogador->dy = 0;
    } else {
        jogador->no_chao = false;
    }
}

bool update_chefe(Chefe *chefe, Jogador *jogador, Tiro tiros[], Cuspe cuspes[], float mundo_largura) {
    // Se o chefe não estiver ativo, não faz nada
    if (!chefe->ativo) return false;

    // --- 1. LÓGICA DE MOVIMENTO ---
    // O chefe se move de um lado para o outro na arena
    chefe->x += chefe->dx;
    float chefe_largura = 100;
    
    // Agora esta lógica usará o 'mundo_largura' correto
    if (chefe->dx > 0 && chefe->x + chefe_largura > mundo_largura) {
        chefe->dx *= -1;
    } else if (chefe->dx < 0 && chefe->x < mundo_largura - LARGURA) {
        chefe->dx *= -1;
    }

    // --- 2. LÓGICA DE ATAQUE (RAJADA DE 6 TIROS) ---
    // Se não está no meio de uma rajada, espera o cooldown principal
    if (chefe->tiros_na_rajada <= 0) {
        if (chefe->timer_ataque_principal > 0) {
            chefe->timer_ataque_principal--;
        } else {
            // É hora de começar uma nova rajada!
            chefe->tiros_na_rajada = 6;
            chefe->timer_rajada = 0; // Atira o primeiro tiro imediatamente
            chefe->timer_ataque_principal = 240; // Próxima rajada em 4 segundos
        }
    }

    // Se uma rajada está em andamento, dispara os tiros
    if (chefe->tiros_na_rajada > 0) {
        if (chefe->timer_rajada > 0) {
            chefe->timer_rajada--;
        } else {
            // Posição da "boca" do chefe
            float boca_x = chefe->x + (chefe_largura / 2);
            float boca_y = chefe->y + 100; // Ajuste conforme seu futuro sprite

            // Dispara um cuspe mirando no jogador
            disparar_cuspe(cuspes, boca_x, boca_y, jogador->x, jogador->y);
            
            chefe->tiros_na_rajada--; // Um tiro a menos na rajada
            chefe->timer_rajada = 15; // Próximo tiro da rajada em 1/4 de segundo
        }
    }

    // --- 3. LÓGICA DE DANO (CHEFE RECEBENDO TIROS) ---
    for (int i = 0; i < MAX_TIROS; i++) {
        if (tiros[i].ativo) {
            // Colisão com o retângulo do chefe
            if (tiros[i].x > chefe->x && tiros[i].x < chefe->x + chefe_largura &&
                tiros[i].y > chefe->y && tiros[i].y < chefe->y + 150) // 150 é a altura do chefe
            {
                tiros[i].ativo = false;
                chefe->hp -= 20; // Dano do tiro do jogador
                printf("HP do Chefe: %d\n", chefe->hp); // Debug
            }
        }
    }

    // --- 4. CONDIÇÃO DE DERROTA ---
    if (chefe->hp <= 0) {
        chefe->ativo = false;
        printf("CHEFE DERROTADO!\n");
        return true; // Retorna true para sinalizar a vitória
    }

    return false; // Chefe continua vivo
}

void pular(Jogador *jogador) {
    if (jogador->no_chao) {
        jogador->dy = PULO_FORCA; // Aplica a força do pulo
        jogador->no_chao = false; // O jogador não está mais no chão
    }
}

void desenhar_jogador(Jogador* jogador, ALLEGRO_BITMAP* sprite_sheet, float camera_x, int frame_counter) {
    if (!jogador->anim_sequencia_atual) return;

    if (jogador->intangivel_timer > 0 && jogador->ativo && (frame_counter / 6) % 2 != 0) {
        return;
    }

    // Pega os dados do frame atual, incluindo os offsets
    Frame frame_atual = jogador->anim_sequencia_atual[jogador->anim_frame_atual];
    float sx = frame_atual.x;
    float sy = frame_atual.y;
    float sw = frame_atual.w;
    float sh = frame_atual.h;

    float dest_w = sw * PLAYER_ESCALA;
    float dest_h = sh * PLAYER_ESCALA;

    // A posição final na tela é a posição do jogador MENOS o deslocamento do sprite
    float draw_x = jogador->x;
    float draw_y = jogador->y;

    int flags = (jogador->direcao == -1) ? ALLEGRO_FLIP_HORIZONTAL : 0;

    al_draw_scaled_bitmap(sprite_sheet,
                             sx, sy, sw, sh, // Fonte: posição e tamanho do frame na spritesheet
                             draw_x - camera_x, draw_y, // Destino: posição na tela
                             dest_w, dest_h, // Tamanho final do sprite
                             flags); // Flags de desenho (flip horizontal se necessário)
}

void desenhar_inimigos(Assets* assets, Inimigo inimigos[], float camera_x) {
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        if (inimigos[i].ativo) {
           
            float draw_x = inimigos[i].x - camera_x;
            float draw_y = inimigos[i].y;
           
            if (inimigos[i].tipo == ZUMBI_ANDARILHO) {
                // Pega o conjunto de animações e o bitmap corretos
                int index = inimigos[i].sprite_index;
                ZumbiAnimSet* anim_set = &animacoes_zumbis[index];
                ALLEGRO_BITMAP* sprite_sheet = assets->zumbi_sprites[index];

                // Determina qual sequência de frames usar
                Frame* sequencia_atual;
                if (inimigos[i].morrendo) sequencia_atual = anim_set->morte;
                else if (inimigos[i].timer_anim_dano > 0) sequencia_atual = anim_set->dano;
                // ... adicione a lógica de ataque aqui ...
                else sequencia_atual = anim_set->andar;

                // Pega os dados do frame específico e desenha
                Frame frame = sequencia_atual[inimigos[i].anim_frame_atual];
                int flags = (inimigos[i].direcao == -1) ? ALLEGRO_FLIP_HORIZONTAL : 0;
                al_draw_bitmap_region(sprite_sheet, frame.x, frame.y, frame.w, frame.h,
                                      inimigos[i].x - camera_x, inimigos[i].y, flags);
            } else {
                al_draw_filled_circle(draw_x, draw_y, TAM_INIMIGO, al_map_rgb(0, 255, 0));

            }
        }
    }
}

void desenhar_chefe(Chefe *chefe, float camera_x) {
    // Se o chefe não estiver ativo, a função não faz nada
    if (!chefe->ativo) {
        return;
    }

    // Define as dimensões e a posição do chefe na tela
    float chefe_largura = 100;
    float chefe_altura = 150;
    float tela_x = chefe->x - camera_x;
    float tela_y = chefe->y;

    // --- Desenha o corpo do chefe ---
    // A cor muda de vermelho para preto conforme o HP diminui, dando um feedback visual do dano
    float hp_ratio = (float)chefe->hp / chefe->hp_max;
    if (hp_ratio < 0) hp_ratio = 0; // Garante que não fique negativo
    unsigned char red_component = 150 + (105 * hp_ratio); // Varia de 255 a 150

    al_draw_filled_rectangle(tela_x, tela_y,
                             tela_x + chefe_largura, tela_y + chefe_altura,
                             al_map_rgb(red_component, 0, 0));

}

void aplicar_dano_cuspes(Jogador *jogador, Cuspe cuspes[]) {
    // Se o jogador já está intangível, não precisa verificar a colisão
    if (jogador->intangivel_timer > 0) {
        return;
    }

    // Itera por todos os possíveis cuspes na tela
    for (int i = 0; i < MAX_CUSPES; i++) {
        if (cuspes[i].ativo) {
            // Lógica de colisão: verifica a distância entre o centro do jogador e o centro do cuspe
            float raio_jogador = TAM_JOGADOR / 2.0;
            float raio_cuspe = 7.0; // Raio do círculo do cuspe que definimos no desenho

            float dist_x = (jogador->x + raio_jogador) - cuspes[i].x;
            float dist_y = (jogador->y + raio_jogador) - cuspes[i].y;
            float distancia_centros = sqrt(dist_x * dist_x + dist_y * dist_y);

            // Se a distância for menor que a soma dos raios, houve colisão
            if (distancia_centros < raio_jogador + raio_cuspe) {
                
                cuspes[i].ativo = false; // O cuspe que atingiu desaparece

                // Aplica os mesmos efeitos de dano que o toque
                jogador->hp--;
                jogador->intangivel_timer = 180; // Fica intangível por 3 segundos

                // Aplica um pequeno knockback
                if (cuspes[i].dx > 0) { // Se o cuspe veio da esquerda
                    jogador->x += 20; // Empurra o jogador para a direita
                } else { // Se o cuspe veio da direita
                    jogador->x -= 20; // Empurra para a esquerda
                }

                atualizar_velocidade_jogador(jogador);

                // Sai do loop para que o jogador só tome um dano por vez
                break;
            }
        }
    }
}

int inicia_jogo(ALLEGRO_DISPLAY* disp, Assets* assets) {
    al_init_primitives_addon();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();

    float camera_x = 0;
    float mundo_largura = al_get_bitmap_width(assets->fundo);

    EstadoDaFase estado_atual = FASE_NORMAL;

    Chefe chefe;
    chefe.ativo = false;
    chefe.x = mundo_largura - LARGURA / 2;
    chefe.y = ALTURA - ALTURA_CHAO - 150;
    chefe.hp_max = 1000;
    chefe.hp = chefe.hp_max;
    chefe.dx = -1.0;
    chefe.timer_ataque_principal = 120;
    chefe.tiros_na_rajada = 0;
    chefe.timer_rajada = 0;

    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_display_event_source(disp));
    al_register_event_source(fila, al_get_timer_event_source(timer));

    float altura_jogador_atual = anim_idle[0].h * PLAYER_ESCALA;

    Jogador jogador = {
        .x = LARGURA / 2, 
        .y = ALTURA - ALTURA_CHAO - altura_jogador_atual, 
        .dy = 0,
        .no_chao = true, 
        .direcao = 1, 
        .hp = 8, 
        .velocidade = 2.0, 
        .intangivel_timer = 0, 
        .ativo = true,
        .municao =  30,
        .estado_stamina = NORMAL, 
        .timer_stamina = 0,
        .anim_sequencia_atual = anim_idle, 
        .num_frames_na_anim = num_frames_idle,
        .anim_frame_atual = 0, 
        .anim_timer = 0,
        .timer_anim_tiro = 0,
        .timer_anim_reload = 0,
        .timer_anim_morte = -1,
    };


    Tiro tiros[MAX_TIROS] = {0};
    Inimigo inimigos[MAX_INIMIGOS] = {0};
    Cuspe cuspes[MAX_CUSPES] = {0};
    ItemMunicao itens_municao[MAX_MUNICAO_ITENS] = {0};

    int frames_inimigo = 0;
    int zumbis_mortos = 0;
    bool teclas[ALLEGRO_KEY_MAX] = {false};
    ALLEGRO_EVENT ev;
    bool rodando = true;
    bool redesenhar = true;
    int frame_counter = 0;

    al_start_timer(timer);

    while (rodando) {
        al_wait_for_event(fila, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            frame_counter++;

            if (jogador.timer_anim_tiro > 0) jogador.timer_anim_tiro--;

            if (jogador.timer_anim_reload > 0)jogador.timer_anim_reload--;
    

            // Se a vida do jogador chegou a zero e ele ainda estava ativo...
            if (jogador.hp <= 0 && jogador.ativo) {
                jogador.ativo = false; // Desativa o jogador (para input e movimento)
                jogador.timer_anim_morte = 120; // Define um timer de 2 segundos para a tela de Game Over
            }

            // Se o jogador está morto, decrementa o timer de Game Over
            if (!jogador.ativo && jogador.timer_anim_morte > 0) {
                jogador.timer_anim_morte--;
            }

            // Se o timer de Game Over chegou a zero, mostra a tela e sai
            if (jogador.timer_anim_morte == 0) {
                tela_game_over(assets->font, zumbis_mortos);
                rodando = false;
            }

            //SE O CHEFE MORREU
            if(estado_atual == BATALHA_CHEFE && chefe.hp <= 0) {
                al_rest(0.5);
                tela_vitoria(assets->font);
                rodando = false;
            }

            if(jogador.ativo){
                if (jogador.intangivel_timer > 0) jogador.intangivel_timer--;

                update_stamina_jogador(&jogador); 
                float multiplicador_sprint = 1.0;
                if (jogador.estado_stamina == CORRENDO) multiplicador_sprint = 2.0;
                else if (jogador.estado_stamina == CANSADO) multiplicador_sprint = 1.0 / 3.0;
                
                aplicar_dano_jogador(&jogador, inimigos, MAX_INIMIGOS);
                aplicar_dano_cuspes(&jogador, cuspes);

                float limite_esquerdo = (estado_atual == BATALHA_CHEFE) ? mundo_largura - LARGURA : 0;
                float limite_direito = mundo_largura - TAM_JOGADOR;

                float velocidade_final = VELOCIDADE_BASE * jogador.velocidade * multiplicador_sprint;
                if (teclas[ALLEGRO_KEY_A] && jogador.x > limite_esquerdo) {
                    jogador.x -= velocidade_final;
                } else if (teclas[ALLEGRO_KEY_D] && jogador.x < limite_direito) {
                    jogador.x += velocidade_final;
                }
            }

            bool esta_andando = (teclas[ALLEGRO_KEY_A] || teclas[ALLEGRO_KEY_D]);
            atualiza_animacao_jogador(&jogador, esta_andando);

            // Atualiza a física do jogador
            update_jogador(&jogador);

            // Atualiza a câmera
            if (estado_atual == FASE_NORMAL) {
                camera_x = jogador.x - LARGURA / 2.0;
                if (camera_x < 0) camera_x = 0;
                if (camera_x > mundo_largura - LARGURA) camera_x = mundo_largura - LARGURA;

                if (jogador.x > mundo_largura - LARGURA) {
                    estado_atual = BATALHA_CHEFE;
                    chefe.ativo = true;
                }
            } else {
                camera_x = mundo_largura - LARGURA;
            }

            update_tiros(tiros, MAX_TIROS, camera_x);
            update_cuspes(cuspes, MAX_CUSPES, camera_x);
            update_itens_municao(itens_municao);
            update_inimigos(inimigos, tiros, cuspes, &jogador, &zumbis_mortos, camera_x, itens_municao);
            
            if(chefe.ativo){
                update_chefe(&chefe, &jogador, tiros, cuspes, mundo_largura);
            }

            //verifica a frequencia com base na fase do jogo
            int frequencia_inimigos = (estado_atual == BATALHA_CHEFE) ? 60 : 120;
            if (++frames_inimigo >= frequencia_inimigos) {
                gerar_inimigo(inimigos, MAX_INIMIGOS, camera_x);
                frames_inimigo = 0;
            }

            redesenhar = true; // Marca que precisa redesenhar a tela
        }
        /*
        ==========================
        LOGICA DE INPUT DO TECLADO
        ==========================
        */
        else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) { rodando = false; }
        
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            teclas[ev.keyboard.keycode] = true;
            if (ev.keyboard.keycode == ALLEGRO_KEY_A) jogador.direcao = -1;
            else if (ev.keyboard.keycode == ALLEGRO_KEY_D) jogador.direcao = 1;

            if (ev.keyboard.keycode == ALLEGRO_KEY_W) pular(&jogador);
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) rodando = false;

            if (ev.keyboard.keycode == ALLEGRO_KEY_C && jogador.estado_stamina == NORMAL && (teclas[ALLEGRO_KEY_A] || teclas[ALLEGRO_KEY_D])) {
                jogador.estado_stamina = CORRENDO;
                jogador.timer_stamina = 8 * 60;
            }
            
            if (ev.keyboard.keycode == ALLEGRO_KEY_R) {
                for (int i = 0; i < MAX_MUNICAO_ITENS; i++) {
                    if (itens_municao[i].ativo) {
                        float dist_x = fabs(jogador.x - itens_municao[i].x);
                        


                        if(dist_x < TAM_JOGADOR) {
                            jogador.municao += 18; // Adiciona 5 munições
                            itens_municao[i].ativo = false; // Remove o item de munição

                            jogador.timer_anim_reload = 60; // Tempo de recarga
                            jogador.anim_frame_atual = 0; // Reseta o frame da animação de recarga                            

                            printf("Munição coletada! Total: %d\n", jogador.municao);
                            break;
                        }
                    }
                }
            }
            if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                if(jogador.municao > 0) {
                    jogador.municao--;

                    jogador.timer_anim_tiro = 10;
                    jogador.anim_frame_atual = 0;

                    const float VELOCIDADE_TIRO = 10.0;
                    float dir_x = 0, dir_y = 0;

                    if (teclas[ALLEGRO_KEY_W]) dir_y -= 1;
                    if (teclas[ALLEGRO_KEY_S]) dir_y += 1;
                    if (teclas[ALLEGRO_KEY_A]) dir_x -= 1;
                    if (teclas[ALLEGRO_KEY_D]) dir_x += 1;

                    if (dir_x == 0 && dir_y == 0) dir_x = jogador.direcao;

                    // Normaliza a direção do tiro
                    float magnitude = sqrt(dir_x * dir_x + dir_y * dir_y);
                    float vel_x = 0, vel_y = 0;

                    if (magnitude > 0) {
                        vel_x = (dir_x / magnitude) * VELOCIDADE_TIRO;
                        vel_y = (dir_y / magnitude) * VELOCIDADE_TIRO;
                    }

                    float tiro_x = jogador.x + TAM_JOGADOR / 2.0;
                    float tiro_y = jogador.y + TAM_JOGADOR / 2.0;

                    disparar_tiro(tiros, tiro_x, tiro_y, vel_x, vel_y);
                }else{
                    printf("Sem munição!\n");
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            teclas[ev.keyboard.keycode] = false;
            if (ev.keyboard.keycode == ALLEGRO_KEY_A && teclas[ALLEGRO_KEY_D]) jogador.direcao = 1;
            else if (ev.keyboard.keycode == ALLEGRO_KEY_D && teclas[ALLEGRO_KEY_A]) jogador.direcao = -1;
        }
    
        /*
        ========================
            LÓGICA DE DESENHO
        ========================
        */
        if (redesenhar && al_is_event_queue_empty(fila)) {
            al_draw_bitmap_region(assets->fundo, camera_x, 0, LARGURA, ALTURA, 0, 0, 0);

            desenhar_jogador(&jogador, assets->player_sprite, camera_x, frame_counter);

            desenhar_chefe(&chefe, camera_x);
            
            desenhar_inimigos(assets, inimigos, camera_x);
            
            for (int i = 0; i < MAX_MUNICAO_ITENS; i++) {
                if(itens_municao[i].ativo){
                    al_draw_filled_rectangle(itens_municao[i].x - camera_x, itens_municao[i].y, itens_municao[i].x - camera_x + 15, itens_municao[i].y + 15, al_map_rgb(255, 255, 0));
                }
            }

            for (int i = 0; i < MAX_CUSPES; i++) {
                if (cuspes[i].ativo) {
                    al_draw_filled_circle(cuspes[i].x - camera_x, cuspes[i].y, 7, al_map_rgb(0, 255, 0));
                }
            }

            for (int i = 0; i < MAX_TIROS; i++) {
                if (tiros[i].ativo) {
                    al_draw_filled_rectangle(tiros[i].x - camera_x, tiros[i].y,
                                             tiros[i].x - camera_x + TAM_TIROS, tiros[i].y + 5,
                                             al_map_rgb(255, 255, 0)); // Cor amarela para o tiro
                }
            }



            int hp_largura = al_get_bitmap_width(assets->hp_sprite);
            for(int i = 0; i < jogador.hp; i++) {
                al_draw_bitmap(assets->hp_sprite, 10 + (i * (hp_largura + 5)), 10, 0);
            }

            desenha_barra_stamina(&jogador);

            al_draw_textf(assets->font, al_map_rgb(255, 255, 255), 10, 45, 0,
                          "Municao: %d", jogador.municao);

            al_flip_display();
            redesenhar = false;
        }
    }
    
    al_destroy_timer(timer);
    al_destroy_event_queue(fila);
    return 0;
}

int inicia_configuracoes(ALLEGRO_DISPLAY* disp) {
    al_clear_to_color(al_map_rgb(0, 0, 255));
    al_flip_display();
    al_rest(2.0);  // Espera 2 segundos para mostrar a tela preta
    return 0;
}

int inicia_instrucoes(ALLEGRO_DISPLAY* disp) {
    al_clear_to_color(al_map_rgb(0, 255, 0));
    al_flip_display();
    al_rest(2.0);  // Espera 2 segundos para mostrar a tela preta
    return 0;
}

// ==========================================================================
// FUNÇÃO MAIN (PONTO DE ENTRADA)
// ==========================================================================
int main()
{
    // --- 1. INICIALIZAÇÃO DO ALLEGRO E ADDONS ---
    if (!al_init()) {
        fprintf(stderr, "Falha ao inicializar o Allegro.\n");
        return -1;
    }
    al_install_keyboard();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    
    // Chama a função para preencher os dados de animação
    inicializa_dados_animacao_zumbis();

    // --- 2. CRIAÇÃO DOS OBJETOS PRINCIPAIS ---
    ALLEGRO_DISPLAY* disp = al_create_display(LARGURA, ALTURA);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    if (!disp || !queue) {
        fprintf(stderr, "Falha ao criar display ou fila de eventos.\n");
        return -1;
    }

    // --- 3. CARREGAMENTO DE TODOS OS ASSETS ---
    Assets assets;

    assets.fundo = al_load_bitmap("fundo.png");
    assets.font = al_load_ttf_font("font.ttf", 40, 0);
    assets.hp_sprite = al_load_bitmap("HP_sprites.png");
    assets.caveira_sprite = al_load_bitmap("caveira.png");
    assets.player_sprite = al_load_bitmap("sprite_dave.png");
    
    // Carrega a sprite do zumbi cuspidor
    assets.zumbi_cuspidor_sprite = al_load_bitmap("sprites_ze.png");

    // Carrega as 4 sprites dos zumbis andarilhos
    assets.zumbi_sprites[0] = al_load_bitmap("sprites_z1.png");
    assets.zumbi_sprites[1] = al_load_bitmap("sprites_z2.png");
    assets.zumbi_sprites[2] = al_load_bitmap("sprites_z3.png");

    // --- Verificação de Erros no Carregamento ---
    if (!assets.fundo || !assets.font || !assets.hp_sprite || !assets.caveira_sprite || !assets.player_sprite || !assets.zumbi_cuspidor_sprite) {
        fprintf(stderr, "Erro ao carregar um asset básico (fundo, fonte, ui, ou jogador).\n");
        return -1;
    }
    for (int i = 0; i < 3; i++) {
        if (!assets.zumbi_sprites[i]) {
            fprintf(stderr, "Erro ao carregar o arquivo sprites_z%d.png\n", i + 1);
            return -1;
        }
    }

    // --- 4. REGISTRO DE FONTES DE EVENTOS ---
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));

    // --- 5. LOOP PRINCIPAL DA APLICAÇÃO (MENU) ---
    bool app_rodando = true;
    while (app_rodando) {
        int escolha = loop_do_menu(queue, &assets);

        switch (escolha) {
            case 0: // JOGAR
                // Chama a função de jogo passando um ponteiro para a struct de assets
                inicia_jogo(disp, &assets);
                break;
            case 1: // CONFIGURAÇÕES (Placeholder)
                printf("Opcao de configuracoes ainda nao implementada.\n");
                al_rest(1.0);
                break;
            case 2: // SAIR
                app_rodando = false;
                break;
        }
    }

    // --- 6. LIMPEZA DE TODOS OS RECURSOS ---
    al_destroy_font(assets.font);
    al_destroy_bitmap(assets.fundo);
    al_destroy_bitmap(assets.hp_sprite);
    al_destroy_bitmap(assets.caveira_sprite);
    al_destroy_bitmap(assets.player_sprite);
    al_destroy_bitmap(assets.zumbi_cuspidor_sprite);
    for(int i = 0; i < 3; i++) {
        al_destroy_bitmap(assets.zumbi_sprites[i]);
    }
    al_destroy_display(disp);
    al_destroy_event_queue(queue);

    return 0;
}