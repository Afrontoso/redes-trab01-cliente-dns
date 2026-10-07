# Trabalho de Redes 01 - Cliente DNS

Cliente DNS em C que monta manualmente o pacote de consulta (RFC 1034 / RFC 1035)
e o envia via socket UDP para a porta 53 do servidor informado. Realiza apenas
consultas do tipo **MX** (mail exchanger), classe **IN**.

Disciplina: Fundamentos de Redes de Computadores - UnB/FCTE
Professor: Tiago Alves

## Integrantes

| Nome | Matrícula |
|------|-----------|
| Victor Leandro Rocha de Assis| 222021826 |
| Pedro Luiz | 231036980 |
| Gabriel Saraiva Canabrava | 202045769

## Sistema operacional utilizado

- Desenvolvimento: macOS (Apple Silicon), com clang/GCC.
- Testes: Ubuntu 24.04 LTS (WSL2, kernel Linux 6.x, x86_64), com GCC 13.

O código usa apenas a API POSIX de sockets (`sys/socket.h`, `arpa/inet.h`), então
compila em Linux e macOS. **Não compila nativamente no Windows** (MinGW/MSVC). No
Windows, use o WSL.

## Ambiente de desenvolvimento

- Linguagem: C (padrão GNU C17, o padrão do GCC)
- Compilador: GCC (`-Wall -Wextra -g`, compila sem avisos)
- Build: GNU Make
- Editor: Visual Studio Code
- Análise de pacotes: Wireshark (filtro `dns`)
- Conferência dos resultados: `dig` (pacote `dnsutils`)

## Como construir

Pré-requisitos no Ubuntu/Debian:

```bash
sudo apt install build-essential
```

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

- `<dominio>`: nome cujo registro MX se deseja resolver (ex.: `unb.br`).
- `<ip_do_servidor_dns>`: endereço IPv4 do servidor DNS a consultar (ex.: `8.8.8.8`).

Exemplo:

```bash
./meu_cliente unb.br 8.8.8.8
```

Para rodar todos os cenários de teste:

```bash
make && bash testes.sh
```

## Telas / instruções de uso

O programa não possui interface gráfica. Toda a interação acontece via linha de
comando. O resultado é escrito na saída padrão (stdout) e os erros de uso vão para
a saída de erro (stderr). O código de saída é `0` quando um MX é encontrado e `1`
em qualquer falha.

### Resolução bem-sucedida

```text
$ ./meu_cliente unb.br 8.8.8.8
unb.br <> unb-br.mail.protection.outlook.com
```

Quando o domínio possui vários MX, é exibido o de **menor preferência**, que é o
servidor prioritário:

```text
$ ./meu_cliente gmail.com 8.8.8.8
gmail.com <> gmail-smtp-in.l.google.com
```

Quando o nome é um alias (CNAME), o MX do nome canônico é exibido:

```text
$ ./meu_cliente www.github.com 8.8.8.8
www.github.com <> github-com.mail.protection.outlook.com
```

### Domínio inexistente (RCODE = 3, NXDOMAIN)

```text
$ ./meu_cliente imagdaskdasdasj.br 1.1.1.1
Dominio imagdaskdasdasj.br nao encontrado
```

### Domínio sem entrada MX

```text
$ ./meu_cliente fga.unb.br 8.8.8.8
Dominio fga.unb.br nao possui entrada MX
```

### Servidor inacessível ou sem resposta após 3 tentativas

O cliente espera 2 segundos por tentativa, até 3 tentativas (cerca de 6 s no total).
A mesma mensagem é usada quando o servidor responde com erro (SERVFAIL, REFUSED)
ou envia uma resposta malformada.

```text
$ ./meu_cliente unb.br 1.2.3.4
Nao foi possivel coletar entrada MX para unb.br
```

### Erros de uso

```text
$ ./meu_cliente unb.br
uso: ./meu_cliente <dominio> <ip_servidor>

$ ./meu_cliente unb.br abc
ip invalido: abc

$ ./meu_cliente unb..br 8.8.8.8
nome de dominio invalido: unb..br
```

## Funcionamento

1. **Montagem da consulta** (feita byte a byte, sem bibliotecas de resolução):
   - Transaction ID: 16 bits aleatórios lidos de `/dev/urandom` (com `rand()` como alternativa).
   - Flags: `0x0100` (consulta padrão com *Recursion Desired*).
   - QDCOUNT `0x0001`; ANCOUNT, NSCOUNT e ARCOUNT `0x0000`.
   - QNAME no formato de rótulos (`unb.br` → `\x03unb\x02br\x00`), QTYPE = 15 (MX), QCLASS = 1 (IN).
2. **Envio via UDP** para `<ip>:53` com `sendto`.
3. **Espera pela resposta** com `select`, com prazo de 2 s. Só é aceito um pacote
   que venha do IP e da porta consultados, tenha o mesmo Transaction ID e esteja
   com o bit QR = 1. Pacotes que não batem são descartados sem reiniciar o prazo.
   Sem resposta válida, a consulta é reenviada, até 3 vezes.
4. **Interpretação da resposta**:
   - RCODE 3 → domínio não encontrado; outro RCODE ≠ 0 → falha na coleta.
   - Pula a seção de perguntas e percorre **todos** os registros da seção de
     respostas, ignorando os que não são MX (ex.: CNAME) e guardando o MX de menor
     preferência.
   - O nome do servidor de e-mail é lido com suporte a **compressão de nomes**
     (ponteiros `0xC0`, RFC 1035 §4.1.4).
   - Todas as leituras verificam os limites do pacote recebido, o limite de
     255 bytes por nome e o número máximo de saltos de ponteiro, então respostas
     malformadas não causam estouro de buffer nem laço infinito.

## Limitações conhecidas

- Apenas **IPv4**: o servidor DNS precisa ser informado por um endereço IPv4
  (não aceita IPv6 nem nome de host).
- Apenas consultas **MX / IN**, e só um MX é exibido (o de menor preferência).
  Em caso de empate de preferência, fica o primeiro que aparece na resposta.
- **Sem fallback para TCP**: se a resposta vier truncada (bit TC), o cliente usa
  apenas os registros presentes no pacote UDP de até 512 bytes, e não suporta EDNS0.
- Nomes internacionalizados (IDN, com acentos) não são convertidos para punycode.
  Devem ser informados já no formato ASCII (`xn--...`).
- O cliente **não segue a cadeia CNAME** fazendo novas consultas. Ele depende de o
  servidor recursivo já incluir o MX do nome canônico na resposta. Se o destino do
  CNAME não tiver MX (caso de `fga.unb.br`), informa que não há entrada MX.
- Firewalls ou redes que bloqueiam UDP/53 para fora da rede local fazem todas as
  consultas terminarem em "Nao foi possivel coletar entrada MX".
- Depende da API de sockets POSIX: não compila nativamente no Windows.
- Os caracteres acentuados foram omitidos das mensagens de saída de propósito,
  seguindo os exemplos do enunciado.

## Estrutura do repositório

```text
.
├── Makefile       # build (make / make clean)
├── README.md      # esta documentação
├── src/main.c     # código-fonte do cliente
└── testes.sh      # cenários de teste manuais
```

