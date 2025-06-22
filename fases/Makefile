# Nome do executável
TARGET = AS

# Fonte principal
SRC = AggresiveSquares.c

# Compilador
CC = gcc

# Flags de compilação (para includes e avisos)
CFLAGS = -g -Wall -I/usr/include/x86_64-linux-gnu

# Flags de linkagem (as bibliotecas)
# ADICIONADO '-lm' no final para incluir a biblioteca de matemática
LFLAGS = -lallegro_main -lallegro_ttf -lallegro_font -lallegro_image -lallegro_primitives -lallegro_acodec -lallegro_audio -lallegro -lm

# Regra principal para construir o programa
all:
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LFLAGS)

# Limpa o executável
clean:
	rm -f $(TARGET)

# Atalho para compilar e rodar
run: all
	./$(TARGET)