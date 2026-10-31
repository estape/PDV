#include "zan_records.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void zan_records_gerar_chave_fiscal(const zan_venda_t *venda, const char *cnpj, int serie, char *out_chave) {
    if (!out_chave) return;

    // Limpa caracteres especiais do CNPJ
    char clean_cnpj[16] = "00000000000000";
    if (cnpj) {
        int idx = 0;
        for (int i = 0; cnpj[i] && idx < 14; i++) {
            if (cnpj[i] >= '0' && cnpj[i] <= '9') {
                clean_cnpj[idx++] = cnpj[i];
            }
        }
        clean_cnpj[idx] = '\0';
    }

    // Data atual AAMM
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int ano = (tm ? tm->tm_year % 100 : 26);
    int mes = (tm ? tm->tm_mon + 1 : 10);

    int cUF = 35; // SP
    int mod = 65; // NFC-e
    int nNF = venda ? venda->cupom_numero : 1;
    int tpEmis = (venda && venda->contingencia) ? 9 : 1; // 1: Normal, 9: Offline
    int cNF = 12345678;

    // 43 dígitos base
    char base[44];
    snprintf(base, sizeof(base), "%02d%02d%02d%014s%02d%03d%09d%01d%08d",
             cUF, ano, mes, clean_cnpj, mod, serie, nNF, tpEmis, cNF);

    // Cálculo do dígito verificador (módulo 11)
    int pesos[] = {2, 3, 4, 5, 6, 7, 8, 9};
    int soma = 0;
    int peso_idx = 0;
    for (int i = 42; i >= 0; i--) {
        soma += (base[i] - '0') * pesos[peso_idx];
        peso_idx = (peso_idx + 1) % 8;
    }
    int resto = soma % 11;
    int dv = (resto == 0 || resto == 1) ? 0 : (11 - resto);

    snprintf(out_chave, 48, "%s%d", base, dv);
}

int zan_records_gerar_xml_movimento(const zan_venda_t *venda, const zan_engine_t *engine, char *buffer, size_t max_len) {
    if (!venda || !buffer || max_len < 128) return 0;

    int written = 0;
    int offset = 0;

    // Cabeçalho ZAN_M00 (Mestre)
    offset += snprintf(buffer + offset, max_len - offset,
        "<M00>\n"
        "  <M00ZA>%d</M00ZA>\n"      // Loja
        "  <M00ZB>%d</M00ZB>\n"      // Caixa / PDV
        "  <M00AC>%d</M00AC>\n"      // ECF / PDV lógico
        "  <M00AD>%d</M00AD>\n"      // Cupom / Número da operação
        "  <M00AF>%s</M00AF>\n"      // Data
        "  <M00AE>102</M00AE>\n"     // Código da função (Venda)
        "  <M00_CRO>1</M00_CRO>\n"
        "  <M00_TRN>%d</M00_TRN>\n",
        engine ? engine->loja : 1,
        engine ? engine->pdv : 1,
        engine ? engine->pdv : 1,
        venda->cupom_numero,
        venda->data_hora_inicio,
        venda->cupom_numero
    );

    // Resumo do Documento Fiscal ZAN_M45 (NFC-e / SAT)
    offset += snprintf(buffer + offset, max_len - offset,
        "  <ZAN_M45S>\n"
        "    <ZAN_M45>\n"
        "      <M45AH>%d</M45AH>\n"      // Operador
        "      <M45BV>%s</M45BV>\n"      // CPF/CNPJ Consumidor
        "      <M45XB>%s</M45XB>\n"      // Chave de Acesso Fiscal
        "      <M45XE>NFCe-%03d-%06d</M45XE>\n"
        "      <M45AL>%.2f</M45AL>\n"    // Total Bruto
        "      <M45AP>%.2f</M45AP>\n"    // Total Líquido
        "      <M45AQ>%.2f</M45AQ>\n"    // Total Descontos
        "      <M45CONTINGENCIA>%d</M45CONTINGENCIA>\n"
        "    </ZAN_M45>\n"
        "  </ZAN_M45S>\n",
        venda->operador.codigo,
        venda->cpf_cnpj_cliente,
        venda->chave_acesso_fiscal,
        engine ? engine->pdv : 1,
        venda->cupom_numero,
        venda->total_bruto,
        venda->total_liquido,
        venda->total_descontos,
        venda->contingencia ? 1 : 0
    );

    // Itens Vendidos ZAN_M43
    offset += snprintf(buffer + offset, max_len - offset, "  <ZAN_M43S>\n");
    for (int i = 0; i < venda->total_itens && offset < (int)max_len - 512; i++) {
        const zan_item_venda_t *item = &venda->itens[i];
        offset += snprintf(buffer + offset, max_len - offset,
            "    <ZAN_M43>\n"
            "      <M43ZB>%d</M43ZB>\n"          // Caixa
            "      <M43AH>%s</M43AH>\n"          // Código Produto
            "      <M43EAN>%s</M43EAN>\n"        // EAN
            "      <M43DESC><![CDATA[%s]]></M43DESC>\n"
            "      <M43AO>%.3f</M43AO>\n"        // Quantidade
            "      <M43AP>%.2f</M43AP>\n"        // Preço Total
            "      <M43AQ>%.2f</M43AQ>\n"        // Desconto
            "      <M43AY>%.2f</M43AY>\n"        // Alíquota ICMS
            "      <M43CF>%d</M43CF>\n"          // Cancelado (1 ou 0)
            "    </ZAN_M43>\n",
            engine ? engine->pdv : 1,
            item->produto.codigo_interno,
            item->produto.codigo_barras,
            item->produto.descricao,
            item->quantidade,
            item->valor_total,
            item->desconto,
            item->produto.aliquota_icms,
            item->cancelado ? 1 : 0
        );
    }
    offset += snprintf(buffer + offset, max_len - offset, "  </ZAN_M43S>\n");

    // Finalizadoras Utilizadas ZAN_M04
    offset += snprintf(buffer + offset, max_len - offset, "  <ZAN_M04S>\n");
    for (int p = 0; p < venda->total_pagamentos && offset < (int)max_len - 256; p++) {
        const zan_pagamento_t *pag = &venda->pagamentos[p];
        offset += snprintf(buffer + offset, max_len - offset,
            "    <ZAN_M04>\n"
            "      <M04AI>%d</M04AI>\n"          // Código Finalizadora
            "      <M04DESC>%s</M04DESC>\n"
            "      <M04AK>%.2f</M04AK>\n"        // Valor Total Pago
            "      <M04BR>%s</M04BR>\n"          // NSU TEF
            "      <M04AUT>%s</M04AUT>\n"        // Código Autorização
            "    </ZAN_M04>\n",
            (int)pag->finalizadora,
            pag->descricao,
            pag->valor,
            pag->nsu,
            pag->autorizacao
        );
    }
    offset += snprintf(buffer + offset, max_len - offset, "  </ZAN_M04S>\n");

    offset += snprintf(buffer + offset, max_len - offset, "</M00>\n");
    return offset;
}

