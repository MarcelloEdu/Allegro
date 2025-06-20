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

typedef struct {
    ALLEGRO_BITMAP* fundo;
    ALLEGRO_BITMAP* hp_sprite;
    ALLEGRO_BITMAP* caveira_sprite;
    ALLEGRO_BITMAP* player_sprite;
    ALLEGRO_FONT* font;
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
    float x, y;
    float dx, dy;
    bool ativo;
    bool no_chao;

    TipoInimigo tipo;       // Identifica o tipo de zumbi
    int hp;                 // Pontos de vida atuais
    int hp_max;             // Pontos de vida máximos
    int timer_ataque;       // Cooldown para o próximo ataque
} Inimigo;

/*===========================*/
/*DADOS DA ANIMAÇÃO DO JOGADOR*/
/*============================*/



// --- Animações ---
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

Frame anim_tiro[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {0, 699, 41, 65},  // Frame 1
    {10, 699, 50, 65}, // Frame 2
    {20, 699, 104, 65}, // Frame 3
    {30, 699, 104, 65}, // Frame 4
    {40, 699, 104, 65}, // Frame 5
    {50, 699, 104, 65},   // Frame 6
    {60, 699, 41, 65}, // Frame 7
    {70, 699, 41, 65}, // Frame 8
    {80, 699, 41, 65}, // Frame 9
    {90, 699, 41, 65},   // Frame 10
    {100, 699, 41, 65},  // Frame 11
    {110, 699, 41, 65} // Frame 12
};

Frame anim_reload[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {0, 828, 41, 65},  // Frame 1
    {10, 828, 41, 65}, // Frame 2
    {20, 828, 41, 65}, // Frame 3
    {30, 828, 41, 65}, // Frame 4
    {40, 828, 41, 65}, // Frame 5
    {50, 828, 41, 65}, // Frame 6
    {60, 828, 41, 65}, // Frame 7
    {70, 828, 41, 65}, // Frame 8
    {80, 828, 41, 65}, // Frame 9
    {90, 828, 41, 65}, // Frame 10
    {100, 828, 41, 65}, // Frame 11
    {110, 828, 41, 65} // Frame 12
};

Frame anim_morte[MAX_FRAMES_POR_ANIM] = {
    // {x, y, w, h)
    {0, 1210, 50, 65},  // Frame 1
    {10, 1210, 50, 65}, // Frame 2
    {20, 1210, 59, 65}, // Frame 3
    {30, 1210, 81, 65} // Frame 4
};

int num_frames_idle = 6;
int num_frames_andando = 8;
int num_frames_correndo = 8;
int num_frames_pulo = 9;
int num_frames_tiro = 12;
int num_frames_reload = 12;
int num_frames_morte = 4;

void atualiza_animacao_jogador(Jogador* jogador, bool esta_andando) {
    // Ponteiros para as animações definidas globalmente
    Frame* proxima_anim = anim_idle;
    int    proximo_num_frames = num_frames_idle;

    // Decide qual será a próxima animação
    if (!jogador->no_chao) {
        proxima_anim = anim_pulo;
        proximo_num_frames = num_frames_andando;
    } else if (esta_andando) {
        proxima_anim = anim_andando;
        proximo_num_frames = num_frames_andando;
    } else {
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
    const int velocidade_anim = 8;
    jogador->anim_timer++;
    if (jogador->anim_timer >= velocidade_anim) {
        jogador->anim_timer = 0;
        jogador->anim_frame_atual = (jogador->anim_frame_atual + 1) % jogador->num_frames_na_anim;
    }
}

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

            // Escolhe um tipo de inimigo aleatoriamente
            if(rand() % 2 == 0){
                inimigos[i].tipo = ZUMBI_ANDARILHO;
                inimigos[i].hp_max = 100;
                inimigos[i].hp = 100;
            }else {
                inimigos[i].tipo = ZUMBI_CUSPIDOR;
                inimigos[i].hp_max = 200;
                inimigos[i].hp = 200;
            }

            inimigos[i].x = camera_x + LARGURA + (rand() % 100);
            inimigos[i].y = ALTURA - ALTURA_CHAO - TAM_INIMIGO;
            inimigos[i].dy = 0;
            inimigos[i].dx = 0;
            inimigos[i].no_chao = true;
            inimigos[i].ativo = true;
            inimigos[i].timer_ataque = 60 + (rand() % 120); // Cooldown inicial de 1 a 3 segundos
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
            // --- Lógica de IA baseada no tipo de inimigo ---
            switch (inimigos[i].tipo) {
                case ZUMBI_ANDARILHO:
                    // Persegue o jogador
                    if (inimigos[i].x < jogador->x) inimigos[i].x += VELOCIDADE_ZUMBI;
                    else if (inimigos[i].x > jogador->x) inimigos[i].x -= VELOCIDADE_ZUMBI;
                    break;

                case ZUMBI_CUSPIDOR:
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

            // Física e Desativação (comum a todos)
            inimigos[i].dy += GRAVIDADE;
            inimigos[i].y += inimigos[i].dy;
            float chao_y = ALTURA - ALTURA_CHAO - TAM_INIMIGO;
            if (inimigos[i].y > chao_y) {
                inimigos[i].y = chao_y;
                inimigos[i].no_chao = true;
                inimigos[i].dy = 0;
            }
            if (inimigos[i].x < camera_x - TAM_INIMIGO - 50) inimigos[i].ativo = false;

            // Dano dos tiros do JOGADOR no inimigo
            for (int j = 0; j < MAX_TIROS; j++) {
                if (tiros[j].ativo && (dist(tiros[j].x, tiros[j].y, inimigos[i].x, inimigos[i].y) < TAM_INIMIGO)) {
                    tiros[j].ativo = false;
                    inimigos[i].hp -= 20; // Dano do tiro do jogador
                    if (inimigos[i].hp <= 0) {
                        inimigos[i].ativo = false;
                        (*zumbis_mortos)++;

                        if (rand() % 2 == 0) {
                        criar_item_municao(itens_municao, inimigos[i].x, inimigos[i].y);
                        }
                    }
                }
            }
        }
    }
}

