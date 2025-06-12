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

#define LARGURA 1920
#define ALTURA 1080
#define ALTURA_CHAO 150
#define MAX_TIROS 100
#define MAX_INIMIGOS 50
#define MAX_CUSPES 50
#define TAM_JOGADOR 30
#define TAM_TIROS 10
#define TAM_INIMIGO 20

#define VELOCIDADE_ZUMBI 1.0
#define VELOCIDADE_BASE 2.0

#define GRAVIDADE 0.5
#define PULO_FORCA -10.0



typedef struct {
    float x, y;
    float dy; //velocidade do jogador
    bool no_chao;
    int direcao;
    int hp;
    float velocidade; // velocidade do jogador
    int intangivel_timer;

} Jogador;

typedef struct {
    float x, y;
    float dx;
    float dy;
    bool ativo;
} Tiro;

typedef struct {
    float x, y;
    float dx, dy;
    bool ativo;
} Cuspe;


typedef enum {
    ZUMBI_ANDARILHO,
    ZUMBI_CUSPIDOR
} TipoInimigo;

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

void tela_game_over(ALLEGRO_FONT* font, int zumbis_mortos) {
    // Cria uma fila de eventos local apenas para esta tela
    ALLEGRO_EVENT_QUEUE* fila_game_over = al_create_event_queue();
    al_register_event_source(fila_game_over, al_get_keyboard_event_source());

    bool sair = false;
    while (!sair) {
        // --- Desenho da Tela ---
        al_clear_to_color(al_map_rgb(20, 0, 0)); // fundo_menu vermelho bem escuro

        // Texto "GAME OVER"
        al_draw_text(font, al_map_rgb(200, 0, 0), LARGURA / 2, ALTURA / 3, ALLEGRO_ALIGN_CENTER, "GAME OVER");

        // Exibe o número de zumbis derrotados
        al_draw_textf(font, al_map_rgb(255, 255, 255), LARGURA / 2, ALTURA / 2, ALLEGRO_ALIGN_CENTER, "Zumbis derrotados: %d", zumbis_mortos);

        // Instrução para o jogador
        al_draw_text(font, al_map_rgb(150, 150, 150), LARGURA / 2, ALTURA - 100, ALLEGRO_ALIGN_CENTER, "Pressione ENTER para voltar ao menu");

        al_flip_display();

        // --- Lógica de Eventos ---
        ALLEGRO_EVENT ev;
        al_wait_for_event(fila_game_over, &ev);

        if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            // Se o jogador pressionar ENTER ou ESC, sai da tela de Game Over
            if (ev.keyboard.keycode == ALLEGRO_KEY_ENTER || ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                sair = true;
            }
        }
    }

    // Limpa a fila de eventos local
    al_destroy_event_queue(fila_game_over);
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

