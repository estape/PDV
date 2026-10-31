#ifndef ZAN_BARCODE_H
#define ZAN_BARCODE_H

#include <stdbool.h>
#include "zan_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double quantidade;
    char codigo[ZAN_MAX_BARCODE];
    bool is_etiqueta_balanca; // iniciada com 2
    int balanca_plu;
    double balanca_valor_total; // calculado pelo visor da balança
} zan_parsed_barcode_t;

// Processa uma linha digitada ou lida pelo leitor de código de barras
// Exemplos:
// "7891234567890" -> qtd = 1.0, codigo = "7891234567890"
// "6*7891234567890" ou "6 * 7891234567890" -> qtd = 6.0, codigo = "7891234567890"
// "200050004500" -> balança pesável, PLU = 50, valor total = R$ 4.50
bool zan_barcode_parse(const char *input, zan_parsed_barcode_t *out);

// Limpa espaços em branco e caracteres de controle
void zan_barcode_trim(char *str);

#ifdef __cplusplus
}
#endif

#endif // ZAN_BARCODE_H
