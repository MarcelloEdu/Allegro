//Compilação: gcc AggresiveSquares.c -o AS $(pkg-config allegro-5 allegro_main-5 allegro_font-5 --libs --cflags)

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>

int main(){
    al_init();
    al_install_keyboard();
    al_init_image_addon();
    al_init_font_addon();
	// al_init_ttf_addon();

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 30.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_FONT* font = al_create_builtin_font();  // Fonte padrão (bitmap)
    ALLEGRO_DISPLAY* disp = al_create_display(800, 600);  // Tela mais leve para testes
    ALLEGRO_BITMAP* fundo = al_load_bitmap("orig_big.png");


    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    ALLEGRO_EVENT event;
    al_start_timer(timer);

    while (1) {
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_TIMER) {
            // Desenha o fundo
            al_draw_bitmap(fundo, 0, 0, 0);

            // Desenha o menu (centralizado)
            al_draw_text(font, al_map_rgb(0, 0, 0), 400, 200, ALLEGRO_ALIGN_CENTER, "JOGAR");
            al_draw_text(font, al_map_rgb(0, 0, 0), 400, 250, ALLEGRO_ALIGN_CENTER, "CONFIGURAÇÕES");
            al_draw_text(font, al_map_rgb(0, 0, 0), 400, 300, ALLEGRO_ALIGN_CENTER, "INSTRUÇÕES");

            al_flip_display();
        } else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }
    }

    al_destroy_font(font);
    al_destroy_bitmap(fundo);
    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}
