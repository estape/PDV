#include "zan_fiscal.h"
#include "zan_pricing.h"
#include <stdio.h>
#include <string.h>

static void format_chave_blocos(const char *chave, char *out) {
    if (!chave || !out) return;
    int len = (int)strlen(chave);
    int o = 0;
    for (int i = 0; i < len; i++) {
        out[o++] = chave[i];
        if ((i + 1) % 4 == 0 && (i + 1) < len) {
            out[o++] = ' ';
        }
    }
    out[o] = '\0';
}

int zan_fiscal_imprimir_danfe_texto(const zan_venda_t *venda, const zan_engine_t *engine, char *buffer, size_t max_len) {
    if (!venda || !buffer || max_len < 256) return 0;

    int offset = 0;
    double imposto = zan_pricing_calcular_imposto_aproximado(venda);
    char chave_formatada[64];
    format_chave_blocos(venda->chave_acesso_fiscal, chave_formatada);

    offset += snprintf(buffer + offset, max_len - offset,
        "========================================\n"
        "          %s          \n"
        "      CNPJ: %s  IE: %s      \n"
        "    RUA DO COMERCIO, 1000 - SAO PAULO/SP\n"
        "----------------------------------------\n"
        "       DANFE NFC-e - DOCUMENTO AUXILIAR \n"
        "   DA NOTA FISCAL DE CONSUMIDOR ELETRONICA\n"
        "%s"
        "----------------------------------------\n"
        "#|COD|DESC|QTD|UN|VL UN|TOTAL\n"
        "----------------------------------------\n",
        engine ? engine->razao_social : "SUPERMERCADO MODELO LTDA",
        engine ? engine->cnpj : "12.345.678/0001-90",
        engine ? engine->inscricao_estadual : "123.456.789.111",
        venda->contingencia ? "  ** EMITIDA EM CONTINGENCIA OFFLINE ** \n" : ""
    );

    int item_count = 0;
    for (int i = 0; i < venda->total_itens && offset < (int)max_len - 1024; i++) {
        const zan_item_venda_t *item = &venda->itens[i];
        if (item->cancelado) continue;
        item_count++;
        offset += snprintf(buffer + offset, max_len - offset,
            "%03d %-13s %-20.20s\n"
            "   %.3f %-2s X R$ %.2f %s%s= R$ %.2f\n",
            item_count,
            item->produto.codigo_barras,
            item->produto.descricao,
            item->quantidade,
            item->produto.unidade,
            item->preco_unitario,
            item->atacado_aplicado ? "[ATAC] " : "",
            item->desconto > 0 ? "(-DESC) " : "",
            item->valor_total
        );
    }

    offset += snprintf(buffer + offset, max_len - offset,
        "----------------------------------------\n"
        "QTD. TOTAL DE ITENS                  %03d\n"
        "VALOR TOTAL R$                    %8.2f\n"
        "DESCONTOS R$                      %8.2f\n"
        "VALOR A PAGAR R$                  %8.2f\n"
        "----------------------------------------\n"
        "FORMA DE PAGAMENTO            VALOR PAGO\n",
        item_count,
        venda->total_bruto,
        venda->total_descontos,
        venda->total_liquido
    );

    for (int p = 0; p < venda->total_pagamentos && offset < (int)max_len - 512; p++) {
        const zan_pagamento_t *pag = &venda->pagamentos[p];
        offset += snprintf(buffer + offset, max_len - offset,
            "%-24s      %8.2f\n",
            pag->descricao, pag->valor
        );
    }

    if (venda->troco > 0.0) {
        offset += snprintf(buffer + offset, max_len - offset,
            "TROCO R$                          %8.2f\n",
            venda->troco
        );
    }

    offset += snprintf(buffer + offset, max_len - offset,
        "----------------------------------------\n"
        "Trib. Totais Incid. (Lei 12.741): R$ %.2f\n"
        "----------------------------------------\n"
        "CONSUMIDOR: %s\n"
        "EMISSAO: %s  PDV: %03d  CUPOM: %06d\n"
        "CHAVE DE ACESSO:\n"
        "%s\n"
        "----------------------------------------\n"
        "  Consulte pela Chave de Acesso em:\n"
        "  https://www.nfce.fazenda.sp.gov.br\n"
        "========================================\n",
        imposto,
        strlen(venda->cpf_cnpj_cliente) > 0 ? venda->cpf_cnpj_cliente : "NAO INFORMADO",
        venda->data_hora_inicio,
        engine ? engine->pdv : 1,
        venda->cupom_numero,
        chave_formatada
    );

    return offset;
}

