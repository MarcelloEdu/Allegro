#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

//compilação: gcc AggresiveSquares.c -o jogo $(pkg-config --libs allegro-5 allegro_font-5 allegro_ttf-5 allegro_image-5 allegro_primitives-5) -lm

#define OPCOES 3

#define LARGURA 800
#define ALTURA 600
#define ALTURA_CHAO 150
#define MAX_TIROS 100
#define MAX_INIMIGOS 50
#define TAM_JOGADOR 30
#define TAM_TIROS 10
#define TAM_INIMIGO 20

#define VELOCIDADE_ZUMBI 1.0

#define GRAVIDADE 0.5
#define PULO_FORCA -10.0

typedef struct {
    float x, y;
    float dy; //velocidade do jogador
    bool no_chao;
    int direcao;
} Jogador;

typedef struct {
    float x, y;
    float dx;
    bool ativo;
} Tiro;

typedef struct {
    float x, y;
    float dx, dy;
    float r, g, b; //vai ser uma representação do HP 
    bool ativo;
    bool no_chao;
} Inimigo;

void get_movement(bool *teclas, float *x, float *y) {
    if (teclas[ALLEGRO_KEY_W] && *y > 0) *y -= 4;
    if (teclas[ALLEGRO_KEY_S] && *y + TAM_JOGADOR < ALTURA - ALTURA_CHAO) *y += 4;
    if (teclas[ALLEGRO_KEY_A] && *x > 0) *x -= 4;
    if (teclas[ALLEGRO_KEY_D] && *x + TAM_JOGADOR < LARGURA) *x += 4;
}

// A assinatura agora recebe a direção
void disparar_tiro(Tiro tiros[], float x, float y, int direcao) {
    for (int i = 0; i < MAX_TIROS; i++) {
        if (!tiros[i].ativo) {
            tiros[i].x = x;
            tiros[i].y = y;
            tiros[i].dx = 10 * direcao;
            tiros[i].ativo = true;
            break;
        }
    }
}

void update_tiros(Tiro tiros[], int max) {
    for (int i = 0; i < max; i++) {
        if (tiros[i].ativo) {
            tiros[i].x += tiros[i].dx;
            if (tiros[i].x > LARGURA || tiros[i].x < 0) {
                tiros[i].ativo = false;
            }
        }
    }
}

void gerar_inimigo(Inimigo inimigos[], int max) {
    for (int i = 0; i < max; i++) {
        if (!inimigos[i].ativo) {
            // simulando que estão vindo "de frente" no cenário.
            inimigos[i].x = LARGURA;

            //posicionando o zumbi no chão.
            inimigos[i].y = ALTURA - ALTURA_CHAO - TAM_INIMIGO;
            inimigos[i].dy = 0;
            inimigos[i].no_chao = true;

            // Cor/vida inicial
            inimigos[i].r = 0;
            inimigos[i].g = 0;
            inimigos[i].b = 255;
            inimigos[i].ativo = true;
            break;
        }
    }
}

