#include "zan_pricing.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

void zan_pricing_calcular_item(zan_item_venda_t *item) {
    if (!item) return;

    item->preco_tabela = item->produto.preco_venda;

    // Regra de Atacarejo (Zanthus TH24)
    if (item->produto.atacado_qtd_minima > 0 &&
        item->quantidade >= item->produto.atacado_qtd_minima &&
        item->produto.atacado_preco > 0.0) {
        item->preco_unitario = item->produto.atacado_preco;
        item->atacado_aplicado = true;
    } else {
        item->preco_unitario = item->produto.preco_venda;
        item->atacado_aplicado = false;
    }

    double total_bruto = item->quantidade * item->preco_unitario;
    item->valor_total = total_bruto - item->desconto + item->acrescimo;
    if (item->valor_total < 0.0) item->valor_total = 0.0;
}

bool zan_pricing_desconto_item_valor(zan_item_venda_t *item, double valor) {
    if (!item || valor < 0.0) return false;
    double max_desconto = item->quantidade * item->preco_unitario;
    if (valor > max_desconto) valor = max_desconto;
    item->desconto = valor;
    zan_pricing_calcular_item(item);
    return true;
}

bool zan_pricing_desconto_item_perc(zan_item_venda_t *item, double percentual) {
    if (!item || percentual < 0.0 || percentual > 100.0) return false;
    double max_bruto = item->quantidade * item->preco_unitario;
    double valor = (max_bruto * percentual) / 100.0;
    return zan_pricing_desconto_item_valor(item, valor);
}

void zan_pricing_recalcular_venda(zan_venda_t *venda) {
    if (!venda) return;

    double bruto = 0.0;
    double descontos_itens = 0.0;
    double acrescimos_itens = 0.0;

    for (int i = 0; i < venda->total_itens; i++) {
        if (!venda->itens[i].cancelado) {
            zan_pricing_calcular_item(&venda->itens[i]);
            bruto += (venda->itens[i].quantidade * venda->itens[i].preco_unitario);
            descontos_itens += venda->itens[i].desconto;
            acrescimos_itens += venda->itens[i].acrescimo;
        }
    }

    venda->total_bruto = bruto;
    venda->total_acrescimos = acrescimos_itens;

    // Se o desconto global for maior que o desconto nos itens, mantém o global
    if (venda->total_descontos < descontos_itens) {
        venda->total_descontos = descontos_itens;
    }

    venda->total_liquido = venda->total_bruto - venda->total_descontos + venda->total_acrescimos;
    if (venda->total_liquido < 0.0) venda->total_liquido = 0.0;

    // Total pago por finalizadoras
    double pago = 0.0;
    for (int p = 0; p < venda->total_pagamentos; p++) {
        pago += venda->pagamentos[p].valor;
    }
    venda->total_pago = pago;

    if (venda->total_pago >= venda->total_liquido) {
        venda->troco = venda->total_pago - venda->total_liquido;
        venda->saldo_restante = 0.0;
    } else {
        venda->troco = 0.0;
        venda->saldo_restante = venda->total_liquido - venda->total_pago;
    }
}

bool zan_pricing_desconto_subtotal_valor(zan_venda_t *venda, double valor) {
    if (!venda || valor < 0.0) return false;
    if (valor > venda->total_bruto) valor = venda->total_bruto;
    venda->total_descontos = valor;
    zan_pricing_recalcular_venda(venda);
    return true;
}

bool zan_pricing_desconto_subtotal_perc(zan_venda_t *venda, double percentual) {
    if (!venda || percentual < 0.0 || percentual > 100.0) return false;
    double valor = (venda->total_bruto * percentual) / 100.0;
    return zan_pricing_desconto_subtotal_valor(venda, valor);
}

double zan_pricing_calcular_imposto_aproximado(const zan_venda_t *venda) {
    if (!venda) return 0.0;
    // Média nacional IBPT de tributos federais e estaduais sobre varejo de alimentos/mercadorias: ~31.4%
    return round((venda->total_liquido * 0.314) * 100.0) / 100.0;
}
