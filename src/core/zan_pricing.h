#ifndef ZAN_PRICING_H
#define ZAN_PRICING_H

#include "zan_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Calcula o preço do item considerando atacarejo (TH24) e descontos
void zan_pricing_calcular_item(zan_item_venda_t *item);

// Aplica desconto em valor monetário ao item
bool zan_pricing_desconto_item_valor(zan_item_venda_t *item, double valor);

// Aplica desconto percentual ao item
bool zan_pricing_desconto_item_perc(zan_item_venda_t *item, double percentual);

// Recalcula totais da venda (bruto, descontos, acréscimos, líquido e saldo restante)
void zan_pricing_recalcular_venda(zan_venda_t *venda);

// Aplica desconto no subtotal (em valor ou %)
bool zan_pricing_desconto_subtotal_valor(zan_venda_t *venda, double valor);
bool zan_pricing_desconto_subtotal_perc(zan_venda_t *venda, double percentual);

// Estima impostos aproximados (Lei 12.741/2012 - IBPT: ~31% médio no varejo)
double zan_pricing_calcular_imposto_aproximado(const zan_venda_t *venda);

#ifdef __cplusplus
}
#endif

#endif // ZAN_PRICING_H
