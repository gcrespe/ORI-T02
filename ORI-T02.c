/* ==========================================================================
 * Universidade Federal de São Carlos - Campus Sorocaba
 * Disciplina: Organização de Recuperação da Informação
 * Prof. Tiago A. Almeida
 *
 * Trabalho 02 - Árvores-B
 *
 * RA:
 * Aluno:
 * ========================================================================== */

/* Bibliotecas */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

//  ;)
typedef enum {false, true} bool;

/* Tamanho dos campos dos registros */
/* Campos de tamanho fixo */
#define TAM_CPF 12
#define TAM_NASCIMENTO 11
#define TAM_CELULAR 12
#define TAM_SALDO 14
#define TAM_TIMESTAMP 15
#define QTD_MAX_CHAVES_PIX 4

/* Campos de tamanho variável (tamanho máximo) */
// Restaram 204 bytes (48B nome, 47B email, 11B chave CPF, 11B chave número de celular,
// 47B chave email, 37B chave aleatória, 3B demilitadores)
#define TAM_MAX_NOME 48
#define TAM_MAX_EMAIL 47
#define TAM_MAX_CHAVE_PIX 48

#define MAX_REGISTROS 1000
#define TAM_REGISTRO_CLIENTE 256
#define TAM_REGISTRO_TRANSACAO 49
#define TAM_ARQUIVO_CLIENTE (TAM_REGISTRO_CLIENTE * MAX_REGISTROS + 1)
#define TAM_ARQUIVO_TRANSACAO (TAM_REGISTRO_TRANSACAO * MAX_REGISTROS + 1)

#define TAM_RRN_REGISTRO 4
#define TAM_CHAVE_CLIENTES_IDX (TAM_CPF + TAM_RRN_REGISTRO - 1)
#define TAM_CHAVE_TRANSACOES_IDX (TAM_CPF + TAM_TIMESTAMP + TAM_RRN_REGISTRO - 2)
#define TAM_CHAVE_CHAVES_PIX_IDX (TAM_MAX_CHAVE_PIX + TAM_CPF - 2)
#define TAM_CHAVE_TIMESTAMP_CPF_ORIGEM_IDX (TAM_TIMESTAMP + TAM_CPF - 2)

#define TAM_ARQUIVO_CLIENTES_IDX (1000 * MAX_REGISTROS + 1)
#define TAM_ARQUIVO_TRANSACOES_IDX (1000 * MAX_REGISTROS + 1)
#define TAM_ARQUIVO_CHAVES_PIX_IDX (1000 * MAX_REGISTROS + 1)
#define TAM_ARQUIVO_TIMESTAMP_CPF_ORIGEM_IDX (1000 * MAX_REGISTROS + 1)

/* Mensagens padrões */
#define SUCESSO                          "OK\n"
#define RRN_NOS                          "Nos percorridos:"
#define AVISO_NENHUM_REGISTRO_ENCONTRADO "AVISO: Nenhum registro encontrado\n"
#define ERRO_OPCAO_INVALIDA              "ERRO: Opcao invalida\n"
#define ERRO_MEMORIA_INSUFICIENTE        "ERRO: Memoria insuficiente\n"
#define ERRO_PK_REPETIDA                 "ERRO: Ja existe um registro com a chave primaria %s\n"
#define ERRO_REGISTRO_NAO_ENCONTRADO     "ERRO: Registro nao encontrado\n"
#define ERRO_SALDO_NAO_SUFICIENTE        "ERRO: Saldo insuficiente\n"
#define ERRO_VALOR_INVALIDO              "ERRO: Valor invalido\n"
#define ERRO_CHAVE_PIX_REPETIDA          "ERRO: Ja existe uma chave PIX %s\n"
#define ERRO_CHAVE_PIX_DUPLICADA         "ERRO: Chave PIX tipo %c ja cadastrada para este usuario\n"
#define ERRO_CHAVE_PIX_INVALIDA          "ERRO: Chave PIX invalida\n"
#define ERRO_TIPO_PIX_INVALIDO           "ERRO: Tipo %c invalido para chaves PIX\n"
#define ERRO_ARQUIVO_VAZIO               "ERRO: Arquivo vazio\n"
#define ERRO_NAO_IMPLEMENTADO            "ERRO: Funcao %s nao implementada\n"

/* Registro de Cliente */
typedef struct {
    char cpf[TAM_CPF];
    char nome[TAM_MAX_NOME];
    char nascimento[TAM_NASCIMENTO];
    char email[TAM_MAX_EMAIL];
    char celular[TAM_CELULAR];
    double saldo;
    char chaves_pix[QTD_MAX_CHAVES_PIX][TAM_MAX_CHAVE_PIX];
} Cliente;

/* Registro de transação */
typedef struct {
    char cpf_origem[TAM_CPF];
    char cpf_destino[TAM_CPF];
    double valor;
    char timestamp[TAM_TIMESTAMP];
} Transacao;

/*----- Registros dos índices -----*/
/* Struct para um nó de Árvore-B */
typedef struct {
    int this_rrn;
    int qtd_chaves;
    char **chaves; // ponteiro para o começo do campo de chaves no arquivo de índice respectivo
    bool folha;
    int *filhos; // vetor de int para o RRN dos nós filhos (DEVE SER DESALOCADO APÓS O USO!!!)
} btree_node;

