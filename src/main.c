#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("uso: %s <dominio> <ip_servidor_dns>\n", argv[0]);
        return 1;
    }
    printf("dominio: %s, servidor: %s\n", argv[1], argv[2]);
    return 0;
}