void disparar_cuspe(Cuspe cuspes[], float x, float y, float alvo_x) {
    for (int i = 0; i < MAX_CUSPES; i++) {
        if (!cuspes[i].ativo) {
            cuspes[i].x = x;
            cuspes[i].y = y;
            cuspes[i].ativo = true;

            // Lógica da Parábola:
            // 1. Calcula a distância horizontal até o jogador
            float dist_x = alvo_x - x;

            // 2. Define uma velocidade horizontal na direção do jogador
            // O tempo para atingir o alvo é dist_x / vel_x. Vamos definir uma vel_x.
            float vel_x = (dist_x > 0) ? 4.0 : -4.0;
            if (dist_x < 1) vel_x = 0; // Evita divisão por zero
            
            // 3. Define uma velocidade vertical inicial para cima
            float vel_y = -8.0; // Pulo inicial do cuspe

            cuspes[i].dx = vel_x;
            cuspes[i].dy = vel_y;
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

void update_inimigos(Inimigo inimigos[], Tiro tiros[], Cuspe cuspes[], Jogador *jogador, int *zumbis_mortos, float camera_x) {
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
                    // Fica parado e ataca à distância
                    if (inimigos[i].timer_ataque > 0) {
                        inimigos[i].timer_ataque--;
                    } else {
                        // Se o jogador estiver na frente e a uma distância razoável
                        if (fabs(jogador->x - inimigos[i].x) < LARGURA / 2) {
                            disparar_cuspe(cuspes, inimigos[i].x, inimigos[i].y, jogador->x);
                            inimigos[i].timer_ataque = 180; // Cooldown de 3 segundos
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

int inicia_jogo(ALLEGRO_DISPLAY* disp, ALLEGRO_FONT* font, ALLEGRO_BITMAP* fundo) {
    al_init_primitives_addon();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* fila = al_create_event_queue();

    float camera_x = 0;
    float mundo_largura = al_get_bitmap_width(fundo);

    al_register_event_source(fila, al_get_keyboard_event_source());
    al_register_event_source(fila, al_get_display_event_source(disp));
    al_register_event_source(fila, al_get_timer_event_source(timer));

    Jogador jogador = {
        .x = LARGURA / 2, .y = ALTURA - ALTURA_CHAO - TAM_JOGADOR, .dy = 0,
        .no_chao = true, .direcao = 1, .hp = 8, .velocidade = 2.0, .intangivel_timer = 0
    };

    Tiro tiros[MAX_TIROS] = {0};
    Inimigo inimigos[MAX_INIMIGOS] = {0};
    Cuspe cuspes[MAX_CUSPES] = {0};
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

            //verifica se o jogador morreu
            if (jogador.hp <= 0) {
                al_rest(0.5);
                tela_game_over(font, zumbis_mortos);
                rodando = false;
            }

            if (jogador.intangivel_timer > 0) jogador.intangivel_timer--;
            
            aplicar_dano_jogador(&jogador, inimigos, MAX_INIMIGOS);

            if (teclas[ALLEGRO_KEY_A] && jogador.x > 0) 
            {
                jogador.x -= VELOCIDADE_BASE * jogador.velocidade;
            } 
            else if (teclas[ALLEGRO_KEY_D] && jogador.x < mundo_largura - TAM_JOGADOR) 
            {
                jogador.x += VELOCIDADE_BASE * jogador.velocidade;
            }

            camera_x = jogador.x - LARGURA / 2.0;
            if (camera_x < 0) camera_x = 0;
            if (camera_x > mundo_largura - LARGURA) camera_x = mundo_largura - LARGURA;

            update_jogador(&jogador);
            update_tiros(tiros, MAX_TIROS, camera_x);
            update_cuspes(cuspes, MAX_CUSPES, camera_x);
            update_inimigos(inimigos, tiros, cuspes, &jogador, &zumbis_mortos, camera_x);

            frames_inimigo++;
            if (frames_inimigo >= 120) {
                gerar_inimigo(inimigos, MAX_INIMIGOS, camera_x);
                frames_inimigo = 0;
            }
            redesenhar = true;
        }
        else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            rodando = false;
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            teclas[ev.keyboard.keycode] = true;
            if (ev.keyboard.keycode == ALLEGRO_KEY_A) jogador.direcao = -1;
            else if (ev.keyboard.keycode == ALLEGRO_KEY_D) jogador.direcao = 1;

            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) rodando = false;
            if (ev.keyboard.keycode == ALLEGRO_KEY_W) pular(&jogador);

            if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
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
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            teclas[ev.keyboard.keycode] = false;
            if (ev.keyboard.keycode == ALLEGRO_KEY_A && teclas[ALLEGRO_KEY_D]) jogador.direcao = 1;
            else if (ev.keyboard.keycode == ALLEGRO_KEY_D && teclas[ALLEGRO_KEY_A]) jogador.direcao = -1;
        }

        if (redesenhar && al_is_event_queue_empty(fila)) {
            al_draw_bitmap_region(fundo, camera_x, 0, LARGURA, ALTURA, 0, 0, 0);

            if (!(jogador.intangivel_timer > 0 && (frame_counter / 6) % 2)) {
                 al_draw_filled_rectangle(jogador.x - camera_x, jogador.y,
                                          jogador.x - camera_x + TAM_JOGADOR, jogador.y + TAM_JOGADOR,
                                          al_map_rgb(255, 140, 0));
            }
            
            for (int i = 0; i < MAX_INIMIGOS; i++) {
                if (inimigos[i].ativo) {
                    ALLEGRO_COLOR cor;
                    if (inimigos[i].tipo == ZUMBI_ANDARILHO) {
                        cor = al_map_rgb(0, 0, 255); // Azul
                    } else {
                        cor = al_map_rgb(0, 255, 0); // Verde
                    }

                    float brilho = (float)inimigos[i].hp / inimigos[i].hp_max;
                    cor = al_map_rgb(0, 255 * brilho, 0); // Exemplo para o cuspidor

                    al_draw_filled_circle(inimigos[i].x - camera_x, inimigos[i].y, TAM_INIMIGO, cor);
                }
            }

            for (int i = 0; i < MAX_TIROS; i++) {
                if (tiros[i].ativo) {
                    al_draw_filled_rectangle(tiros[i].x - camera_x, tiros[i].y,
                                             tiros[i].x - camera_x + TAM_TIROS, tiros[i].y + 5,
                                             al_map_rgb(255, 255, 0));
                }
            }

            for (int i = 0; i < MAX_CUSPES; i++) {
                if (cuspes[i].ativo) {
                    al_draw_filled_circle(cuspes[i].x - camera_x, cuspes[i].y, 7, al_map_rgb(0, 255, 0));
                }
            }

            for (int i = 0; i < jogador.hp; i++) {
                al_draw_filled_circle(20 + (i * 15), 25, 5, al_map_rgb(255, 0, 0));
            }
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
    ALLEGRO_BITMAP* fundo_menu = al_load_bitmap("orig_big.png");
    ALLEGRO_BITMAP* fundo_jogo = al_load_bitmap("fundo.png");
    ALLEGRO_FONT* font = al_load_ttf_font("font.ttf", 40, 0);
    
    if (!disp || !fundo_menu || !font || !timer || !queue) {
        if (!disp) fprintf(stderr, "Falha ao criar a janela.\n");
        if (!fundo_menu) fprintf(stderr, "Falha ao carregar a imagem de fundo_menu.\n");
        if (!font) fprintf(stderr, "Falha ao carregar a fonte.\n");
        if (!timer) fprintf(stderr, "Falha ao criar o timer.\n");
        if (!queue) fprintf(stderr, "Falha ao criar a fila de eventos.\n");
        fprintf(stderr, "Falha ao inicializar Allegro ou carregar recursos.\n");
        return -1;
    }

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
                    inicia_jogo(disp, font, fundo_jogo);
                } else if (opcao_selecionada == 1) {
                    inicia_configuracoes(disp);
                } else if (opcao_selecionada == 2) {
                    inicia_instrucoes(disp);
                }
            }
        }

        else if (event.type == ALLEGRO_EVENT_TIMER) {
            al_draw_bitmap(fundo_menu, 0, 0, 0);

            for (int i = 0; i < OPCOES; i++) {
                ALLEGRO_COLOR cor = (i == opcao_selecionada) ? al_map_rgb(255, 0, 0) : al_map_rgb(0, 0, 0);
                al_draw_text(font, cor, 400, 200 + i * 60, ALLEGRO_ALIGN_CENTER, opcoes[i]);
            }

            al_flip_display();
        }
    }

    al_destroy_font(font);
    al_destroy_bitmap(fundo_menu);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(disp);

    return 0;
}