/* Variáveis globais */
/* Arquivos de dados */
char ARQUIVO_CLIENTES[TAM_ARQUIVO_CLIENTE];
char ARQUIVO_TRANSACOES[TAM_ARQUIVO_TRANSACAO];

/* Ordem das Árvores-B */
int btree_order = 3; // valor padrão

/* Índices */
/* Struct para os parâmetros de uma Árvore-B */
typedef struct {
    // RRN da raíz
    int rrn_raiz;

    // Ponteiro para o arquivo de índice
    char *arquivo;

    // Quantidade de nós no arquivo de índice
    unsigned qtd_nos;

    // Tamanho de uma chave nesse índice
    unsigned tam_chave;

    // Função utilizada para comparar as chaves do índice.
    // Igual às funções de comparação do bsearch e qsort.
    int (*compar)(const void *key, const void *elem);
} btree;

typedef struct {
    char chave_promovida[TAM_CHAVE_CHAVES_PIX_IDX + 1]; // TAM_CHAVE_CHAVES_PIX_IDX é a maior chave possível
    int filho_direito;
} promovido_aux;

/* Arquivos de índices */
char ARQUIVO_CLIENTES_IDX[TAM_ARQUIVO_CLIENTES_IDX];
char ARQUIVO_TRANSACOES_IDX[TAM_ARQUIVO_TRANSACOES_IDX];
char ARQUIVO_CHAVES_PIX_IDX[TAM_ARQUIVO_CHAVES_PIX_IDX];
char ARQUIVO_TIMESTAMP_CPF_ORIGEM_IDX[TAM_ARQUIVO_TIMESTAMP_CPF_ORIGEM_IDX];

/* Comparam a chave (key) com cada elemento do índice (elem).
 * Funções auxiliares para o buscar e inserir chaves em Árvores-B.
 * Note que, desta vez, as funções comparam chaves no formato de strings, não structs.
 * key é a chave do tipo string que está sendo buscada ou inserida. elem é uma chave do tipo string da struct btree_node.
 *
 * Dica: busque sobre as funções strncmp e strnlen, muito provavelmente vai querer utilizá-las no código.
 * */
int order_clientes_idx(const void *key, const void *elem);
int order_transacoes_idx(const void *key, const void *elem);
int order_chaves_pix_idx(const void *key, const void *elem);
int order_timestamp_cpf_origem_idx(const void *key, const void *elem);

btree clientes_idx = {
    .rrn_raiz = -1,
    .arquivo = ARQUIVO_CLIENTES_IDX,
    .qtd_nos = 0,
    .tam_chave = TAM_CHAVE_CLIENTES_IDX,
    .compar = order_clientes_idx,
};

btree transacoes_idx = {
    .rrn_raiz = -1,
    .arquivo = ARQUIVO_TRANSACOES_IDX,
    .qtd_nos = 0,
    .tam_chave = TAM_CHAVE_TRANSACOES_IDX,
    .compar = order_transacoes_idx,
};

btree chaves_pix_idx = {
    .rrn_raiz = -1,
    .arquivo = ARQUIVO_CHAVES_PIX_IDX,
    .qtd_nos = 0,
    .tam_chave = TAM_CHAVE_CHAVES_PIX_IDX,
    .compar = order_chaves_pix_idx,
};

btree timestamp_cpf_origem_idx = {
    .rrn_raiz = -1,
    .arquivo = ARQUIVO_TIMESTAMP_CPF_ORIGEM_IDX,
    .qtd_nos = 0,
    .tam_chave = TAM_CHAVE_TIMESTAMP_CPF_ORIGEM_IDX,
    .compar = order_timestamp_cpf_origem_idx,
};

/* Contadores */
unsigned qtd_registros_clientes = 0;
unsigned qtd_registros_transacoes = 0;

/* Funções de geração determinística de números pseudo-aleatórios */
uint64_t prng_seed;

void prng_srand(uint64_t value) {
    prng_seed = value;
}

uint64_t prng_rand() {
    // https://en.wikipedia.org/wiki/Xorshift#xorshift*
    uint64_t x = prng_seed; // O estado deve ser iniciado com um valor diferente de 0
    x ^= x >> 12; // a
    x ^= x << 25; // b
    x ^= x >> 27; // c
    prng_seed = x;
    return x * UINT64_C(0x2545F4914F6CDD1D);
}

/**
 * Gera um <a href="https://en.wikipedia.org/wiki/Universally_unique_identifier">UUID Version-4 Variant-1</a>
 * (<i>string</i> aleatória) de 36 caracteres utilizando o gerador de números pseudo-aleatórios
 * <a href="https://en.wikipedia.org/wiki/Xorshift#xorshift*">xorshift*</a>. O UUID é
 * escrito na <i>string</i> fornecida como parâmetro.<br />
 * <br />
 * Exemplo de uso:<br />
 * <code>
 * char chave_aleatoria[37];<br />
 * new_uuid(chave_aleatoria);<br />
 * printf("chave aleatória: %s&#92;n", chave_aleatoria);<br />
 * </code>
 *
 * @param buffer String de tamanho 37 no qual será escrito
 * o UUID. É terminado pelo caractere <code>\0</code>.
 */
void new_uuid(char buffer[37]) {
    uint64_t r1 = prng_rand();
    uint64_t r2 = prng_rand();

    sprintf(buffer, "%08x-%04x-%04lx-%04lx-%012lx", (uint32_t)(r1 >> 32), (uint16_t)(r1 >> 16), 0x4000 | (r1 & 0x0fff), 0x8000 | (r2 & 0x3fff), r2 >> 16);
}

