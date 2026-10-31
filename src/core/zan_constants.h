#ifndef ZAN_CONSTANTS_H
#define ZAN_CONSTANTS_H

// ============================================================================
// CÓDIGOS DE FUNÇÕES OFICIAIS ZANTHUS ZEUS PDV (Manual M008)
// ============================================================================

// Níveis de autorização e perfis
#define ZAN_NIVEL_OPERADOR      1
#define ZAN_NIVEL_SUPERVISOR    2
#define ZAN_NIVEL_GERENTE       3
#define ZAN_NIVEL_MANUTENCAO    4

// Operações de Caixa e Operador
#define ZAN_FUNC_ABERTURA_CAIXA         151 // Abertura de Caixa (Supervisor)
#define ZAN_FUNC_LOGIN_OPERADOR         154 // Entrada do Operador
#define ZAN_FUNC_SAIDA_OPERADOR         155 // Saída do Operador
#define ZAN_FUNC_TROCA_OPERADOR         156 // Troca de Operador
#define ZAN_FUNC_SUPRIMENTO             157 // Fundo de Troco / Suprimento
#define ZAN_FUNC_SANGRIA                158 // Retirada / Sangria de Caixa
#define ZAN_FUNC_FECHAMENTO_CAIXA       159 // Fechamento de Caixa com conferência
#define ZAN_FUNC_TRAVA_OPERACAO         143 // Bloqueio rápido do terminal
#define ZAN_FUNC_BLOQUEIO_SANGRIA_LIM   3758// Bloqueio por sangria pendente atingida

// Cancelamentos e Descontos
#define ZAN_FUNC_CANCELA_ULTIMO_ITEM    3701 // Cancela item imediatamente anterior
#define ZAN_FUNC_CANCELA_ITEM_ORDINAL   3702 // Cancela item pelo número de ordem
#define ZAN_FUNC_CANCELA_ITEM_CODIGO    1146 // Cancela item por código de barras/PLU
#define ZAN_FUNC_CANCELA_CUPOM_ABERTO   3703 // Cancela cupom em andamento
#define ZAN_FUNC_CANCELA_CUPOM_ANTERIOR 147  // Cancela cupom emitido anteriormente

#define ZAN_FUNC_DESCONTO_ITEM_VALOR    3711 // Desconto em R$ no item
#define ZAN_FUNC_DESCONTO_ITEM_PERC     3712 // Desconto em % no item
#define ZAN_FUNC_DESCONTO_SUBTOT_VALOR  3713 // Desconto em R$ no subtotal
#define ZAN_FUNC_DESCONTO_SUBTOT_PERC   3714 // Desconto em % no subtotal
#define ZAN_FUNC_ACRESCIMO_VALOR        1820 // Acréscimo em R$
#define ZAN_FUNC_ACRESCIMO_PERC         1810 // Acréscimo em %

// Gaveta e Periféricos
#define ZAN_FUNC_ABRE_GAVETA            9999 // Abertura manual da gaveta (Supervisor)
#define ZAN_FUNC_ABRE_GAVETA_VENDA      9998 // Abertura da gaveta durante a venda
#define ZAN_FUNC_LEITURA_BALANCA        6701 // Captura peso da balança serial checkout

// Relatórios e Auditoria
#define ZAN_FUNC_LEITURA_X              201  // Leitura X fiscal/gerencial
#define ZAN_FUNC_TOTAL_OPERADOR         202  // Relatório do operador
#define ZAN_FUNC_SALDOS                 212  // Saldos acumulados por finalizadora
#define ZAN_FUNC_FIM_DE_DIA             8159 // Fechamento do dia (Manager)

// Fiscal e Contingência
#define ZAN_FUNC_CONTINGENCIA_ATIVA     3511 // Ativa modo de contingência offline
#define ZAN_FUNC_CONTINGENCIA_DESATIVA  3512 // Desativa modo contingência offline
#define ZAN_FUNC_RETRANSMITIR_PENDENTES 12585// Retransmissão de notas em contingência

// TEF
#define ZAN_FUNC_MENU_TEF               8910 // Menu administrativo TEF
#define ZAN_FUNC_REIMPRIME_TEF          8415 // Reimpressão de comprovante TEF

#endif // ZAN_CONSTANTS_H
