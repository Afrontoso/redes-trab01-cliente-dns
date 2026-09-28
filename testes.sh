#!/bin/bash
# Executa os cenarios de teste do cliente DNS. Rode "make" antes.

echo "== 1. MX encontrado =="
./meu_cliente unb.br 8.8.8.8

echo "== 2. dominio inexistente =="
./meu_cliente imagdaskdasdasj.br 1.1.1.1

echo "== 3. dominio sem MX (so possui CNAME) =="
./meu_cliente fga.unb.br 8.8.8.8

echo "== 4. servidor nao responde (demora ~6s: 3 tentativas de 2s) =="
./meu_cliente unb.br 1.2.3.4

echo "== 5. varios MX, mostra o de menor preferencia =="
./meu_cliente gmail.com 8.8.8.8

echo "== 6. CNAME seguido de MX na resposta =="
./meu_cliente www.github.com 8.8.8.8

echo "== 7. nome com ponto final (FQDN) =="
./meu_cliente unb.br. 1.1.1.1

echo "== 8. nome de dominio invalido (rotulo vazio) =="
./meu_cliente unb..br 8.8.8.8

echo "== 9. IP invalido =="
./meu_cliente unb.br abc

echo "== 10. argumentos errados =="
./meu_cliente unb.br