/* Funções de manipulação de data (timestamp) */
int64_t epoch;

void set_time(int64_t value) {
    epoch = value;
}

void tick_time() {
    epoch += prng_rand() % 864000; // 10 dias
}

struct tm gmtime_(const int64_t lcltime) {
    // based on https://sourceware.org/git/?p=newlib-cygwin.git;a=blob;f=newlib/libc/time/gmtime_r.c;
    struct tm res;
    long days = lcltime / 86400 + 719468;
    long rem = lcltime % 86400;
    if (rem < 0) {
        rem += 86400;
        --days;
    }

    res.tm_hour = (int) (rem / 3600);
    rem %= 3600;
    res.tm_min = (int) (rem / 60);
    res.tm_sec = (int) (rem % 60);

    int weekday = (3 + days) % 7;
    if (weekday < 0) weekday += 7;
    res.tm_wday = weekday;

    int era = (days >= 0 ? days : days - 146096) / 146097;
    unsigned long eraday = days - era * 146097;
    unsigned erayear = (eraday - eraday / 1460 + eraday / 36524 - eraday / 146096) / 365;
    unsigned yearday = eraday - (365 * erayear + erayear / 4 - erayear / 100);
    unsigned month = (5 * yearday + 2) / 153;
    unsigned day = yearday - (153 * month + 2) / 5 + 1;
    month += month < 10 ? 2 : -10;

    int isleap = ((erayear % 4) == 0 && (erayear % 100) != 0) || (erayear % 400) == 0;
    res.tm_yday = yearday >= 306 ? yearday - 306 : yearday + 59 + isleap;
    res.tm_year = (erayear + era * 400 + (month <= 1)) - 1900;
    res.tm_mon = month;
    res.tm_mday = day;
    res.tm_isdst = 0;

    return res;
}

/**
 * Escreve a <i>timestamp</i> atual no formato <code>AAAAMMDDHHmmSS</code> em uma <i>string</i>
 * fornecida como parâmetro.<br />
 * <br />
 * Exemplo de uso:<br />
 * <code>
 * char timestamp[TAM_TIMESTAMP];<br />
 * current_timestamp(timestamp);<br />
 * printf("timestamp atual: %s&#92;n", timestamp);<br />
 * </code>
 *
 * @param buffer String de tamanho <code>TAM_TIMESTAMP</code> no qual será escrita
 * a <i>timestamp</i>. É terminado pelo caractere <code>\0</code>.
 */
void current_timestamp(char buffer[TAM_TIMESTAMP]) {
    // http://www.cplusplus.com/reference/ctime/strftime/
    // http://www.cplusplus.com/reference/ctime/gmtime/
    // AAAA MM DD HH mm SS
    // %Y   %m %d %H %M %S
    struct tm tm_ = gmtime_(epoch);
    strftime(buffer, TAM_TIMESTAMP, "%Y%m%d%H%M%S", &tm_);
}

/* Remove comentários (--) e caracteres whitespace do começo e fim de uma string */
void clear_input(char *str) {
    char *ptr = str;
    int len = 0;

    for (; ptr[len]; ++len) {
        if (strncmp(&ptr[len], "--", 2) == 0) {
            ptr[len] = '\0';
            break;
        }
    }

    while(len-1 > 0 && isspace(ptr[len-1]))
        ptr[--len] = '\0';

    while(*ptr && isspace(*ptr))
        ++ptr, --len;

    memmove(str, ptr, len + 1);
}

/* ==========================================================================
 * ========================= PROTÓTIPOS DAS FUNÇÕES =========================
 * ========================================================================== */

/* Cria o índice respectivo */
void criar_clientes_idx();
void criar_transacoes_idx();
void criar_chaves_pix_idx();
void criar_timestamp_cpf_origem_idx();

/* Exibe um registro com base no RRN */
bool exibir_cliente(int rrn);
bool exibir_transacao(int rrn);

/* Exibe um registro com base na chave de um btree_node */
bool exibir_btree_cliente(char *chave);
bool exibir_btree_transacao(char *chave);
bool exibir_btree_timestamp_cpf_origem(char *chave);

/* Recupera do arquivo o registro com o RRN informado
 * e retorna os dados nas struct Cliente/Transacao */
Cliente recuperar_registro_cliente(int rrn);
Transacao recuperar_registro_transacao(int rrn);

/* Escreve no arquivo respectivo na posição informada (RRN)
 * os dados na struct Cliente/Transacao */
void escrever_registro_cliente(Cliente c, int rrn);
void escrever_registro_transacao(Transacao t, int rrn);

/* Funções principais */
void cadastrar_cliente_menu(char *cpf, char *nome, char *nascimento, char *email, char *celular);
void alterar_saldo_menu(char *cpf, double valor);
void cadastrar_chave_pix_menu(char *cpf, char tipo);
void transferir_menu(char *chave_pix_origem, char *chave_pix_destino, double valor);

/* Busca */
void buscar_cliente_cpf_menu(char *cpf);
void buscar_cliente_chave_pix_menu(char *chave_pix);
void buscar_transacao_cpf_origem_timestamp_menu(char *cpf, char *timestamp);

/* Listagem */
void listar_clientes_cpf_menu();
void listar_transacoes_periodo_menu(char *data_inicio, char *data_fim);
void listar_transacoes_cpf_origem_periodo_menu(char *cpf, char *data_inicio, char *data_fim);

