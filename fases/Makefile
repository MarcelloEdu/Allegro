# Nome do executável
TARGET = AS

# Fonte principal
SRC = AggresiveSquares.c

# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -I/usr/include/x86_64-linux-gnu
LFLAGS = -lallegro_main -lallegro_ttf -lallegro_font -lallegro_image -lallegro_primitives -lallegro -lm

# Regra principal
all:
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LFLAGS)

# Limpa o executável
clean:
	rm -f $(TARGET)
