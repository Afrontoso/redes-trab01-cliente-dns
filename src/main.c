/*
 * Cliente DNS - Trabalho 01 de Fundamentos de Redes de Computadores
 *
 * Monta manualmente uma consulta DNS do tipo MX (RFC 1035), envia via UDP
 * para a porta 53 do servidor informado e imprime o servidor de e-mail
 * encontrado no formato "dominio <> servidor_email".
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#define PORTA_DNS       53
#define TAM_MAX_UDP     512   /* tamanho maximo de mensagem DNS via UDP (RFC 1035 2.3.4) */
#define TAM_CABECALHO   12
#define TAM_MAX_NOME    255   /* tamanho maximo de um nome no formato de rotulos */
#define TAM_MAX_ROTULO  63
#define TIMEOUT_SEG     2
#define MAX_TENTATIVAS  3
#define MAX_SALTOS      32    /* limite de ponteiros de compressao seguidos */

#define TIPO_MX         15
#define CLASSE_IN       1

#define RCODE_NXDOMAIN  3

/*
 * Converte "unb.br" para o formato de rotulos "\3unb\2br\0".
 * Retorna o numero de bytes escritos em buf ou -1 se o nome for invalido.
 * buf precisa ter pelo menos TAM_MAX_NOME bytes.
 */
int monta_qname(const char *nome, unsigned char *buf) {
    int pos = 0;
    int len = strlen(nome);

    /* aceita o ponto final opcional de um FQDN ("unb.br.") */
    if (len > 0 && nome[len - 1] == '.') {
        len--;
    }

    if (len == 0) {
        return -1;
    }

    int inicio = 0;
    while (inicio <= len) {
        int fim = inicio;
        while (fim < len && nome[fim] != '.') {
            fim++;
        }

        int contador = fim - inicio;
        if (contador == 0 || contador > TAM_MAX_ROTULO) {
            return -1;
        }

        /* rotulo + byte de tamanho + byte zero final nao podem passar de 255 */
        if (pos + 1 + contador + 1 > TAM_MAX_NOME) {
            return -1;
        }

        buf[pos] = contador;
        memcpy(buf + pos + 1, nome + inicio, contador);
        pos += contador + 1;

        inicio = fim + 1;
    }

    buf[pos] = 0;
    pos++;

    return pos;
}

void escreve16(unsigned char *buf, int pos, unsigned int valor) {
    buf[pos]     = (valor >> 8) & 0xFF;
    buf[pos + 1] = valor & 0xFF;
}

unsigned int le16(const unsigned char *buf, int pos) {
    return (buf[pos] << 8) | buf[pos + 1];
}

/*
 * Pula um nome (possivelmente comprimido) a partir de pos.
 * Retorna a posicao logo apos o nome ou -1 se a mensagem estiver malformada.
 */
int pula_nome(const unsigned char *buf, int tam, int pos) {
    while (pos < tam) {
        unsigned char rotulo = buf[pos];

        if (rotulo == 0) {
            return pos + 1;
        }

        if ((rotulo & 0xC0) == 0xC0) {
            return (pos + 2 <= tam) ? pos + 2 : -1;
        }

        if ((rotulo & 0xC0) != 0) {
            return -1;  /* tipos de rotulo reservados */
        }

        pos += rotulo + 1;
    }

    return -1;
}

/*
 * Le o nome em pos, seguindo ponteiros de compressao, e escreve em saida
 * no formato "a.b.c". Retorna 0 em sucesso ou -1 se a mensagem estiver
 * malformada. saida precisa ter pelo menos TAM_MAX_NOME + 1 bytes.
 */