/* Imprimir arquivos de dados */
void imprimir_arquivo_clientes_menu();
void imprimir_arquivo_transacoes_menu();

/* Imprimir arquivos de índice */
void imprimir_clientes_idx_menu();
void imprimir_transacoes_idx_menu();
void imprimir_chaves_pix_idx_menu();
void imprimir_timestamp_cpf_origem_idx_menu();

/* Liberar memória e encerrar programa */
void liberar_memoria_menu();

/* Funções de manipulação de Árvores-B */

/**
 * Responsável por inserir uma chave (k) em uma Árvore-B (T). Atualiza os parâmetros da Árvore-B conforme necessário.<br />
 * A chave a ser inserida deve estar no formato correto e com tamanho t->tam_chave.<br />
 * O funcionamento deve ser genérico para qualquer Árvore-B, considerando que os únicos parâmetros que se alteram entre
 * as árvores é o t->tam_chave.<br />
 * Comportamento de acordo com as especificações do PDF sobre Árvores-B e suas operações.<br />
 * <br />
 * Exemplo de uso:<br />
 * <code>
 * ...<br />
 * char cliente_str[TAM_CHAVE_CLIENTES_IDX + 1];<br />
 * sprintf(cliente_str, "%s%04d", cpf, rrn_cliente);<br />
 * btree_insert(cliente_str, &clientes_idx);<br />
 * ...<br />
 * </code>
 *
 * @param chave Chave a ser inserida na Árvore-B.
 * @param t Ponteiro para o índice do tipo Árvore-B no qual será inserida a chave.
 */
void btree_insert(char *chave, btree *t);

/**
 * Função auxiliar de inserção de uma chave (k) em uma Árvore-B (T). Atualiza os parâmetros da Árvore-B conforme necessário.<br />
 * Esta é uma função recursiva. Ela recebe o RRN do nó a ser trabalhado sobre.<br />
 * Comportamento de acordo com as especificações do PDF sobre Árvores-B e suas operações.<br />
 *
 * @param chave Chave a ser inserida na Árvore-B.
 * @param rrn RRN do nó no qual deverá ser processado.
 * @param t Ponteiro para o índice do tipo Árvore-B no qual será inserida a chave.
 * @return Retorna uma struct do tipo promovido_aux que contém a chave promovida e o RRN do filho direito.
 */
promovido_aux btree_insert_aux(char *chave, int rrn, btree *t);

/**
 * Função auxiliar para dividir um nó de uma Árvore-B (T). Atualiza os parâmetros conforme necessário.<br />
 * Comportamento de acordo com as especificações do PDF sobre Árvores-B e suas operações.<br />
 *
 * @param chave Chave a ser inserida na Árvore-B.
 * @param filho_direito RRN do filho direito.
 * @param rrn RRN do nó no qual deverá ser processado.
 * @param t Ponteiro para o índice do tipo Árvore-B no qual será inserida a chave.
 * @return Retorna uma struct do tipo promovido_aux que contém a chave promovida e o RRN do filho direito.
 */
promovido_aux btree_divide(char *chave, int filho_direito, int rrn, btree *t);

/**
 * Responsável por buscar uma chave (k) em uma Árvore-B (T). O valor de retorno indica se a chave foi encontrada ou não.
 * O ponteiro para a string result pode ser fornecido opcionalmente, e conterá o resultado encontrado.<br />
 * Esta é uma função recursiva. O parâmetro rrn recebe a raíz da Árvore-B na primeira chamada, e nas chamadas
 * subsequentes é o RRN do filho de acordo com o algoritmo fornecido.<br />
 * Comportamento de acordo com as especificações do PDF sobre Árvores-B e suas operações.<br />
 * <br />
 * Exemplos de uso:<br />
 * <code>
 * // Exemplo 1. A chave encontrada deverá ser retornada e o caminho não deve ser informado.<br />
 * ...<br />
 * char result[TAM_CHAVE_CLIENTES_IDX + 1];<br />
 * bool found = btree_search(result, false, cpf, clientes_idx.rrn_raiz, &clientes_idx);<br />
 * ...<br />
 * <br />
 * // Exemplo 2. Não há interesse na chave encontrada, apenas se ela existe, e o caminho não deve ser informado.<br />
 * ...<br />
 * bool found = btree_search(NULL, false, cpf, clientes_idx.rrn_raiz, &clientes_idx);<br />
 * ...<br />
 * <br />
 * // Exemplo 3. Busca por uma chave de tamanho variável (específico para o caso de buscas por chaves PIX).<br />
 * ...<br />
 * char chave_pix_str[TAM_MAX_CHAVE_PIX];<br />
 * strcpy(chave_pix_str, c.chaves_pix[i] + 1);<br />
 * strpadright(chave_pix_str, '#', TAM_MAX_CHAVE_PIX - 1);<br />
 * bool found = btree_search(NULL, false, chave_pix_str, chaves_pix_idx.rrn_raiz, &chaves_pix_idx);<br />
 * ...<br />
 * <br />
* // Exemplo 4. Há interesse no caminho feito para encontrar a chave.<br />
  * ...<br />
 * char result[TAM_CHAVE_CLIENTES_IDX + 1];<br />
 * printf(RRN_NOS);<br />
 * bool found = btree_search(result, true, cpf, clientes_idx.rrn_raiz, &clientes_idx);<br />
 * printf("\n");<br />
 * </code>
 *
 * @param result Ponteiro para ser escrita a chave encontrada. É ignorado caso NULL.
 * @param exibir_caminho Indica se o caminho percorrido deve ser impresso.
 * @param chave Chave a ser buscada na Árvore-B.
 * @param rrn RRN do nó no qual deverá ser processado. É o RRN da raíz da Árvore-B caso seja a primeira chamada.
 * @param t Ponteiro para o índice do tipo Árvore-B no qual será buscada a chave.
 * @return Indica se a chave foi encontrada.
 */
