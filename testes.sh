#!/bin/bash

echo "== 1. MX encontrado =="
./meu_cliente unb.br 8.8.8.8

echo "== 2. dominio inexistente =="
./meu_cliente imagdaskdasdasj.br 1.1.1.1

echo "== 3. dominio sem MX (CNAME) =="
./meu_cliente fga.unb.br 8.8.8.8

echo "== 4. servidor nao responde (demora ~6s) =="
./meu_cliente unb.br 127.0.0.1

echo "== 5. varios MX, mostra so o primeiro =="
./meu_cliente gmail.com 8.8.8.8

echo "== 6. IP invalido =="
./meu_cliente unb.br abc

echo "== 7. argumentos errados =="
./meu_cliente unb.br