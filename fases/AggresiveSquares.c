#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

//compilação: gcc AggresiveSquares.c -o seu_jogo $(pkg-config --libs allegro-5 allegro_font-5 allegro_ttf-5 allegro_image-5 allegro_primitives-5) -lm

#define OPCOES 3

#define LARGURA 800
#define ALTURA 600
#define MAX_TIROS 100
#define MAX_INIMIGOS 50
#define TAM_JOGADOR 30
#define TAM_TIROS 10
#define TAM_INIMIGO 20

typedef struct {
    float x, y;
    float dx;
    bool ativo;
} Tiro;

typedef struct {
    float x, y;
    float dx, dy;
    float r, g, b;
    bool ativo;
} Inimigo;

int inicia_jogo(ALLEGRO_DISPLAY* disp) {
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);
    ALLEGRO_EVENT ev;

    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_display_event_source(disp));
    al_register_event_source(fila, al_get_timer_event_source(timer));
    al_start_timer(timer);

    Tiro tiros[MAX_TIROS] = {0};
    Inimigo inimigos[MAX_INIMIGOS] = {0};
    float jogador_x = LARGURA / 2, jogador_y = ALTURA / 2;
    bool teclas[ALLEGRO_KEY_MAX] = {false};

    double tempo_ultimo_spawn = al_get_time();

    srand(time(NULL));

    bool rodando = true;
    while (rodando) {
        while (al_get_next_event(fila, &ev)) {
            if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                rodando = false;
            } else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                teclas[ev.keyboard.keycode] = true;
                if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                    for (int i = 0; i < MAX_TIROS; i++) {
                        if (!tiros[i].ativo) {
                            tiros[i].x = jogador_x + TAM_JOGADOR / 2;
                            tiros[i].y = jogador_y + TAM_JOGADOR / 2;
                            tiros[i].dx = 10;
                            tiros[i].ativo = true;
                            break;
                        }
                    }
                } else if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    rodando = false;
                }
            } else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
                teclas[ev.keyboard.keycode] = false;
            }
        }

        // Movimento do jogador
        if (teclas[ALLEGRO_KEY_W]) jogador_y -= 4;
        if (teclas[ALLEGRO_KEY_S]) jogador_y += 4;
        if (teclas[ALLEGRO_KEY_A]) jogador_x -= 4;
        if (teclas[ALLEGRO_KEY_D]) jogador_x += 4;

        // Atualiza tiros
        for (int i = 0; i < MAX_TIROS; i++) {
            if (tiros[i].ativo) {
                tiros[i].x += tiros[i].dx;
                if (tiros[i].x > LARGURA) tiros[i].ativo = false;
            }
        }

        // Spawna inimigo a cada 30 segundos
        if (al_get_time() - tempo_ultimo_spawn >= 30) {
            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (!inimigos[i].ativo) {
                    inimigos[i].x = rand() % LARGURA;
                    inimigos[i].y = rand() % ALTURA;
                    inimigos[i].dx = ((rand() % 5) - 2);
                    inimigos[i].dy = ((rand() % 5) - 2);
                    inimigos[i].r = 0;
                    inimigos[i].g = 0;
                    inimigos[i].b = 255;
                    inimigos[i].ativo = true;
                    break;
                }
            }
            tempo_ultimo_spawn = al_get_time();
        }

        // Atualiza inimigos
        for (int i = 0; i < MAX_INIMIGOS; i++) {
            if (inimigos[i].ativo) {
                inimigos[i].x += inimigos[i].dx;
                inimigos[i].y += inimigos[i].dy;

                // Colisão com tiro
                for (int j = 0; j < MAX_TIROS; j++) {
                    if (tiros[j].ativo) {
                        float dx = inimigos[i].x - tiros[j].x;
                        float dy = inimigos[i].y - tiros[j].y;
                        float dist = sqrt(dx * dx + dy * dy);
                        if (dist < TAM_INIMIGO) {
                            tiros[j].ativo = false;
                            // Clareia a cor (menos azul, mais branco)
                            if (inimigos[i].b > 0) inimigos[i].b -= 50;
                            if (inimigos[i].r < 255) inimigos[i].r += 25;
                            if (inimigos[i].g < 255) inimigos[i].g += 25;

                            // Se já está branco, "morre"
                            if (inimigos[i].b <= 0 && inimigos[i].r >= 255 && inimigos[i].g >= 255) {
                                inimigos[i].ativo = false;
                            }
                        }
                    }
                }
            }
        }

        // Desenha tudo
        al_clear_to_color(al_map_rgb(0, 0, 0));

        // Jogador
        al_draw_filled_rectangle(jogador_x, jogador_y,
                                 jogador_x + TAM_JOGADOR, jogador_y + TAM_JOGADOR,
                                 al_map_rgb(255, 128, 0));

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

        al_flip_display();
        al_rest(1.0 / 30.0);
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

int main() {
    al_init();
    al_install_keyboard();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_DISPLAY* disp = al_create_display(800, 600);
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
                    inicia_jogo(disp);
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