void update_inimigos(Inimigo inimigos[], Tiro tiros[], Jogador *jogador, int *zumbis_mortos, int max_inimigos, int max_tiros) {    for (int i = 0; i < max_inimigos; i++) {
        if (inimigos[i].ativo) {
            // --- Lógica de Movimento do Zumbi (sem alteração) ---
            if (inimigos[i].x < jogador->x) {
                inimigos[i].x += VELOCIDADE_ZUMBI;
            } else if (inimigos[i].x > jogador->x) {
                inimigos[i].x -= VELOCIDADE_ZUMBI;
            }

            inimigos[i].dy += GRAVIDADE;
            inimigos[i].y += inimigos[i].dy;

            float chao_y = ALTURA - ALTURA_CHAO - TAM_INIMIGO;
            if (inimigos[i].y > chao_y) {
                inimigos[i].y = chao_y;
                inimigos[i].no_chao = true;
                inimigos[i].dy = 0;
            } else {
                inimigos[i].no_chao = false;
            }

            if (inimigos[i].x < -TAM_INIMIGO - 20 || inimigos[i].x > LARGURA + 20) {
                inimigos[i].ativo = false;
            }

            // --- Lógica de Colisão com Tiros ---
            for (int j = 0; j < max_tiros; j++) {
                if (tiros[j].ativo) {
                    float dx = inimigos[i].x - tiros[j].x;
                    float dy = inimigos[i].y - tiros[j].y;
                    float dist = sqrt(dx * dx + dy * dy);

                    if (dist < TAM_INIMIGO) {
                        tiros[j].ativo = false;
                        if (inimigos[i].b > 0) inimigos[i].b -= 50;
                        if (inimigos[i].r < 255) inimigos[i].r += 25;
                        if (inimigos[i].g < 255) inimigos[i].g += 25;

                        // Se o inimigo morrer...
                        if (inimigos[i].b <= 0 && inimigos[i].r >= 255 && inimigos[i].g >= 255) {
                            inimigos[i].ativo = false;
                            (*zumbis_mortos)++; // Mantém apenas o contador de mortes.
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

    float chao_y = ALTURA - ALTURA_CHAO - TAM_JOGADOR;
    if (jogador->y > chao_y) {
        jogador->y = chao_y; // Garante que o jogador não passe do chão
        jogador->no_chao = true; // O jogador está no chão
        jogador->dy = 0; // Reseta a velocidade vertical
    } else {
        jogador->no_chao = false; // O jogador está no ar
    }
}

void pular(Jogador *jogador) {
    if (jogador->no_chao) {
        jogador->dy = PULO_FORCA; // Aplica a força do pulo
        jogador->no_chao = false; // O jogador não está mais no chão
    }
}

void desenhar_jogador(Jogador *jogador) {
    al_draw_filled_rectangle(
        jogador->x, jogador->y,
        jogador->x + TAM_JOGADOR, jogador -> y + TAM_JOGADOR,
        al_map_rgb(255, 128, 0)); // Cor laranja
}

void desenhar_jogo(float jogador_x, float jogador_y, Tiro tiros[], Inimigo inimigos[]) {
    al_clear_to_color(al_map_rgb(0, 0, 0));

    al_draw_filled_rectangle(0, ALTURA - ALTURA_CHAO,
                            LARGURA, ALTURA, 
                            al_map_rgb(34, 139, 34));  // verde grama


    al_draw_filled_rectangle(jogador_x, jogador_y,
                             jogador_x + TAM_JOGADOR, jogador_y + TAM_JOGADOR,
                             al_map_rgb(255, 128, 0));

    for (int i = 0; i < MAX_TIROS; i++) {
        if (tiros[i].ativo) {
            al_draw_filled_rectangle(tiros[i].x, tiros[i].y,
                                     tiros[i].x + TAM_TIROS, tiros[i].y + 5,
                                     al_map_rgb(255, 255, 0));
        }
    }

    for (int i = 0; i < MAX_INIMIGOS; i++) {
        if (inimigos[i].ativo) {
            ALLEGRO_COLOR cor = al_map_rgb((int)inimigos[i].r, (int)inimigos[i].g, (int)inimigos[i].b);
            al_draw_filled_circle(inimigos[i].x, inimigos[i].y, TAM_INIMIGO, cor);
        }
    }

    al_flip_display();
}

int inicia_jogo(ALLEGRO_DISPLAY* disp, ALLEGRO_FONT* font) {
    al_init_primitives_addon(); // Necessário para desenhar formas

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();

    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_display_event_source(disp));
    al_register_event_source(fila, al_get_timer_event_source(timer));

    // Inicializa jogador com o campo 'direcao'
    Jogador jogador = {
        .x = LARGURA / 2,
        .y = ALTURA - ALTURA_CHAO - TAM_JOGADOR,
        .dy = 0,
        .no_chao = true,
        .direcao = 1 // Começa virado para a direita
    };

    // Inicializa arrays de tiros e inimigos
    Tiro tiros[MAX_TIROS] = {0};
    Inimigo inimigos[MAX_INIMIGOS] = {0};
    int frames_inimigo = 0;

    // Variável para o contador de mortes
    int zumbis_mortos = 0;

    // Controles do jogo
    bool teclas[ALLEGRO_KEY_MAX] = {false};
    ALLEGRO_EVENT ev;
    bool rodando = true;
    bool redesenhar = true;

    al_start_timer(timer);

    while (rodando) {
        al_wait_for_event(fila, &ev);

        // --- LÓGICA DO TIMER (ATUALIZAÇÃO DO ESTADO DO JOGO) ---
        if (ev.type == ALLEGRO_EVENT_TIMER) {
            // Aplica o movimento baseado nas teclas pressionadas
            if (teclas[ALLEGRO_KEY_A] && jogador.x > 0) {
                jogador.x -= 4;
            } else if (teclas[ALLEGRO_KEY_D] && jogador.x + TAM_JOGADOR < LARGURA) {
                jogador.x += 4;
            }

            // Atualiza a física do jogador e dos objetos
            update_jogador(&jogador);
            update_tiros(tiros, MAX_TIROS);
            update_inimigos(inimigos, tiros, &jogador, &zumbis_mortos, MAX_INIMIGOS, MAX_TIROS);

            // Gera novos inimigos periodicamente
            frames_inimigo++;
            if (frames_inimigo >= 120) { // Gera um inimigo a cada 2 segundos
                gerar_inimigo(inimigos, MAX_INIMIGOS);
                frames_inimigo = 0;
            }

            redesenhar = true;
        }

        // --- LÓGICA DE EVENTOS DE ENTRADA (INPUT) ---
        else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            teclas[ev.keyboard.keycode] = true;

            // Atualiza a direção imediatamente ao pressionar a tecla
            if (ev.keyboard.keycode == ALLEGRO_KEY_A) {
                jogador.direcao = -1;
            } else if (ev.keyboard.keycode == ALLEGRO_KEY_D) {
                jogador.direcao = 1;
            }

            // Ações de pressionar (pulo, tiro, sair)
            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                rodando = false;

            if (ev.keyboard.keycode == ALLEGRO_KEY_W)
                pular(&jogador);

            if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                float tiro_x;
                if (jogador.direcao == 1) { // Direita
                    tiro_x = jogador.x + TAM_JOGADOR;
                } else { // Esquerda
                    tiro_x = jogador.x - TAM_TIROS;
                }
                disparar_tiro(tiros, tiro_x, jogador.y + (TAM_JOGADOR / 2) - (TAM_TIROS / 2), jogador.direcao);
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            teclas[ev.keyboard.keycode] = false;

            // Atualiza a direção se uma tecla for solta e a outra continuar pressionada
            if (ev.keyboard.keycode == ALLEGRO_KEY_A && teclas[ALLEGRO_KEY_D]) {
                jogador.direcao = 1;
            } else if (ev.keyboard.keycode == ALLEGRO_KEY_D && teclas[ALLEGRO_KEY_A]) {
                jogador.direcao = -1;
            }
        }

        // --- LÓGICA DE DESENHO ---
        if (redesenhar && al_is_event_queue_empty(fila)) {
            al_clear_to_color(al_map_rgb(0, 0, 0));

            // Chão verde
            al_draw_filled_rectangle(0, ALTURA - ALTURA_CHAO, LARGURA, ALTURA, al_map_rgb(34, 139, 34));

            // Jogador
            desenhar_jogador(&jogador);

            // Tiros
            for (int i = 0; i < MAX_TIROS; i++) {
                if (tiros[i].ativo) {
                    al_draw_filled_rectangle(tiros[i].x, tiros[i].y,
                                             tiros[i].x + TAM_TIROS, tiros[i].y + 5,
                                             al_map_rgb(255, 255, 0));
                }
            }

            // Inimigos
            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativo) {
                    ALLEGRO_COLOR cor = al_map_rgb((int)inimigos[i].r, (int)inimigos[i].g, (int)inimigos[i].b);
                    al_draw_filled_circle(inimigos[i].x, inimigos[i].y, TAM_INIMIGO, cor);
                }
            }

            al_draw_textf(font, al_map_rgb(255, 255, 255), 10, 10, 0, "MORTES: %d", zumbis_mortos);


            // Contador de zumbis mortos (bolinhas brancas)
            for (int i = 0; i < zumbis_mortos; i++) {
                al_draw_filled_circle(LARGURA - 15 - (i * 15), 25, 5, al_map_rgb(255, 255, 255));
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
    al_init();
    al_install_keyboard();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_DISPLAY* disp = al_create_display(LARGURA, ALTURA);
    ALLEGRO_BITMAP* fundo = al_load_bitmap("orig_big.png");
    ALLEGRO_FONT* font = al_load_ttf_font("font.ttf", 40, 0);

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    ALLEGRO_EVENT event;
    int opcao_selecionada = 0;
    const char* opcoes[OPCOES] = {"JOGAR", "CONFIGURAÇÕES", "INSTRUÇÕES"};

    al_start_timer(timer);

    while (1) {
        al_wait_for_event(queue, &event);
        
        
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }

        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_UP) {
                opcao_selecionada = (opcao_selecionada - 1 + OPCOES) % OPCOES;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
                opcao_selecionada = (opcao_selecionada + 1) % OPCOES;
            } else if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
                if (opcao_selecionada == 0) {
                    inicia_jogo(disp, font);
                } else if (opcao_selecionada == 1) {
                    inicia_configuracoes(disp);
                } else if (opcao_selecionada == 2) {
                    inicia_instrucoes(disp);
                }
            }
        }

        else if (event.type == ALLEGRO_EVENT_TIMER) {
            al_draw_bitmap(fundo, 0, 0, 0);

            for (int i = 0; i < OPCOES; i++) {
                ALLEGRO_COLOR cor = (i == opcao_selecionada) ? al_map_rgb(255, 0, 0) : al_map_rgb(0, 0, 0);
                al_draw_text(font, cor, 400, 200 + i * 60, ALLEGRO_ALIGN_CENTER, opcoes[i]);
            }

            al_flip_display();
        }
    }

    al_destroy_font(font);
    al_destroy_bitmap(fundo);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(disp);

    return 0;
}
