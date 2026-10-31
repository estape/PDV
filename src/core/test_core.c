#include "zan_engine.h"
#include "zan_barcode.h"
#include "zan_pricing.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

int main(void) {
    printf("=== TESTANDO CORE C ZANTHUS ZEUS PDV ===\n");

    zan_engine_t engine;
    bool ok = zan_engine_init(&engine, 1, 1, "test_pdv.db");
    assert(ok);
    printf("[OK] zan_engine_init bem sucedido!\n");

    // Teste 1: Parser de Código de Barras
    zan_parsed_barcode_t parsed;
    ok = zan_barcode_parse("6 * 7891000100103", &parsed);
    assert(ok && parsed.quantidade == 6.0 && strcmp(parsed.codigo, "7891000100103") == 0);
    printf("[OK] Parser de multiplicacao (6 * code) aprovado!\n");

    ok = zan_barcode_parse("200050004500", &parsed);
    assert(ok && parsed.is_etiqueta_balanca && parsed.balanca_plu == 50 && parsed.balanca_valor_total == 4.50);
    printf("[OK] Parser de balanca padrao 2 (Queijo Prato) aprovado!\n");

    // Teste 2: Abertura e Login
    ok = zan_engine_login_operador(&engine, 101, "1234");
    assert(ok && engine.operador_ativo.codigo == 101);
    printf("[OK] Login de operador (101) aprovado!\n");

    // Teste 3: Suprimento Inicial
    char recibo[4096];
    ok = zan_engine_suprimento(&engine, 100.0, "FUNDO DE TROCO INICIAL", recibo, sizeof(recibo));
    assert(ok && engine.acumuladores.total_dinheiro_gaveta == 100.0);
    printf("[OK] Suprimento inicial de R$ 100.00 aprovado!\n");

    // Teste 4: Registro de Itens e Atacarejo
    // Arroz 5kg atacado: a partir de 5 unidades, preço cai de 29.90 para 26.50
    ok = zan_engine_registrar_item(&engine, "5 * 7891000100103");
    assert(ok);
    assert(engine.venda_atual.itens[0].atacado_aplicado == true);
    assert(engine.venda_atual.itens[0].preco_unitario == 26.50);
    printf("[OK] Regra de Atacarejo (Zanthus TH24) aplicada com sucesso (5 x R$ 26.50 = R$ %.2f)!\n",
           engine.venda_atual.itens[0].valor_total);

    // Produto de Balança
    ok = zan_engine_registrar_item(&engine, "200050004500");
    assert(ok);
    assert(engine.venda_atual.total_itens == 2);
    printf("[OK] Item de balanca registrado: %s (Qtd: %.3f kg = R$ %.2f)!\n",
           engine.venda_atual.itens[1].produto.descricao,
           engine.venda_atual.itens[1].quantidade,
           engine.venda_atual.itens[1].valor_total);

    // Teste 5: Subtotal e Pagamento
    ok = zan_engine_subtotalizar(&engine);
    assert(ok);
    printf("[OK] Subtotal da venda: R$ %.2f\n", engine.venda_atual.total_liquido);

    // Pagamento em dinheiro com troco
    double total = engine.venda_atual.total_liquido;
    ok = zan_engine_adicionar_pagamento(&engine, ZAN_FINALIZADORA_DINHEIRO, total + 50.0, "", "");
    assert(ok && engine.venda_atual.troco == 50.0);
    printf("[OK] Pagamento em dinheiro: R$ %.2f (Troco: R$ %.2f)\n", total + 50.0, engine.venda_atual.troco);

    // Teste 6: Finalização e Emissão do DANFE
    char danfe[8192];
    ok = zan_engine_finalizar_venda(&engine, danfe, sizeof(danfe));
    assert(ok);
    printf("[OK] Venda finalizada com sucesso! Chave de Acesso Fiscal gerada:\n%s\n",
           engine.venda_atual.chave_acesso_fiscal);

    // Teste 7: Leitura X
    char leitura_x[4096];
    ok = zan_engine_obter_leitura_x(&engine, leitura_x, sizeof(leitura_x));
    assert(ok);
    printf("[OK] Leitura X gerada com sucesso!\n");

    // Teste 8: JSON serialization
    char json_buf[8192];
    int json_len = zan_engine_to_json(&engine, json_buf, sizeof(json_buf));
    assert(json_len > 100);
    printf("[OK] Serializacao JSON para Frontend gerada (%d bytes)!\n", json_len);

    printf("\n=== TODOS OS TESTES DO CORE C FORAM CONCLUIDOS COM SUCESSO! ===\n");
    return 0;
}
