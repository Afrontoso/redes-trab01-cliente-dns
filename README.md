# Trabalho de Redes 01 - Cliente DNS

Cliente DNS em C que monta o pacote de consulta manualmente e envia via socket UDP.
Realiza apenas consultas do tipo MX (mail exchanger).

Disciplina: Fundamentos de Redes de Computadores - UnB/FCTE
Professor: Tiago Alves

## Integrantes

| Nome | Matrícula |
|------|-----------|
| Victor | 222021826 |
| (preencher) | |
| (preencher) | |

## Sistema operacional utilizado

TODO: preencher com a distribuição e versão usada no desenvolvimento.

## Ambiente de desenvolvimento

- Compilador: GCC
- Build: GNU Make
- Editor: TODO
- Análise de pacotes: Wireshark

## Como construir

Na raiz do projeto:

```bash
make
```

Isso gera o executável `meu_cliente`. Para limpar os artefatos de compilação:

```bash
make clean
```

## Como executar

```bash
./meu_cliente <dominio> <ip_do_servidor_dns>
```

Exemplo:

```bash
./meu_cliente unb.br 8.8.8.8
```

## Telas / instruções de uso

O programa não possui interface gráfica. Toda a interação acontece via linha de
comando, e o resultado é escrito na saída padrão.

## Resolução bem sucedida: 

```bash
 ./meu_cliente unb.br 8.8.8.8
unb.br <> unb-br.mail.protection.outlook.com
```

## Domínio inexistente:

```bash
 ./meu_cliente imagdaskdasdasj.br 1.1.1.1
Dominio imagdaskdasdasj.br nao encontrado
```

## Domínio sem entrada MX:

```bash
 ./meu_cliente fga.unb.br 8.8.8.8
Dominio fga.unb.br nao possui entrada MX
```

## Servidor inacessível ou sem resposta após 3 tentativas:

```bash
 ./meu_cliente unb.br 1.2.3.4
Nao foi possivel coletar entrada MX para unb.br
```

## Limitações conhecidas

TODO: preencher ao final do desenvolvimento.

## Observações

O payload da requisição DNS é montado byte a byte, sem uso de bibliotecas de
resolução de nomes. Apenas a API de sockets UDP do sistema é utilizada, conforme
exigido no enunciado.