void update_jogador(Jogador *jogador) {
    jogador->dy += GRAVIDADE; // Aplica a gravidade
    jogador->y += jogador->dy; // Atualiza a posição vertical do jogador

    float altura_atual = jogador->anim_sequencia_atual[jogador->anim_frame_atual].h * PLAYER_ESCALA;
    float chao_y = ALTURA - ALTURA_CHAO - TAM_JOGADOR;
    
    if (jogador->y > chao_y) {
        jogador->y = chao_y; // Garante que o jogador não passe do chão
        jogador->no_chao = true; // O jogador está no chão
        jogador->dy = 0; // Reseta a velocidade vertical
    } else {
        jogador->no_chao = false; // O jogador está no ar
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
    if (!jogador->ativo || !jogador->anim_sequencia_atual) return;

    if (jogador->intangivel_timer > 0 && (frame_counter / 6) % 2 != 0) {
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

int inicia_jogo(ALLEGRO_DISPLAY* disp, ALLEGRO_FONT* font, ALLEGRO_BITMAP* fundo, ALLEGRO_BITMAP* hp_sprite, ALLEGRO_BITMAP* caveira_sprite, ALLEGRO_BITMAP* sprite_dave) {
    al_init_primitives_addon();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();

    float camera_x = 0;
    float mundo_largura = al_get_bitmap_width(fundo);

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
        .anim_timer = 0
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

            //SE O JOGADOR MORREU
            if (!jogador.ativo || jogador.hp <= 0) {
                al_rest(10.0);
                tela_game_over(font, zumbis_mortos);
                rodando = false;
            }

            //SE O CHEFE MORREU
            if(estado_atual == BATALHA_CHEFE && chefe.hp <= 0) {
                al_rest(0.5);
                tela_vitoria(font);
                rodando = false;
            }

            if (jogador.intangivel_timer > 0) jogador.intangivel_timer--;

            update_stamina_jogador(&jogador); 
            float multiplicador_sprint = 1.0;
            if (jogador.estado_stamina == CORRENDO) multiplicador_sprint = 2.0;
            else if (jogador.estado_stamina == CANSADO) multiplicador_sprint = 1.0 / 3.0;
            
            aplicar_dano_jogador(&jogador, inimigos, MAX_INIMIGOS);
            aplicar_dano_cuspes(&jogador, cuspes);

            float limite_esquerdo = (estado_atual == BATALHA_CHEFE) ? mundo_largura - LARGURA : 0;
            float limite_direito = mundo_largura - TAM_JOGADOR;

            // Aplica o movimento, respeitando os limites dinâmicos
            if (teclas[ALLEGRO_KEY_A] && jogador.x > limite_esquerdo) {
                jogador.x -= VELOCIDADE_BASE * jogador.velocidade * multiplicador_sprint;
            } else if (teclas[ALLEGRO_KEY_D] && jogador.x < limite_direito) {
                jogador.x += VELOCIDADE_BASE * jogador.velocidade * multiplicador_sprint;
            }
            
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

            update_jogador(&jogador);

            bool esta_andando = (teclas[ALLEGRO_KEY_A] || teclas[ALLEGRO_KEY_D]);
            atualiza_animacao_jogador(&jogador, esta_andando);

            update_tiros(tiros, MAX_TIROS, camera_x);
            update_cuspes(cuspes, MAX_CUSPES, camera_x);
            update_itens_municao(itens_municao);
            update_inimigos(inimigos, tiros, cuspes, &jogador, &zumbis_mortos, camera_x, itens_municao);
            
            if(chefe.ativo){
                update_chefe(&chefe, &jogador, tiros, cuspes, mundo_largura);
            }

            
            frames_inimigo++;
            //verifica a frequencia com base na fase do jogo
            int frequencia_inimigos = (estado_atual == BATALHA_CHEFE) ? 60 : 120;
            if (frames_inimigo >= frequencia_inimigos) {
                gerar_inimigo(inimigos, MAX_INIMIGOS, camera_x);
                frames_inimigo = 0;
            }
            redesenhar = true;
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
                            printf("Munição coletada! Total: %d\n", jogador.municao);
                            break;
                        }
                    }
                }
            }
            if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                if(jogador.municao > 0) {
                    jogador.municao--;
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
            al_draw_bitmap_region(fundo, camera_x, 0, LARGURA, ALTURA, 0, 0, 0);

            desenhar_jogador(&jogador, sprite_dave, camera_x, frame_counter);

            desenhar_chefe(&chefe, camera_x);
            
            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativo) {
                    ALLEGRO_COLOR cor;
                    if (inimigos[i].tipo == ZUMBI_ANDARILHO) {
                        cor = al_map_rgb(0, 0, 255); // Azul
                    } else {
                        cor = al_map_rgb(0, 255, 0); // Verde
                    }

                    al_draw_filled_circle(inimigos[i].x - camera_x, inimigos[i].y, TAM_INIMIGO, cor);
                }
            }

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



            int hp_largura = al_get_bitmap_width(hp_sprite);
            for(int i = 0; i < jogador.hp; i++) {
                al_draw_bitmap(hp_sprite, 10 + (i * (hp_largura + 5)), 10, 0);
            }

            desenha_barra_stamina(&jogador);

            al_draw_textf(font, al_map_rgb(255, 255, 255), 10, 45, 0,
                          "Municao: %d", jogador.municao);


            const float tamanho_caveira = 40.0f; 

            // 2. Pega as dimensões originais da imagem da caveira.
            float sw = al_get_bitmap_width(caveira_sprite);
            float sh = al_get_bitmap_height(caveira_sprite);

            // 3. Desenha cada caveira usando o tamanho arbitrário.
            for (int i = 0; i < zumbis_mortos; i++) {
                // Calcula a posição X para alinhar à direita
                float draw_x = (LARGURA - 10 - tamanho_caveira) - (i * (tamanho_caveira + 5));
                float draw_y = 10;

                al_draw_scaled_bitmap(caveira_sprite, // A imagem da caveira
                                      0, 0,           // Posição (x, y) do recorte (a imagem inteira)
                                      sw, sh,         // Tamanho (largura, altura) do recorte (a imagem inteira)
                                      draw_x, draw_y, // Posição (x, y) final na tela
                                      tamanho_caveira, tamanho_caveira, // Usa o nosso tamanho arbitrário
                                      0);             // Flags (sem espelhamento)
            }

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

