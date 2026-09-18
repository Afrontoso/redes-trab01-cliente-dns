CC = gcc
CFLAGS = -Wall -Wextra -g
meu_cliente: src/main.c
	$(CC) $(CFLAGS) -o meu_cliente src/main.c

clean:
	rm -f meu_cliente
