#ifndef ZAN_ENGINE_H
#define ZAN_ENGINE_H

#include "zan_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa a máquina de estados e o contexto operacional do PDV
bool zan_engine_init(zan_engine_t *engine, int loja, int pdv, const char *db_path);

// Executa abertura de caixa (Função 151) com autorização de supervisor/gerente
bool zan_engine_abrir_caixa(zan_engine_t *engine, int cod_supervisor, const char *senha);

// Efetua login/entrada do operador no caixa (Função 154)
bool zan_engine_login_operador(zan_engine_t *engine, int cod_operador, const char *senha);

// Registra entrada de fundo de troco / suprimento (Função 157)
bool zan_engine_suprimento(zan_engine_t *engine, double valor, const char *motivo, char *out_recibo, size_t max_recibo);

// Efetua retirada de valores / sangria (Função 158)
bool zan_engine_sangria(zan_engine_t *engine, double valor, const char *motivo, char *out_recibo, size_t max_recibo);

// Processa a leitura de um código de barras ou entrada digitada do operador
// Suporta: "789...", "3*789...", "200050004500" (balança)
bool zan_engine_registrar_item(zan_engine_t *engine, const char *barcode_input);

// Cancela o último item registrado (Função 3701)
bool zan_engine_cancelar_ultimo_item(zan_engine_t *engine);

// Cancela item pelo seu número ordinal (Função 3702)
bool zan_engine_cancelar_item_num(zan_engine_t *engine, int item_num);

// Cancela o cupom aberto em andamento (Função 3703)
bool zan_engine_cancelar_cupom(zan_engine_t *engine, int cod_supervisor, const char *senha);

// Subtotaliza a venda corrente
bool zan_engine_subtotalizar(zan_engine_t *engine);

// Aplica desconto (Funções 3711 a 3714)
bool zan_engine_aplicar_desconto(zan_engine_t *engine, int tipo_funcao, double valor);

// Registra uma parcela/pagamento da venda (Função 3740 e finalizadoras)
bool zan_engine_adicionar_pagamento(zan_engine_t *engine, zan_finalizadora_t fin, double valor, const char *nsu, const char *aut);

// Finaliza a venda e emite o documento fiscal
bool zan_engine_finalizar_venda(zan_engine_t *engine, char *out_danfe, size_t max_danfe);

// Executa uma função genérica do Zanthus pelo seu código oficial (ex: 201, 158, 9999, etc.)
bool zan_engine_executar_funcao(zan_engine_t *engine, int codigo_funcao, const char *param_str, char *out_msg, size_t max_msg);

// Gera Leitura X (Função 201)
bool zan_engine_obter_leitura_x(zan_engine_t *engine, char *out_relatorio, size_t max_relatorio);

// Executa fechamento do caixa (Função 159)
bool zan_engine_fechamento_caixa(zan_engine_t *engine, char *out_relatorio, size_t max_relatorio);

// Bloqueio e desbloqueio temporário (Função 143)
void zan_engine_travar(zan_engine_t *engine);
bool zan_engine_destravar(zan_engine_t *engine, const char *senha);

// Abertura manual da gaveta de dinheiro (Função 9999)
bool zan_engine_abrir_gaveta(zan_engine_t *engine);

// Serializa o estado atual do PDV em JSON para atualização do Frontend
int zan_engine_to_json(const zan_engine_t *engine, char *buffer, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif // ZAN_ENGINE_H
