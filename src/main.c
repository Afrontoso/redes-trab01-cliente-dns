#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    unsigned char resposta[512];


    if (argc != 3) {
        printf("uso: %s <dominio> <ip_servidor>\n", argv[0]);
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

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("erro ao criar socket\n");
        return 1;
    }

    struct sockaddr_in servidor;
    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(53);

    if (inet_pton(AF_INET, argv[2], &servidor.sin_addr) != 1) {
        printf("ip invalido: %s\n", argv[2]);
        close(sock);
        return 1;
    }

    if (sendto(sock, buf, pos, 0, (struct sockaddr *)&servidor, sizeof(servidor)) < 0) {
        printf("erro ao enviar\n");
        close(sock);
        return 1;
    }

    int recebidos = recvfrom(sock, resposta, sizeof(resposta), 0, NULL, NULL);
    if (recebidos < 0) {
        printf("erro ao receber\n");
        close(sock);
        return 1;
    }

    printf("resposta com %d bytes:\n", recebidos);
    for (int i = 0; i < recebidos; i++) {
        printf("%02X ", resposta[i]);
        if ((i + 1) % 12 == 0) printf("\n");
    }
    printf("\n");

    close(sock);
    return 0;
}
