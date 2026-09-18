#include <stdio.h>
#include <string.h>

int monta_qname(const char *nome, unsigned char *buf) {
    int pos = 0;
    int pos_tamanho = 0;
    int contador = 0;

    pos = 1;

    for (int i = 0; nome[i] != '\0'; i++) {
        if (nome[i] == '.') {
            buf[pos_tamanho] = contador;
            pos_tamanho = pos;
            pos++;
            contador = 0;
        } else {
            buf[pos] = nome[i];
            pos++;
            contador++;
        }
    }

    buf[pos_tamanho] = contador;
    buf[pos] = 0;
    pos++;

    return pos;
}

int main(int argc, char *argv[]) {
    unsigned char buf[256];

    if (argc != 2) {
        printf("uso: %s <dominio>\n", argv[0]);
        return 1;
    }

    int tamanho = monta_qname(argv[1], buf);

    printf("qname tem %d bytes:\n", tamanho);
    for (int i = 0; i < tamanho; i++) {
        if (buf[i] >= 32 && buf[i] <= 126) {
            printf("[%d '%c'] ", buf[i], buf[i]);
        } else {
            printf("[%d] ", buf[i]);
        }
    }
    printf("\n");

    return 0;
}