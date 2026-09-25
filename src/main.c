#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

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

unsigned int le16(unsigned char *buf, int pos) {
    return (buf[pos] << 8) | buf[pos + 1];
}

int pula_nome(unsigned char *buf, int pos) {
    while (1) {
        unsigned char tam = buf[pos];

        if (tam == 0) {
            return pos + 1;
        }

        if ((tam & 0xC0) == 0xC0) {
            return pos + 2;
        }

        pos += tam + 1;
    }
}

void le_nome(unsigned char *buf, int pos, char *saida) {
    int s = 0;

    while (1) {
        unsigned char tam = buf[pos];

        if (tam == 0) {
            break;
        }

        if ((tam & 0xC0) == 0xC0) {
            pos = ((tam & 0x3F) << 8) | buf[pos + 1];
            continue;
        }

        if (s > 0) {
            saida[s] = '.';
            s++;
        }

        for (int i = 0; i < tam; i++) {
            saida[s] = buf[pos + 1 + i];
            s++;
        }

        pos += tam + 1;
    }

    saida[s] = '\0';
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
    
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int recebidos = -1;

    for (int tentativa = 0; tentativa < 3; tentativa++) {
        sendto(sock, buf, pos, 0, (struct sockaddr *)&servidor, sizeof(servidor));

        recebidos = recvfrom(sock, resposta, sizeof(resposta), 0, NULL, NULL);

        if (recebidos >= 12 && le16(resposta, 0) == id) {
            break;
        }

        recebidos = -1;
    }

    if (recebidos <= 0) {
        printf("Nao foi possivel coletar entrada MX para %s\n", argv[1]);
        close(sock);
        return 0;
    }
    
    unsigned int rcode = resposta[3] & 0x0F;
    unsigned int ancount = le16(resposta, 6);
    
    if (rcode == 3) {
        printf("Dominio %s nao encontrado\n", argv[1]);
        close(sock);
        return 0;
    }

    if (rcode != 0 || ancount == 0) {
        printf("Dominio %s nao possui entrada MX\n", argv[1]);
        close(sock);
        return 0;
    }


    int p = 12;
    p = pula_nome(resposta, p);
    p += 4;

    p = pula_nome(resposta, p);

    unsigned int tipo = le16(resposta, p);

    if (tipo != 15) {
        printf("Dominio %s nao possui entrada MX\n", argv[1]);
        close(sock);
        return 0;
    }

    p += 12;

    char nome_mx[256];
    le_nome(resposta, p, nome_mx);
    printf("%s <> %s\n", argv[1], nome_mx);

    close(sock);
    return 0;
}
