#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

void escreve16(unsigned char *buf, int pos, unsigned int valor) {
    buf[pos]     = (valor >> 8) & 0xFF;
    buf[pos + 1] = valor & 0xFF;
}


int main(int argc, char *argv[]) {
    unsigned char buf[512];

    if (argc != 2) {
        printf("uso: %s <dominio>\n", argv[0]);
        return 1;
    }

    srand(time(NULL));
    unsigned int id = rand() & 0xFFFF;

    escreve16(buf, 0,  id);
    escreve16(buf, 2,  0x0100);
    escreve16(buf, 4,  0x0001);
    escreve16(buf, 6,  0x0000);
    escreve16(buf, 8,  0x0000);
    escreve16(buf, 10, 0x0000);

    int tam_qname = monta_qname(argv[1], buf + 12);
    int pos = 12 + tam_qname;

    escreve16(buf, pos, 15);
    pos += 2;
    escreve16(buf, pos, 1);
    pos += 2;

    printf("pacote com %d bytes:\n", pos);
    for (int i = 0; i < pos; i++) {
        printf("%02X ", buf[i]);
        if ((i + 1) % 12 == 0) printf("\n");
    }
    printf("\n");

    return 0;
}