int le_nome(const unsigned char *buf, int tam, int pos, char *saida) {
    int s = 0;
    int saltos = 0;

    while (1) {
        if (pos >= tam) {
            return -1;
        }

        unsigned char rotulo = buf[pos];

        if (rotulo == 0) {
            break;
        }

        if ((rotulo & 0xC0) == 0xC0) {
            if (pos + 1 >= tam || ++saltos > MAX_SALTOS) {
                return -1;
            }
            pos = ((rotulo & 0x3F) << 8) | buf[pos + 1];
            continue;
        }

        if ((rotulo & 0xC0) != 0 || pos + 1 + rotulo > tam) {
            return -1;
        }

        /* ponto separador + rotulo + '\0' precisam caber na saida */
        if (s + (s > 0) + rotulo > TAM_MAX_NOME) {
            return -1;
        }

        if (s > 0) {
            saida[s] = '.';
            s++;
        }

        memcpy(saida + s, buf + pos + 1, rotulo);
        s += rotulo;

        pos += rotulo + 1;
    }

    saida[s] = '\0';
    return 0;
}

/* Gera o Transaction ID. Usa /dev/urandom e cai para rand() se falhar. */
unsigned int gera_id(void) {
    unsigned char bytes[2];
    int fd = open("/dev/urandom", O_RDONLY);

    if (fd >= 0) {
        ssize_t lidos = read(fd, bytes, sizeof(bytes));
        close(fd);
        if (lidos == (ssize_t) sizeof(bytes)) {
            return (bytes[0] << 8) | bytes[1];
        }
    }

    srand(time(NULL) ^ getpid());
    return rand() & 0xFFFF;
}

/*
 * Aguarda por ate TIMEOUT_SEG segundos uma resposta valida para a consulta:
 * vinda do servidor consultado, com o mesmo ID e com o bit QR ligado.
 * Pacotes que nao batem sao descartados sem reiniciar o prazo.
 * Retorna o numero de bytes recebidos ou -1 se o prazo acabar.
 */
int espera_resposta(int sock, const struct sockaddr_in *servidor, unsigned int id,
                    unsigned char *resposta, int tam_resposta) {
    struct timeval limite, agora, restante;

    gettimeofday(&limite, NULL);
    limite.tv_sec += TIMEOUT_SEG;

    while (1) {
        gettimeofday(&agora, NULL);
        timersub(&limite, &agora, &restante);
        if (restante.tv_sec < 0) {
            return -1;
        }

        fd_set leitura;
        FD_ZERO(&leitura);
        FD_SET(sock, &leitura);

        int pronto = select(sock + 1, &leitura, NULL, NULL, &restante);
        if (pronto < 0 && errno == EINTR) {
            continue;
        }
        if (pronto <= 0) {
            return -1;
        }

        struct sockaddr_in origem;
        socklen_t tam_origem = sizeof(origem);
        int recebidos = recvfrom(sock, resposta, tam_resposta, 0,
                                 (struct sockaddr *) &origem, &tam_origem);
        if (recebidos < 0) {
            /* ex.: ICMP port unreachable (ECONNREFUSED); aguarda ate o prazo */
            continue;
        }

        if (origem.sin_addr.s_addr != servidor->sin_addr.s_addr ||
            origem.sin_port != servidor->sin_port) {
            continue;
        }

        if (recebidos < TAM_CABECALHO || le16(resposta, 0) != id ||
            (resposta[2] & 0x80) == 0) {
            continue;
        }

        return recebidos;
    }
}

/*
 * Procura, na secao de respostas, o registro MX de menor preferencia.
 * Retorna 1 se encontrou (nome em nome_mx), 0 se nao ha MX e -1 se a
 * mensagem estiver malformada.
 */
