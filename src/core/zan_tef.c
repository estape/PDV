#include "zan_tef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void zan_tef_iniciar(zan_tef_transacao_t *tef, zan_finalizadora_t modalidade, double valor, int parcelas) {
    if (!tef) return;
    memset(tef, 0, sizeof(zan_tef_transacao_t));
    tef->status = TEF_STATUS_AGUARDANDO_CARTAO;
    tef->modalidade = modalidade;
    tef->valor = valor;
    tef->parcelas = parcelas > 0 ? parcelas : 1;
    strncpy(tef->mensagem_visor, "INSIRA OU APROXIME O CARTAO", sizeof(tef->mensagem_visor) - 1);
    strncpy(tef->rede, "SITEF / REDECARD", sizeof(tef->rede) - 1);
}

bool zan_tef_avancar_passo(zan_tef_transacao_t *tef, const char *input_senha) {
    if (!tef) return false;

    switch (tef->status) {
        case TEF_STATUS_AGUARDANDO_CARTAO:
            tef->status = TEF_STATUS_AGUARDANDO_SENHA;
            strncpy(tef->mensagem_visor, "DIGITE A SENHA NO PINPAD", sizeof(tef->mensagem_visor) - 1);
            return true;

        case TEF_STATUS_AGUARDANDO_SENHA:
            tef->status = TEF_STATUS_PROCESSANDO;
            strncpy(tef->mensagem_visor, "PROCESSANDO... AGUARDE...", sizeof(tef->mensagem_visor) - 1);
            return true;

        case TEF_STATUS_PROCESSANDO:
            tef->status = TEF_STATUS_APROVADO;
            strncpy(tef->mensagem_visor, "TRANSACAO AUTORIZADA! RETIRE O CARTAO", sizeof(tef->mensagem_visor) - 1);
            return true;

        default:
            return false;
    }
}

void zan_tef_aprovar(zan_tef_transacao_t *tef, int pdv, int cupom) {
    if (!tef) return;

    tef->status = TEF_STATUS_APROVADO;
    strncpy(tef->mensagem_visor, "TRANSACAO AUTORIZADA! RETIRE O CARTAO", sizeof(tef->mensagem_visor) - 1);

    srand((unsigned int)time(NULL));
    int rand_nsu = 100000 + (rand() % 899999);
    int rand_aut = 200000 + (rand() % 799999);

    snprintf(tef->nsu, sizeof(tef->nsu), "%06d", rand_nsu);
    snprintf(tef->codigo_autorizacao, sizeof(tef->codigo_autorizacao), "%06d", rand_aut);

    const char *tipo_desc = (tef->modalidade == ZAN_FINALIZADORA_TEF_DEBITO) ? "CARTAO DE DEBITO" : "CARTAO DE CREDITO";

    // Comprovante Via do Cliente
    snprintf(tef->comprovante_cliente, sizeof(tef->comprovante_cliente),
        "========================================\n"
        "           COMPROVANTE TEF              \n"
        "           VIA DO CLIENTE               \n"
        "----------------------------------------\n"
        "REDE: %s\n"
        "MODALIDADE: %s\n"
        "PDV: %03d   CUPOM: %06d\n"
        "VALOR TOTAL: R$ %8.2f\n"
        "PARCELAS: %02d\n"
        "CARTAO: **** **** **** 4321\n"
        "NSU TEF: %s\n"
        "AUTORIZACAO: %s\n"
        "STATUS: APROVADA COM SUCESSO\n"
        "========================================\n",
        tef->rede, tipo_desc, pdv, cupom, tef->valor, tef->parcelas,
        tef->nsu, tef->codigo_autorizacao
    );

    // Comprovante Via do Estabelecimento
    snprintf(tef->comprovante_loja, sizeof(tef->comprovante_loja),
        "========================================\n"
        "           COMPROVANTE TEF              \n"
        "       VIA DO ESTABELECIMENTO           \n"
        "----------------------------------------\n"
        "REDE: %s\n"
        "MODALIDADE: %s\n"
        "PDV: %03d   CUPOM: %06d\n"
        "VALOR: R$ %8.2f\n"
        "NSU: %s  AUT: %s\n"
        "TRANSACAO APROVADA - SITEF HOMOLOGADO\n"
        "========================================\n",
        tef->rede, tipo_desc, pdv, cupom, tef->valor,
        tef->nsu, tef->codigo_autorizacao
    );
}
