#ifndef ZAN_RECORDS_H
#define ZAN_RECORDS_H

#include "zan_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Gera chave de acesso fiscal brasileira padrão SEFAZ (44 dígitos numéricos)
// Formato: cUF(2) + AAMM(4) + CNPJ(14) + mod(2) + serie(3) + nNF(9) + tpEmis(1) + cNF(8) + cDV(1)
void zan_records_gerar_chave_fiscal(const zan_venda_t *venda, const char *cnpj, int serie, char *out_chave);

// Formata o XML de movimento Zanthus completo (mestre ZAN_M00 e filhas ZAN_M01, M03, M04, M43, M45)
// Compatível com a especificação ZMI / Manager Zanthus
int zan_records_gerar_xml_movimento(const zan_venda_t *venda, const zan_engine_t *engine, char *buffer, size_t max_len);

// Gera o registro ZAN_M07 (Sangria e Suprimento fora de cupom)
int zan_records_gerar_xml_sangria_suprimento(int loja, int pdv, int operador, int tipo_funcao, double valor, const char *motivo, char *buffer, size_t max_len);

// Gera o registro ZAN_M10 (Fechamento do Operador / Resumo de Caixa)
int zan_records_gerar_xml_fechamento_operador(const zan_engine_t *engine, char *buffer, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ZAN_RECORDS_H
