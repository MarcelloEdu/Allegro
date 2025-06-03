#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <stdio.h>

#define OPCOES 3

int inicia_jogo(ALLEGRO_DISPLAY* disp) {
    al_clear_to_color(al_map_rgb(0, 0, 0));
    al_flip_display();
    al_rest(2.0);
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
    al_init_ttf_addon();  // Importante para TTF

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

        // if (event.type == ALLEGRO_EVENT_TIMER) {
        //     al_draw_bitmap(fundo, 0, 0, 0);

        //     al_draw_text(font, al_map_rgb(0, 0, 0), 400, 200, ALLEGRO_ALIGN_CENTER, "JOGAR");
        //     al_draw_text(font, al_map_rgb(0, 0, 0), 400, 260, ALLEGRO_ALIGN_CENTER, "CONFIGURAÇÕES");
        //     al_draw_text(font, al_map_rgb(0, 0, 0), 400, 320, ALLEGRO_ALIGN_CENTER, "INSTRUÇÕES");

        //     al_flip_display();
        // }
        
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