bool btree_search(char *result, bool exibir_caminho, char *chave, int rrn, btree *t);

/**
 * Função para percorrer uma Árvore-B (T) em ordem.<br />
 * Os parâmetros chave_inicio e chave_fim podem ser fornecidos opcionalmente, e contém o intervalo do percurso.
 * Caso chave_inicio e chave_fim sejam NULL, o índice inteiro é percorrido.
 * Esta é uma função recursiva. O parâmetro rrn recebe a raíz da Árvore-B na primeira chamada, e nas chamadas
 * subsequentes é o RRN do filho de acordo com o algoritmo de percursão em ordem.<br />
 * <br />
 * Exemplo de uso:<br />
 * <code>
 * // Exemplo 1. Intervalo não especificado.
 * ...<br />
 * bool imprimiu = btree_print_in_order(NULL, NULL, exibir_btree_cliente, clientes_idx.rrn_raiz, &clientes_idx);
 * ...<br />
 * <br />
 * // Exemplo 2. Imprime as transações contidas no intervalo especificado.
 * ...<br />
 * bool imprimiu = btree_print_in_order(data_inicio, data_fim, exibir_btree_timestamp_cpf_origem, timestamp_cpf_origem_idx.rrn_raiz, &timestamp_cpf_origem_idx);
 * ...<br />
 * </code>
 *
 * @param chave_inicio Começo do intervalo. É ignorado caso NULL.
 * @param chave_fim Fim do intervalo. É ignorado caso NULL.
 * @param exibir Função utilizada para imprimir uma chave no índice. É informada a chave para a função.
 * @param rrn RRN do nó no qual deverá ser processado.
 * @param t Ponteiro para o índice do tipo Árvore-B no qual será inserida a chave.
 * @return Indica se alguma chave foi impressa.
 */
bool btree_print_in_order(char *chave_inicio, char *chave_fim, bool (*exibir)(char *chave), int rrn, btree *t);

/**
 * Função interna para ler um nó em uma Árvore-B (T).<br />
 *
 * @param no No a ser lido da Árvore-B.
 * @param t Árvore-B na qual será feita a leitura do nó.
 */
btree_node btree_read(int rrn, btree *t);

/**
 * Função interna para escrever um nó em uma Árvore-B (T).<br />
 *
 * @param no No a ser escrito na Árvore-B.
 * @param t Árvore-B na qual será feita a escrita do nó.
 */
void btree_write(btree_node no, btree *t);

/**
 * Função interna para alocar o espaço necessário dos campos chaves (vetor de strings) e filhos (vetor de inteiros) da struct btree_node.<br />
 *
 * @param t Árvore-B base para o qual será alocado um struct btree_node.
 */
btree_node btree_node_malloc(btree *t);

/**
 * Função interna para liberar o espaço alocado dos campos chaves (vetor de strings) e filhos (vetor de inteiros) da struct btree_node.<br />
 *
 * @param t Árvore-B base para o qual será liberado o espaço alocado para um struct btree_node.
 */
void btree_node_free(btree_node no);

/**
 * Preenche uma string str com o caractere pad para completar o tamanho size.<br />
 *
 * @param str Ponteiro para a string a ser manipulada.
 * @param pad Caractere utilizado para fazer o preenchimento à direita.
 * @param size Tamanho desejado para a string.
 */
void strpadright(char *str, char pad, unsigned size);

/* <<< COLOQUE AQUI OS DEMAIS PROTÓTIPOS DE FUNÇÕES, SE NECESSÁRIO >>> */

/* ==========================================================================
 * ============================ FUNÇÃO PRINCIPAL ============================
 * =============================== NÃO ALTERAR ============================== */