int main()
{
    // --- 1. INICIALIZAÇÃO DO ALLEGRO ---
    al_init();
    al_install_keyboard();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();

    ALLEGRO_DISPLAY* disp = al_create_display(LARGURA, ALTURA);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    // --- 2. CARREGAMENTO DOS ASSETS ---
    Assets assets;
    assets.fundo = al_load_bitmap("fundo.png");
    assets.font = al_load_ttf_font("font.ttf", 40, 0);
    assets.hp_sprite = al_load_bitmap("HP_sprites.png");
    assets.caveira_sprite = al_load_bitmap("caveira.png");
    assets.player_sprite = al_load_bitmap("sprite_dave.png");

    // Adicione verificações de erro para todos os assets aqui!
    if (!assets.player_sprite) { 
        fprintf(stderr, "Erro ao carregar sprite do jogador.\n");
        return -1;
    }

    if (!assets.fundo) {
        fprintf(stderr, "Erro ao carregar fundo.\n");
        return -1;
    }

    if (!assets.font) {
        fprintf(stderr, "Erro ao carregar fonte.\n");
        return -1;
    }

    if (!assets.hp_sprite) {
        fprintf(stderr, "Erro ao carregar sprite de HP.\n");
        return -1;
    }

    if (!assets.caveira_sprite) {
        fprintf(stderr, "Erro ao carregar sprite de caveira.\n");
        return -1;
    }



    // --- 3. REGISTRO DE FONTES DE EVENTOS ---
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));

    // --- 4. LOOP PRINCIPAL DO JOGO ---
    bool jogando = true;
    while (jogando) {
        // Chama o loop do menu e espera uma escolha
        int escolha = loop_do_menu(queue, &assets);

        switch (escolha) {
            case 0: // JOGAR
                // Chama a função principal do jogo
                inicia_jogo(disp, assets.font, assets.fundo, assets.hp_sprite, assets.caveira_sprite, assets.player_sprite);
                break;
            case 1: // CONFIGURAÇÕES
                // (Função de configurações pode ser adicionada aqui no futuro)
                printf("Opcao de configuracoes ainda nao implementada.\n");
                al_rest(1.0); // Pausa para o usuário ver a mensagem
                break;
            case 2: // SAIR
                jogando = false;
                break;
        }
    }

    // --- 5. LIMPEZA DE RECURSOS ---
    al_destroy_font(assets.font);
    al_destroy_bitmap(assets.fundo);
    al_destroy_bitmap(assets.hp_sprite);
    al_destroy_bitmap(assets.caveira_sprite);
    al_destroy_bitmap(assets.player_sprite);
    al_destroy_display(disp);
    al_destroy_event_queue(queue);

    return 0;
}