int zan_records_gerar_xml_sangria_suprimento(int loja, int pdv, int operador, int tipo_funcao, double valor, const char *motivo, char *buffer, size_t max_len) {
    if (!buffer || max_len < 64) return 0;
    return snprintf(buffer, max_len,
        "<M00>\n"
        "  <M00ZA>%d</M00ZA>\n"
        "  <M00ZB>%d</M00ZB>\n"
        "  <M00AE>%d</M00AE>\n" // 157=Suprimento, 158=Sangria
        "  <ZAN_M07>\n"
        "    <M07OPERADOR>%d</M07OPERADOR>\n"
        "    <M07VALOR>%.2f</M07VALOR>\n"
        "    <M07MOTIVO><![CDATA[%s]]></M07MOTIVO>\n"
        "  </ZAN_M07>\n"
        "</M00>\n",
        loja, pdv, tipo_funcao, operador, valor, motivo ? motivo : ""
    );
}

int zan_records_gerar_xml_fechamento_operador(const zan_engine_t *engine, char *buffer, size_t max_len) {
    if (!engine || !buffer || max_len < 64) return 0;
    return snprintf(buffer, max_len,
        "<M00>\n"
        "  <M00ZA>%d</M00ZA>\n"
        "  <M00ZB>%d</M00ZB>\n"
        "  <M00AE>159</M00AE>\n"
        "  <ZAN_M10>\n"
        "    <M10OPERADOR>%d</M10OPERADOR>\n"
        "    <M10CUPOM_INI>%d</M10CUPOM_INI>\n"
        "    <M10CUPOM_FIM>%d</M10CUPOM_FIM>\n"
        "    <M10TOTAL_BRUTO>%.2f</M10TOTAL_BRUTO>\n"
        "    <M10TOTAL_LIQUIDO>%.2f</M10TOTAL_LIQUIDO>\n"
        "    <M10TOTAL_SANGRIA>%.2f</M10TOTAL_SANGRIA>\n"
        "    <M10TOTAL_SUPRIMENTO>%.2f</M10TOTAL_SUPRIMENTO>\n"
        "    <M10GAVETA_DINHEIRO>%.2f</M10GAVETA_DINHEIRO>\n"
        "  </ZAN_M10>\n"
        "</M00>\n",
        engine->loja, engine->pdv, engine->operador_ativo.codigo,
        engine->acumuladores.cupom_inicial, engine->acumuladores.cupom_final,
        engine->acumuladores.acumulado_vendas_bruto,
        engine->acumuladores.acumulado_liquido,
        engine->acumuladores.total_sangrias,
        engine->acumuladores.total_suprimentos,
        engine->acumuladores.total_dinheiro_gaveta
    );
}