int main() {
    // variáveis utilizadas pelo interpretador de comandos
    char input[500];
    uint64_t seed = 2;
    uint64_t time = 1616077800; // UTC 18/03/2021 14:30:00
    char cpf[TAM_CPF];
    char nome[TAM_MAX_NOME];
    char nascimento[TAM_NASCIMENTO];
    char email[TAM_MAX_EMAIL];
    char celular[TAM_CELULAR];
    double valor;
    char tipo_chave_pix;
    char chave_pix_origem[TAM_MAX_CHAVE_PIX];
    char chave_pix_destino[TAM_MAX_CHAVE_PIX];
    char chave_pix[TAM_MAX_CHAVE_PIX];
    char timestamp[TAM_TIMESTAMP];
    char data_inicio[TAM_TIMESTAMP];
    char data_fim[TAM_TIMESTAMP];

    scanf("SET BTREE_ORDER %d;\n", &btree_order);

    scanf("SET ARQUIVO_CLIENTES '%[^\n]\n", ARQUIVO_CLIENTES);
    int temp_len = strlen(ARQUIVO_CLIENTES);
    if (temp_len < 2) temp_len = 2; // corrige o tamanho caso a entrada seja omitida
    qtd_registros_clientes = (temp_len - 2) / TAM_REGISTRO_CLIENTE;
    ARQUIVO_CLIENTES[temp_len - 2] = '\0';

    scanf("SET ARQUIVO_TRANSACOES '%[^\n]\n", ARQUIVO_TRANSACOES);
    temp_len = strlen(ARQUIVO_TRANSACOES);
    if (temp_len < 2) temp_len = 2; // corrige o tamanho caso a entrada seja omitida
    qtd_registros_transacoes = (temp_len - 2) / TAM_REGISTRO_TRANSACAO;
    ARQUIVO_TRANSACOES[temp_len - 2] = '\0';

    // inicialização do gerador de números aleatórios e função de datas
    prng_srand(seed);
    set_time(time);

    criar_clientes_idx();
    criar_transacoes_idx();
    criar_chaves_pix_idx();
    criar_timestamp_cpf_origem_idx();

    while (1) {
        fgets(input, 500, stdin);
        printf("%s", input);

        clear_input(input);

        if (strcmp("", input) == 0)
            continue; // não avança o tempo nem imprime o comando este seja em branco

        /* Funções principais */
        if (sscanf(input, "INSERT INTO clientes VALUES ('%[^']', '%[^']', '%[^']', '%[^']', '%[^']');", cpf, nome, nascimento, email, celular) == 5)
            cadastrar_cliente_menu(cpf, nome, nascimento, email, celular);
        else if (sscanf(input, "UPDATE clientes SET saldo = saldo + %lf WHERE cpf = '%[^']';", &valor, cpf) == 2)
            alterar_saldo_menu(cpf, valor);
        else if (sscanf(input, "UPDATE clientes SET chaves_pix = array_append(chaves_pix, '%c') WHERE cpf = '%[^']';", &tipo_chave_pix, cpf) == 2)
            cadastrar_chave_pix_menu(cpf, tipo_chave_pix);
        else if (sscanf(input, "INSERT INTO transacoes VALUES ('%[^']', '%[^']', %lf);", chave_pix_origem, chave_pix_destino, &valor) == 3)
            transferir_menu(chave_pix_origem, chave_pix_destino, valor);

        /* Busca */
        else if (sscanf(input, "SELECT * FROM clientes WHERE cpf = '%[^']';", cpf) == 1)
            buscar_cliente_cpf_menu(cpf);
        else if (sscanf(input, "SELECT * FROM clientes WHERE '%[^']' = ANY (chaves_pix);", chave_pix) == 1)
            buscar_cliente_chave_pix_menu(chave_pix);
        else if (sscanf(input, "SELECT * FROM transacoes WHERE cpf_origem = '%[^']' AND timestamp = '%[^']';", cpf, timestamp) == 2)
            buscar_transacao_cpf_origem_timestamp_menu(cpf, timestamp);

        /* Listagem */
        else if (strcmp("SELECT * FROM clientes ORDER BY cpf ASC;", input) == 0)
            listar_clientes_cpf_menu();
        else if (sscanf(input, "SELECT * FROM transacoes WHERE timestamp BETWEEN '%[^']' AND '%[^']' ORDER BY timestamp ASC;", data_inicio, data_fim) == 2)
            listar_transacoes_periodo_menu(data_inicio, data_fim);
        else if (sscanf(input, "SELECT * FROM transacoes WHERE cpf_origem = '%[^']' AND timestamp BETWEEN '%[^']' AND '%[^']' ORDER BY timestamp ASC;", cpf, data_inicio, data_fim) == 3)
            listar_transacoes_cpf_origem_periodo_menu(cpf, data_inicio, data_fim);

        /* Imprimir arquivos de dados */
        else if (strcmp("\\echo file ARQUIVO_CLIENTES", input) == 0)
            imprimir_arquivo_clientes_menu();
        else if (strcmp("\\echo file ARQUIVO_TRANSACOES", input) == 0)
            imprimir_arquivo_transacoes_menu();

        /* Imprimir arquivos de índice */
        else if (strcmp("\\echo index clientes_idx", input) == 0)
            imprimir_clientes_idx_menu();
        else if (strcmp("\\echo index transacoes_idx", input) == 0)
            imprimir_transacoes_idx_menu();
        else if (strcmp("\\echo index chaves_pix_idx", input) == 0)
            imprimir_chaves_pix_idx_menu();
        else if (strcmp("\\echo index timestamp_cpf_origem_idx", input) == 0)
            imprimir_timestamp_cpf_origem_idx_menu();

        /* Liberar memória eventualmente alocada e encerrar programa */
        else if (strcmp("\\q", input) == 0)
            { liberar_memoria_menu(); return 0; }
        else if (sscanf(input, "SET SRAND %lu;", &seed) == 1)
            { prng_srand(seed); printf(SUCESSO); continue; }
        else if (sscanf(input, "SET TIME %lu;", &time) == 1)
            { set_time(time); printf(SUCESSO); continue; }
        else
            printf(ERRO_OPCAO_INVALIDA);

        tick_time();
    }
}

/* Cria o índice primário clientes_idx */
void criar_clientes_idx() {
    char cliente_str[TAM_CHAVE_CLIENTES_IDX + 1];
    for (unsigned i = 0; i < qtd_registros_clientes; ++i) {
        Cliente c = recuperar_registro_cliente(i);

        sprintf(cliente_str, "%s%04d", c.cpf, i);
        btree_insert(cliente_str, &clientes_idx);
    }
}