int extrai_mx(const unsigned char *resposta, int tam, char *nome_mx) {
    unsigned int qdcount = le16(resposta, 4);
    unsigned int ancount = le16(resposta, 6);
    int p = TAM_CABECALHO;
    int achou = 0;
    unsigned int melhor_pref = 0;

    /* pula a secao de perguntas (QNAME + QTYPE + QCLASS) */
    for (unsigned int i = 0; i < qdcount; i++) {
        p = pula_nome(resposta, tam, p);
        if (p < 0 || p + 4 > tam) {
            return -1;
        }
        p += 4;
    }

    for (unsigned int i = 0; i < ancount; i++) {
        p = pula_nome(resposta, tam, p);
        /* TYPE(2) + CLASS(2) + TTL(4) + RDLENGTH(2) */
        if (p < 0 || p + 10 > tam) {
            return -1;
        }

        unsigned int tipo = le16(resposta, p);
        unsigned int classe = le16(resposta, p + 2);
        unsigned int rdlength = le16(resposta, p + 8);
        p += 10;

        if (p + (int) rdlength > tam) {
            return -1;
        }

        /* RDATA do MX: PREFERENCE(2) + EXCHANGE(nome) */
        if (tipo == TIPO_MX && classe == CLASSE_IN && rdlength >= 3) {
            unsigned int pref = le16(resposta, p);
            char candidato[TAM_MAX_NOME + 1];

            if (le_nome(resposta, tam, p + 2, candidato) < 0) {
                return -1;
            }

            if (!achou || pref < melhor_pref) {
                achou = 1;
                melhor_pref = pref;
                strcpy(nome_mx, candidato);
            }
        }

        p += rdlength;
    }

    return achou;
}

int main(int argc, char *argv[]) {
    unsigned char buf[TAM_MAX_UDP];
    unsigned char resposta[TAM_MAX_UDP];

    if (argc != 3) {
        fprintf(stderr, "uso: %s <dominio> <ip_servidor>\n", argv[0]);
        return 1;
    }

    const char *dominio = argv[1];

    struct sockaddr_in servidor;
    memset(&servidor, 0, sizeof(servidor));
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PORTA_DNS);

    if (inet_pton(AF_INET, argv[2], &servidor.sin_addr) != 1) {
        fprintf(stderr, "ip invalido: %s\n", argv[2]);
        return 1;
    }

    /* Cabecalho: ID aleatorio, consulta recursiva, 1 pergunta, demais zerados */
    unsigned int id = gera_id();

    escreve16(buf, 0,  id);
    escreve16(buf, 2,  0x0100);
    escreve16(buf, 4,  0x0001);
    escreve16(buf, 6,  0x0000);
    escreve16(buf, 8,  0x0000);
    escreve16(buf, 10, 0x0000);

    int tam_qname = monta_qname(dominio, buf + TAM_CABECALHO);
    if (tam_qname < 0) {
        fprintf(stderr, "nome de dominio invalido: %s\n", dominio);
        return 1;
    }

    int pos = TAM_CABECALHO + tam_qname;

    escreve16(buf, pos, TIPO_MX);
    pos += 2;
    escreve16(buf, pos, CLASSE_IN);
    pos += 2;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("erro ao criar socket");
        return 1;
    }

    int recebidos = -1;

    for (int tentativa = 0; tentativa < MAX_TENTATIVAS && recebidos < 0; tentativa++) {
        if (sendto(sock, buf, pos, 0, (struct sockaddr *) &servidor, sizeof(servidor)) < 0) {
            /* ex.: rede inalcancavel; conta como tentativa perdida */
            sleep(TIMEOUT_SEG);
            continue;
        }

        recebidos = espera_resposta(sock, &servidor, id, resposta, sizeof(resposta));
    }

    close(sock);

    if (recebidos < 0) {
        printf("Nao foi possivel coletar entrada MX para %s\n", dominio);
        return 1;
    }

    unsigned int rcode = resposta[3] & 0x0F;

    if (rcode == RCODE_NXDOMAIN) {
        printf("Dominio %s nao encontrado\n", dominio);
        return 1;
    }

    /* SERVFAIL, REFUSED etc.: o servidor nao soube responder */
    if (rcode != 0) {
        printf("Nao foi possivel coletar entrada MX para %s\n", dominio);
        return 1;
    }

    char nome_mx[TAM_MAX_NOME + 1];
    int resultado = extrai_mx(resposta, recebidos, nome_mx);

    if (resultado < 0) {
        printf("Nao foi possivel coletar entrada MX para %s\n", dominio);
        return 1;
    }

    if (resultado == 0) {
        printf("Dominio %s nao possui entrada MX\n", dominio);
        return 1;
    }

    printf("%s <> %s\n", dominio, nome_mx);
    return 0;
}
