#ifndef ZAN_FISCAL_H
#define ZAN_FISCAL_H

#include "zan_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Gera o texto formatado do DANFE NFC-e / Extrato SAT para impressora térmica de 40 colunas
int zan_fiscal_imprimir_danfe_texto(const zan_venda_t *venda, const zan_engine_t *engine, char *buffer, size_t max_len);

// Gera o relatório de Leitura X (Função 201) em 40 colunas
int zan_fiscal_imprimir_leitura_x(const zan_engine_t *engine, char *buffer, size_t max_len);

// Gera o comprovante de Sangria / Suprimento
int zan_fiscal_imprimir_comprovante_gaveta(int tipo_funcao, double valor, const char *motivo, const zan_engine_t *engine, char *buffer, size_t max_len);

// Gera o comprovante de Fechamento do Caixa (Função 159)
int zan_fiscal_imprimir_fechamento_caixa(const zan_engine_t *engine, char *buffer, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ZAN_FISCAL_H
