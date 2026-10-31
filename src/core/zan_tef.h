#ifndef ZAN_TEF_H
#define ZAN_TEF_H

#include "zan_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TEF_STATUS_IDLE = 0,
    TEF_STATUS_AGUARDANDO_CARTAO = 1,
    TEF_STATUS_AGUARDANDO_SENHA = 2,
    TEF_STATUS_PROCESSANDO = 3,
    TEF_STATUS_APROVADO = 4,
    TEF_STATUS_NEGADO = 5
} zan_tef_status_t;

typedef struct {
    zan_tef_status_t status;
    zan_finalizadora_t modalidade; // ZAN_FINALIZADORA_TEF_DEBITO ou CREDITO
    double valor;
    int parcelas;
    char mensagem_visor[64];
    char nsu[32];
    char codigo_autorizacao[32];
    char rede[32];
    char comprovante_cliente[1024];
    char comprovante_loja[1024];
} zan_tef_transacao_t;

// Inicia uma transação TEF simulada
void zan_tef_iniciar(zan_tef_transacao_t *tef, zan_finalizadora_t modalidade, double valor, int parcelas);

// Avança para o próximo passo do pinpad
bool zan_tef_avancar_passo(zan_tef_transacao_t *tef, const char *input_senha);

// Finaliza com sucesso e gera os comprovantes TEF
void zan_tef_aprovar(zan_tef_transacao_t *tef, int pdv, int cupom);

#ifdef __cplusplus
}
#endif

#endif // ZAN_TEF_H
