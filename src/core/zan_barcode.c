#include "zan_barcode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void zan_barcode_trim(char *str) {
    if (!str) return;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

bool zan_barcode_parse(const char *input, zan_parsed_barcode_t *out) {
    if (!input || !out) return false;
    memset(out, 0, sizeof(zan_parsed_barcode_t));
    out->quantidade = 1.0;

    char buffer[128];
    strncpy(buffer, input, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    zan_barcode_trim(buffer);

    if (strlen(buffer) == 0) return false;

    // Procura por multiplicador '*' ou 'X'/'x'
    char *asterisk = strchr(buffer, '*');
    if (!asterisk) asterisk = strchr(buffer, 'x');
    if (!asterisk) asterisk = strchr(buffer, 'X');

    if (asterisk) {
        *asterisk = '\0';
        char *qty_str = buffer;
        char *code_str = asterisk + 1;
        zan_barcode_trim(qty_str);
        zan_barcode_trim(code_str);

        // Substitui vírgula por ponto para conversão correta em locales pt-BR
        char *comma = strchr(qty_str, ',');
        if (comma) *comma = '.';

        double qty = atof(qty_str);
        if (qty <= 0.0001) qty = 1.0;
        out->quantidade = qty;

        strncpy(out->codigo, code_str, ZAN_MAX_BARCODE - 1);
    } else {
        out->quantidade = 1.0;
        strncpy(out->codigo, buffer, ZAN_MAX_BARCODE - 1);
    }

    // Verifica padrão brasileiro de balança de açougue / padaria / hortifruti (EAN-13 iniciando com '2')
    // Padrão: 2 CCCCC VVVVV D
    // Tamanho total 12 ou 13 dígitos
    size_t len = strlen(out->codigo);
    if (len >= 12 && len <= 13 && out->codigo[0] == '2') {
        char plu_str[6] = {0};
        char val_str[6] = {0};

        // Pega 4 a 5 dígitos do código interno
        strncpy(plu_str, out->codigo + 1, 5);
        plu_str[5] = '\0';

        // Pega 5 dígitos do valor (em centavos)
        strncpy(val_str, out->codigo + 6, 5);
        val_str[5] = '\0';

        out->is_etiqueta_balanca = true;
        out->balanca_plu = atoi(plu_str);
        out->balanca_valor_total = atof(val_str) / 100.0;
    }

    return strlen(out->codigo) > 0;
}