/* Cria o índice primário transacoes_idx */
void criar_transacoes_idx() {
    printf(ERRO_NAO_IMPLEMENTADO, "criar_transacoes_idx");
}

/* Cria o índice secundário chaves_pix_idx */
void criar_chaves_pix_idx() {
    printf(ERRO_NAO_IMPLEMENTADO, "criar_chaves_pix_idx");
}

/* Cria o índice secundário timestamp_cpf_origem_idx */
void criar_timestamp_cpf_origem_idx() {
    printf(ERRO_NAO_IMPLEMENTADO, "criar_timestamp_cpf_origem_idx");
}

/* Exibe um cliente dado seu RRN */
bool exibir_cliente(int rrn) {
    if (rrn < 0)
        return 0;

    Cliente c = recuperar_registro_cliente(rrn);

    printf("%s, %s, %s, %s, %s, %.2lf, {", c.cpf, c.nome, c.nascimento, c.email, c.celular, c.saldo);

    int imprimiu = 0;
    for (int i = 0; i < QTD_MAX_CHAVES_PIX; ++i) {
        if (c.chaves_pix[i][0] != '\0') {
            if (imprimiu)
                printf(", ");
            printf("%s", c.chaves_pix[i]);
            imprimiu = 1;
        }
    }
    printf("}\n");

    return 1;
}

/* Exibe uma transação dada seu RRN */
bool exibir_transacao(int rrn) {
    if (rrn < 0)
        return 0;

    Transacao t = recuperar_registro_transacao(rrn);

    printf("%s, %s, %.2lf, %s\n", t.cpf_origem, t.cpf_destino, t.valor, t.timestamp);

    return 1;
}

bool exibir_btree_cliente(char *chave) {
    int rrn = atoi(chave + TAM_CPF - 1);
    return exibir_cliente(rrn);
}

bool exibir_btree_transacao(char *chave) {
    printf(ERRO_NAO_IMPLEMENTADO, "exibir_btree_transacao");
}

bool exibir_btree_timestamp_cpf_origem(char *chave) {
    printf(ERRO_NAO_IMPLEMENTADO, "exibir_btree_timestamp_cpf_origem");
}

/* Recupera do arquivo de clientes o registro com o RRN
 * informado e retorna os dados na struct Cliente */
Cliente recuperar_registro_cliente(int rrn) {
    Cliente c;
    char temp[TAM_REGISTRO_CLIENTE + 1], *p;
    strncpy(temp, ARQUIVO_CLIENTES + (rrn * TAM_REGISTRO_CLIENTE), TAM_REGISTRO_CLIENTE);
    temp[TAM_REGISTRO_CLIENTE] = '\0';

    p = strtok(temp, ";");
    strcpy(c.cpf, p);
    p = strtok(NULL, ";");
    strcpy(c.nome, p);
    p = strtok(NULL, ";");
    strcpy(c.nascimento, p);
    p = strtok(NULL, ";");
    strcpy(c.email, p);
    p = strtok(NULL, ";");
    strcpy(c.celular, p);
    p = strtok(NULL, ";");
    c.saldo = atof(p);
    p = strtok(NULL, ";");

    for(int i = 0; i < QTD_MAX_CHAVES_PIX; ++i)
        c.chaves_pix[i][0] = '\0';

    if(p[0] != '#') {
        p = strtok(p, "&");

        for(int pos = 0; p; p = strtok(NULL, "&"), ++pos)
            strcpy(c.chaves_pix[pos], p);
    }

    return c;
}

/* Recupera do arquivo de transações o registro com o RRN
 * informado e retorna os dados na struct Transacao */
Transacao recuperar_registro_transacao(int rrn) {
    printf(ERRO_NAO_IMPLEMENTADO, "recuperar_registro_transacao");
}

/* Escreve no arquivo de clientes na posição informada (RRN)
 * os dados na struct Cliente */
void escrever_registro_cliente(Cliente c, int rrn) {
    printf(ERRO_NAO_IMPLEMENTADO, "escrever_registro_cliente");
}

/* Escreve no arquivo de transações na posição informada (RRN)
 * os dados na struct Transacao */
void escrever_registro_transacao(Transacao t, int rrn) {
    printf(ERRO_NAO_IMPLEMENTADO, "escrever_registro_transacao");
}

/* Funções principais */
void cadastrar_cliente_menu(char *cpf, char *nome, char *nascimento, char *email, char *celular) {
    printf(ERRO_NAO_IMPLEMENTADO, "cadastrar_cliente_menu");
}

void alterar_saldo_menu(char *cpf, double valor) {
    printf(ERRO_NAO_IMPLEMENTADO, "alterar_saldo_menu");
}

void cadastrar_chave_pix_menu(char *cpf, char tipo) {
    printf(ERRO_NAO_IMPLEMENTADO, "cadastrar_chave_pix_menu");
}

void transferir_menu(char *chave_pix_origem, char *chave_pix_destino, double valor) {
    printf(ERRO_NAO_IMPLEMENTADO, "transferir_menu");
}

/* Busca */
void buscar_cliente_cpf_menu(char *cpf) {
    printf(ERRO_NAO_IMPLEMENTADO, "buscar_cliente_cpf_menu");
}

void buscar_cliente_chave_pix_menu(char *chave_pix) {
    printf(ERRO_NAO_IMPLEMENTADO, "buscar_cliente_chave_pix_menu");
}