int zan_fiscal_imprimir_leitura_x(const zan_engine_t *engine, char *buffer, size_t max_len) {
    if (!engine || !buffer || max_len < 256) return 0;

    return snprintf(buffer, max_len,
        "========================================\n"
        "          %s          \n"
        "        LEITURA X (FUNCAO 201)         \n"
        "----------------------------------------\n"
        "LOJA: %03d   PDV: %03d   OPERADOR: %04d\n"
        "OPERADOR NOME: %s\n"
        "----------------------------------------\n"
        "CUPOM INICIAL:                    %06d\n"
        "CUPOM FINAL:                      %06d\n"
        "TOTAL DE VENDAS CONCLUIDAS:       %06d\n"
        "TOTAL DE VENDAS CANCELADAS:       %06d\n"
        "----------------------------------------\n"
        "TOTAL VENDA BRUTA R$              %8.2f\n"
        "TOTAL DESCONTOS R$                %8.2f\n"
        "TOTAL CANCELAMENTOS R$            %8.2f\n"
        "TOTAL LIQUIDO R$                  %8.2f\n"
        "----------------------------------------\n"
        "VALORES EM CAIXA (GAVETA):\n"
        "DINHEIRO EM GAVETA R$             %8.2f\n"
        "TOTAL SUPRIMENTOS (TROCO) R$      %8.2f\n"
        "TOTAL SANGRIAS R$                 %8.2f\n"
        "----------------------------------------\n"
        "TOTAL POR FINALIZADORA:\n"
        "DINHEIRO:                         %8.2f\n"
        "CARTAO DE DEBITO:                 %8.2f\n"
        "CARTAO DE CREDITO:                %8.2f\n"
        "PIX:                              %8.2f\n"
        "========================================\n",
        engine->razao_social,
        engine->loja, engine->pdv, engine->operador_ativo.codigo,
        engine->operador_ativo.nome,
        engine->acumuladores.cupom_inicial,
        engine->acumuladores.cupom_final,
        engine->acumuladores.total_vendas_concluidas,
        engine->acumuladores.total_vendas_canceladas,
        engine->acumuladores.acumulado_vendas_bruto,
        engine->acumuladores.acumulado_descontos,
        engine->acumuladores.acumulado_cancelamentos,
        engine->acumuladores.acumulado_liquido,
        engine->acumuladores.total_dinheiro_gaveta,
        engine->acumuladores.total_suprimentos,
        engine->acumuladores.total_sangrias,
        engine->acumuladores.total_por_finalizadora[ZAN_FINALIZADORA_DINHEIRO],
        engine->acumuladores.total_por_finalizadora[ZAN_FINALIZADORA_TEF_DEBITO],
        engine->acumuladores.total_por_finalizadora[ZAN_FINALIZADORA_TEF_CREDITO],
        engine->acumuladores.total_por_finalizadora[ZAN_FINALIZADORA_PIX]
    );
}

int zan_fiscal_imprimir_comprovante_gaveta(int tipo_funcao, double valor, const char *motivo, const zan_engine_t *engine, char *buffer, size_t max_len) {
    if (!engine || !buffer || max_len < 128) return 0;

    const char *titulo = (tipo_funcao == ZAN_FUNC_SUPRIMENTO) ? "SUPRIMENTO DE CAIXA / TROCO" : "SANGRIA DE CAIXA / RETIRADA";

    return snprintf(buffer, max_len,
        "========================================\n"
        "          %s          \n"
        "         COMPROVANTE NAO FISCAL         \n"
        "----------------------------------------\n"
        "OPERACAO: %s\n"
        "FUNCAO ZANTHUS: %d\n"
        "LOJA: %03d  PDV: %03d  OPERADOR: %04d\n"
        "----------------------------------------\n"
        "VALOR DA OPERACAO R$              %8.2f\n"
        "SALDO GAVETA ATUAL R$             %8.2f\n"
        "MOTIVO / HISTORICO: %s\n"
        "----------------------------------------\n"
        "\n\n\n"
        "________________________________________\n"
        "  ASSINATURA DO FISCAL / SUPERVISOR\n"
        "========================================\n",
        engine->razao_social,
        titulo,
        tipo_funcao,
        engine->loja, engine->pdv, engine->operador_ativo.codigo,
        valor,
        engine->acumuladores.total_dinheiro_gaveta,
        motivo ? motivo : "OPERACAO DE ROTINA"
    );
}

int zan_fiscal_imprimir_fechamento_caixa(const zan_engine_t *engine, char *buffer, size_t max_len) {
    if (!engine || !buffer || max_len < 256) return 0;

    return snprintf(buffer, max_len,
        "========================================\n"
        "          %s          \n"
        "     FECHAMENTO DE CAIXA (FUNCAO 159)   \n"
        "----------------------------------------\n"
        "LOJA: %03d  PDV: %03d  DATA: HOJE\n"
        "OPERADOR: %04d - %s\n"
        "----------------------------------------\n"
        "TOTAL DE CUPONS EMITIDOS:         %06d\n"
        "TOTAL VENDA BRUTA R$              %8.2f\n"
        "TOTAL CANCELAMENTOS R$            %8.2f\n"
        "TOTAL DESCONTOS R$                %8.2f\n"
        "TOTAL VENDA LIQUIDA R$            %8.2f\n"
        "----------------------------------------\n"
        "MOVIMENTO FINANCEIRO DE GAVETA:\n"
        "TOTAL SUPRIMENTOS R$              %8.2f\n"
        "TOTAL SANGRIAS R$                 %8.2f\n"
        "SALDO APURADO EM DINHEIRO R$      %8.2f\n"
        "----------------------------------------\n"
        "SITUACAO: CAIXA FECHADO COM SUCESSO\n"
        "DADOS SINCRONIZADOS COM CONCENTRADOR\n"
        "========================================\n",
        engine->razao_social,
        engine->loja, engine->pdv, engine->operador_ativo.codigo,
        engine->operador_ativo.nome,
        engine->acumuladores.total_vendas_concluidas,
        engine->acumuladores.acumulado_vendas_bruto,
        engine->acumuladores.acumulado_cancelamentos,
        engine->acumuladores.acumulado_descontos,
        engine->acumuladores.acumulado_liquido,
        engine->acumuladores.total_suprimentos,
        engine->acumuladores.total_sangrias,
        engine->acumuladores.total_dinheiro_gaveta
    );
}
