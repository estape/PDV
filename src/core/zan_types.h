#ifndef ZAN_TYPES_H
#define ZAN_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "zan_constants.h"

#define ZAN_MAX_DESC 64
#define ZAN_MAX_BARCODE 32
#define ZAN_MAX_ITEMS 500
#define ZAN_MAX_PAYMENTS 20

// Estados operacionais do terminal PDV
typedef enum {
    ZAN_STATE_FECHADO = 0,         // Caixa fechado (precisa de abertura)
    ZAN_STATE_LIVRE_ABERTO = 1,    // Caixa aberto e livre (pronto para nova venda)
    ZAN_STATE_EM_VENDA = 2,        // Registrando itens da venda
    ZAN_STATE_SUBTOTAL = 3,        // Subtotalizado, aguardando início de pagamento
    ZAN_STATE_PAGAMENTO = 4,       // Recebendo finalizadoras
    ZAN_STATE_BLOQUEIO_SANGRIA = 5,// Trava por excesso de dinheiro no caixa
    ZAN_STATE_TRAVADO = 6          // Terminal travado momentaneamente pelo operador
} zan_pos_state_t;

// Códigos das finalizadoras de pagamento Zanthus (TH09)
typedef enum {
    ZAN_FINALIZADORA_DINHEIRO = 1,
    ZAN_FINALIZADORA_CHEQUE = 2,
    ZAN_FINALIZADORA_TEF_DEBITO = 3,
    ZAN_FINALIZADORA_TEF_CREDITO = 4,
    ZAN_FINALIZADORA_CONVENIO = 5,
    ZAN_FINALIZADORA_VALE = 6,
    ZAN_FINALIZADORA_PIX = 7
} zan_finalizadora_t;

// Informações de Operador (TH16 / TH17)
typedef struct {
    int codigo;
    char nome[64];
    char senha[32];
    int nivel_acesso; // 1: Operador, 2: Supervisor, 3: Gerente, 4: Manutencao
    bool ativo;
} zan_operador_t;

// Cadastro de Produto (TH06 / TH24 / TH25)
typedef struct {
    int id;
    char codigo_interno[16];
    char codigo_barras[ZAN_MAX_BARCODE];
    char descricao[ZAN_MAX_DESC];
    char unidade[4]; // UN, KG, LT
    double preco_venda;
    double aliquota_icms;
    char ncm[12];
    bool permite_fracionado;
    // Atacarejo (TH24)
    int atacado_qtd_minima;
    double atacado_preco;
} zan_produto_t;

// Item registrado na venda em andamento
typedef struct {
    int item_num;
    zan_produto_t produto;
    double quantidade;
    double preco_unitario;
    double preco_tabela;
    double valor_total;
    double desconto;
    double acrescimo;
    bool atacado_aplicado;
    bool cancelado;
} zan_item_venda_t;

// Pagamento registrado na venda
typedef struct {
    zan_finalizadora_t finalizadora;
    char descricao[32];
    double valor;
    double troco;
    char nsu[32];
    char autorizacao[32];
    char bandeira[32];
} zan_pagamento_t;

// Estrutura de Venda em Andamento
typedef struct {
    int cupom_numero;
    int pdv_numero;
    int loja_numero;
    char data_hora_inicio[32];
    char data_hora_fim[32];
    zan_operador_t operador;
    zan_item_venda_t itens[ZAN_MAX_ITEMS];
    int total_itens;
    zan_pagamento_t pagamentos[ZAN_MAX_PAYMENTS];
    int total_pagamentos;
    double total_bruto;
    double total_descontos;
    double total_acrescimos;
    double total_liquido;
    double total_pago;
    double saldo_restante;
    double troco;
    char cpf_cnpj_cliente[20];
    char chave_acesso_fiscal[48];
    bool contingencia;
    bool finalizada;
    bool cancelada;
} zan_venda_t;

// Acumuladores e Totalizadores do Caixa (para Leitura X e Fechamento)
typedef struct {
    int cupom_inicial;
    int cupom_final;
    int total_vendas_concluidas;
    int total_vendas_canceladas;
    double acumulado_vendas_bruto;
    double acumulado_descontos;
    double acumulado_cancelamentos;
    double acumulado_liquido;
    double total_dinheiro_gaveta;
    double total_sangrias;
    double total_suprimentos;
    double total_por_finalizadora[10]; // indexado por zan_finalizadora_t
} zan_acumuladores_caixa_t;

// Contexto Global do PDV (Zanthus Engine)
typedef struct {
    int loja;
    int pdv;
    char razao_social[64];
    char cnpj[20];
    char inscricao_estadual[20];
    zan_pos_state_t estado;
    zan_operador_t operador_ativo;
    zan_venda_t venda_atual;
    zan_acumuladores_caixa_t acumuladores;
    double limite_sangria_alerta; // ex: R$ 500.00
    bool gaveta_aberta;
    bool modo_contingencia;
    bool concentrador_online;
    int ultimo_cupom;
    char ultima_mensagem[128];
} zan_engine_t;

#endif // ZAN_TYPES_H