void buscar_transacao_cpf_origem_timestamp_menu(char *cpf, char *timestamp) {
    printf(ERRO_NAO_IMPLEMENTADO, "buscar_transacao_cpf_origem_timestamp_menu");
}

/* Listagem */
void listar_clientes_cpf_menu() {
    printf(ERRO_NAO_IMPLEMENTADO, "listar_clientes_cpf_menu");
}

void listar_transacoes_periodo_menu(char *data_inicio, char *data_fim) {
    printf(ERRO_NAO_IMPLEMENTADO, "listar_transacoes_periodo_menu");
}

void listar_transacoes_cpf_origem_periodo_menu(char *cpf, char *data_inicio, char *data_fim) {
    printf(ERRO_NAO_IMPLEMENTADO, "listar_transacoes_cpf_origem_periodo_menu");
}

/* Imprimir arquivos de dados */
void imprimir_arquivo_clientes_menu() {
    if (qtd_registros_clientes == 0)
        printf(ERRO_ARQUIVO_VAZIO);
    else
        printf("%s\n", ARQUIVO_CLIENTES);
}

void imprimir_arquivo_transacoes_menu() {
    if (qtd_registros_transacoes == 0)
        printf(ERRO_ARQUIVO_VAZIO);
    else
        printf("%s\n", ARQUIVO_TRANSACOES);
}

/* Imprimir arquivos de índice */
void imprimir_clientes_idx_menu() {
    if (clientes_idx.qtd_nos == 0)
        printf(ERRO_ARQUIVO_VAZIO);
    else
        printf("%s\n", ARQUIVO_CLIENTES_IDX);
}

void imprimir_transacoes_idx_menu() {
    if (transacoes_idx.qtd_nos == 0)
        printf(ERRO_ARQUIVO_VAZIO);
    else
        printf("%s\n", ARQUIVO_TRANSACOES_IDX);
}

void imprimir_chaves_pix_idx_menu() {
    if (chaves_pix_idx.qtd_nos == 0)
        printf(ERRO_ARQUIVO_VAZIO);
    else
        printf("%s\n", ARQUIVO_CHAVES_PIX_IDX);
}

void imprimir_timestamp_cpf_origem_idx_menu() {
    if (timestamp_cpf_origem_idx.qtd_nos == 0)
        printf(ERRO_ARQUIVO_VAZIO);
    else
        printf("%s\n", ARQUIVO_TIMESTAMP_CPF_ORIGEM_IDX);
}

/* Liberar memória e encerrar programa */
void liberar_memoria_menu() {
    // libere aqui espaços de memória eventualmente alocados
}

/* Função de comparação entre chaves do índice clientes_idx */
int order_clientes_idx(const void *key, const void *elem) {
    return strncmp(key, elem, TAM_CPF - 1);
}

/* Função de comparação entre chaves do índice transacoes_idx */
int order_transacoes_idx(const void *key, const void *elem) {
    printf(ERRO_NAO_IMPLEMENTADO, "order_transacoes_idx");
}

/* Função de comparação entre chaves do índice chaves_pix_idx */
int order_chaves_pix_idx(const void *key, const void *elem) {
    printf(ERRO_NAO_IMPLEMENTADO, "order_chaves_pix_idx");
}

/* Função de comparação entre chaves do índice timestamp_cpf_origem_idx */
int order_timestamp_cpf_origem_idx(const void *key, const void *elem) {
    printf(ERRO_NAO_IMPLEMENTADO, "order_timestamp_cpf_origem_idx");
}

void btree_insert(char *chave, btree *t) {
    printf(ERRO_NAO_IMPLEMENTADO, "btree_insert");
}

promovido_aux btree_insert_aux(char *chave, int rrn, btree *t) {
    printf(ERRO_NAO_IMPLEMENTADO, "btree_insert_aux");
}

promovido_aux btree_divide(char *chave, int filho_direito, int rrn, btree *t) {
    printf(ERRO_NAO_IMPLEMENTADO, "btree_divide");
}

bool btree_search(char *result, bool exibir_caminho, char *chave, int rrn, btree *t) {
    printf(ERRO_NAO_IMPLEMENTADO, "btree_search");
}

bool btree_print_in_order(char *chave_inicio, char *chave_fim, bool (*exibir)(char *chave), int rrn, btree *t) {
    printf(ERRO_NAO_IMPLEMENTADO, "btree_print_in_order");
}

btree_node btree_read(int rrn, btree *t) {
    printf(ERRO_NAO_IMPLEMENTADO, "btree_read");
}

void btree_write(btree_node no, btree *t) {
    printf(ERRO_NAO_IMPLEMENTADO, "btree_write");
}

btree_node btree_node_malloc(btree *t) {
    btree_node no;

    no.chaves = malloc((btree_order-1) * sizeof(char*));
    for (int i = 0; i < btree_order-1; ++i) {
        no.chaves[i] = malloc(t->tam_chave + 1);
        no.chaves[i][0] = '\0';
    }

    no.filhos = malloc(btree_order * sizeof(int));
    for (int i = 0; i < btree_order; ++i)
        no.filhos[i] = -1;

    return no;
}

void btree_node_free(btree_node no) {
    for (int i = 0; i < btree_order-1; ++i)
        free(no.chaves[i]);

    free(no.chaves);
    free(no.filhos);
}

void strpadright(char *str, char pad, unsigned size) {
    for (unsigned i = strlen(str); i < size; ++i)
        str[i] = pad;
    str[size] = '\0';
}
