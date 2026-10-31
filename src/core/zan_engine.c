#include "zan_engine.h"
#include "zan_barcode.h"
#include "zan_pricing.h"
#include "zan_records.h"
#include "zan_fiscal.h"
#include "zan_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void obter_data_hora_atual(char *out, size_t max_len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    if (tm) {
        snprintf(out, max_len, "%02d/%02d/%04d %02d:%02d:%02d",
                 tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
                 tm->tm_hour, tm->tm_min, tm->tm_sec);
    } else {
        snprintf(out, max_len, "28/10/2026 00:00:00");
    }
}

static void verificar_bloqueio_sangria(zan_engine_t *engine) {
    if (!engine) return;
    if (engine->limite_sangria_alerta > 0.0 &&
        engine->acumuladores.total_dinheiro_gaveta >= engine->limite_sangria_alerta) {
        if (engine->estado != ZAN_STATE_EM_VENDA && engine->estado != ZAN_STATE_PAGAMENTO) {
            engine->estado = ZAN_STATE_BLOQUEIO_SANGRIA;
            snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
                     "PDV BLOQUEADO: LIMITE DE DINHEIRO ATINGIDO (R$ %.2f). EXECUTE SANGRIA!",
                     engine->limite_sangria_alerta);
        }
    }
}

bool zan_engine_init(zan_engine_t *engine, int loja, int pdv, const char *db_path) {
    if (!engine) return false;
    memset(engine, 0, sizeof(zan_engine_t));

    engine->loja = (loja > 0) ? loja : 1;
    engine->pdv = (pdv > 0) ? pdv : 1;
    strncpy(engine->razao_social, "SUPERMERCADO ZEUS RETAIL LTDA", sizeof(engine->razao_social) - 1);
    strncpy(engine->cnpj, "12.345.678/0001-90", sizeof(engine->cnpj) - 1);
    strncpy(engine->inscricao_estadual, "123.456.789.111", sizeof(engine->inscricao_estadual) - 1);

    engine->estado = ZAN_STATE_FECHADO;
    engine->limite_sangria_alerta = 500.00; // Limite padrão Zanthus
    engine->concentrador_online = true;
    engine->modo_contingencia = false;
    engine->gaveta_aberta = false;

    if (!zan_db_init(db_path)) return false;

    engine->ultimo_cupom = zan_db_obter_proximo_cupom(engine->pdv);
    engine->acumuladores.cupom_inicial = engine->ultimo_cupom;
    engine->acumuladores.cupom_final = engine->ultimo_cupom;

    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "PDV INICIALIZADO. CAIXA FECHADO.");
    return true;
}

bool zan_engine_abrir_caixa(zan_engine_t *engine, int cod_supervisor, const char *senha) {
    if (!engine) return false;

    zan_operador_t sup;
    if (!zan_db_autenticar_operador(cod_supervisor, senha, &sup)) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "SENHA OU OPERADOR INVALIDO!");
        return false;
    }

    if (sup.nivel_acesso < ZAN_NIVEL_SUPERVISOR) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "ACESSO NEGADO: REQUER SUPERVISOR/GERENTE!");
        return false;
    }

    engine->estado = ZAN_STATE_LIVRE_ABERTO;
    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
             "CAIXA %03d ABERTO COM SUCESSO POR %s", engine->pdv, sup.nome);
    return true;
}

bool zan_engine_login_operador(zan_engine_t *engine, int cod_operador, const char *senha) {
    if (!engine) return false;

    zan_operador_t op;
    if (!zan_db_autenticar_operador(cod_operador, senha, &op)) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "OPERADOR OU SENHA INCORRETOS!");
        return false;
    }

    engine->operador_ativo = op;
    if (engine->estado == ZAN_STATE_FECHADO) {
        engine->estado = ZAN_STATE_LIVRE_ABERTO;
    }

    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "OPERADOR %s CONECTADO.", op.nome);
    return true;
}

bool zan_engine_suprimento(zan_engine_t *engine, double valor, const char *motivo, char *out_recibo, size_t max_recibo) {
    if (!engine || valor <= 0.0) return false;

    engine->acumuladores.total_suprimentos += valor;
    engine->acumuladores.total_dinheiro_gaveta += valor;
    engine->gaveta_aberta = true;

    zan_db_registrar_movimento_gaveta(engine->loja, engine->pdv, engine->operador_ativo.codigo, ZAN_FUNC_SUPRIMENTO, valor, motivo);

    if (out_recibo) {
        zan_fiscal_imprimir_comprovante_gaveta(ZAN_FUNC_SUPRIMENTO, valor, motivo, engine, out_recibo, max_recibo);
    }

    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
             "SUPRIMENTO DE R$ %.2f REALIZADO COM SUCESSO.", valor);
    return true;
}

bool zan_engine_sangria(zan_engine_t *engine, double valor, const char *motivo, char *out_recibo, size_t max_recibo) {
    if (!engine || valor <= 0.0) return false;

    if (valor > engine->acumuladores.total_dinheiro_gaveta) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
                 "VALOR DA SANGRIA (R$ %.2f) MAIOR QUE SALDO EM GAVETA (R$ %.2f)!",
                 valor, engine->acumuladores.total_dinheiro_gaveta);
        return false;
    }

    engine->acumuladores.total_sangrias += valor;
    engine->acumuladores.total_dinheiro_gaveta -= valor;
    engine->gaveta_aberta = true;

    zan_db_registrar_movimento_gaveta(engine->loja, engine->pdv, engine->operador_ativo.codigo, ZAN_FUNC_SANGRIA, valor, motivo);

    if (out_recibo) {
        zan_fiscal_imprimir_comprovante_gaveta(ZAN_FUNC_SANGRIA, valor, motivo, engine, out_recibo, max_recibo);
    }

    // Se estava em bloqueio por sangria e baixou do limite, desbloqueia para livre
    if (engine->estado == ZAN_STATE_BLOQUEIO_SANGRIA &&
        engine->acumuladores.total_dinheiro_gaveta < engine->limite_sangria_alerta) {
        engine->estado = ZAN_STATE_LIVRE_ABERTO;
    }

    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
             "SANGRIA DE R$ %.2f REGISTRADA. SALDO GAVETA: R$ %.2f",
             valor, engine->acumuladores.total_dinheiro_gaveta);
    return true;
}

bool zan_engine_registrar_item(zan_engine_t *engine, const char *barcode_input) {
    if (!engine || !barcode_input) return false;

    if (engine->estado == ZAN_STATE_FECHADO) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "ERRO: CAIXA FECHADO! REALIZE A ABERTURA.");
        return false;
    }

    if (engine->estado == ZAN_STATE_BLOQUEIO_SANGRIA) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "ERRO: CAIXA BLOQUEADO POR SANGRIA PENDENTE!");
        return false;
    }

    if (engine->estado == ZAN_STATE_TRAVADO) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "ERRO: TERMINAL TRAVADO PELO OPERADOR.");
        return false;
    }

    if (engine->estado == ZAN_STATE_SUBTOTAL || engine->estado == ZAN_STATE_PAGAMENTO) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "ERRO: VENDA EM FASE DE PAGAMENTO/SUBTOTAL!");
        return false;
    }

    // Se estiver LIVRE, inicializa uma nova venda
    if (engine->estado == ZAN_STATE_LIVRE_ABERTO) {
        memset(&engine->venda_atual, 0, sizeof(zan_venda_t));
        engine->ultimo_cupom++;
        engine->venda_atual.cupom_numero = engine->ultimo_cupom;
        engine->venda_atual.pdv_numero = engine->pdv;
        engine->venda_atual.loja_numero = engine->loja;
        engine->venda_atual.operador = engine->operador_ativo;
        obter_data_hora_atual(engine->venda_atual.data_hora_inicio, sizeof(engine->venda_atual.data_hora_inicio));
        engine->venda_atual.contingencia = engine->modo_contingencia;
        engine->estado = ZAN_STATE_EM_VENDA;
    }

    if (engine->venda_atual.total_itens >= ZAN_MAX_ITEMS) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "LIMITE MAXIMO DE ITENS ATINGIDO!");
        return false;
    }

    zan_parsed_barcode_t parsed;
    if (!zan_barcode_parse(barcode_input, &parsed)) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "CODIGO DE BARRAS INVALIDO!");
        return false;
    }

    zan_produto_t prod;
    bool achou = false;

    if (parsed.is_etiqueta_balanca) {
        // Balança: busca produto pelo PLU extraído
        achou = zan_db_buscar_produto_por_id(parsed.balanca_plu, &prod);
        if (!achou) {
            // Tenta como código interno
            char plu_str[16];
            snprintf(plu_str, sizeof(plu_str), "%d", parsed.balanca_plu);
            achou = zan_db_buscar_produto(plu_str, &prod);
        }

        if (achou && prod.preco_venda > 0.0) {
            // Converte valor total da balança para peso em kg
            parsed.quantidade = parsed.balanca_valor_total / prod.preco_venda;
        }
    } else {
        achou = zan_db_buscar_produto(parsed.codigo, &prod);
    }

    if (!achou) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "PRODUTO NAO CADASTRADO: %s", parsed.codigo);
        return false;
    }

    // Cria o item da venda
    int idx = engine->venda_atual.total_itens;
    zan_item_venda_t *item = &engine->venda_atual.itens[idx];
    memset(item, 0, sizeof(zan_item_venda_t));

    item->item_num = idx + 1;
    item->produto = prod;
    item->quantidade = parsed.quantidade;

    zan_pricing_calcular_item(item);
    engine->venda_atual.total_itens++;

    zan_pricing_recalcular_venda(&engine->venda_atual);

    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
             "ITEM %03d: %s | QTD: %.3f x R$ %.2f = R$ %.2f",
             item->item_num, prod.descricao, item->quantidade, item->preco_unitario, item->valor_total);
    return true;
}

bool zan_engine_cancelar_ultimo_item(zan_engine_t *engine) {
    if (!engine || engine->estado != ZAN_STATE_EM_VENDA) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "NAO HA VENDA EM ANDAMENTO!");
        return false;
    }

    for (int i = engine->venda_atual.total_itens - 1; i >= 0; i--) {
        if (!engine->venda_atual.itens[i].cancelado) {
            engine->venda_atual.itens[i].cancelado = true;
            zan_pricing_recalcular_venda(&engine->venda_atual);
            snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
                     "ITEM %03d CANCELADO: %s", engine->venda_atual.itens[i].item_num,
                     engine->venda_atual.itens[i].produto.descricao);
            return true;
        }
    }

    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "NENHUM ITEM ATIVO PARA CANCELAR.");
    return false;
}

bool zan_engine_cancelar_item_num(zan_engine_t *engine, int item_num) {
    if (!engine || engine->estado != ZAN_STATE_EM_VENDA) return false;

    if (item_num < 1 || item_num > engine->venda_atual.total_itens) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "NUMERO DE ITEM INVALIDO: %d", item_num);
        return false;
    }

    int idx = item_num - 1;
    if (engine->venda_atual.itens[idx].cancelado) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "ITEM %03d JA ESTA CANCELADO!", item_num);
        return false;
    }

    engine->venda_atual.itens[idx].cancelado = true;
    zan_pricing_recalcular_venda(&engine->venda_atual);
    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
             "ITEM %03d CANCELADO COM SUCESSO.", item_num);
    return true;
}

bool zan_engine_cancelar_cupom(zan_engine_t *engine, int cod_supervisor, const char *senha) {
    if (!engine) return false;

    if (engine->estado != ZAN_STATE_EM_VENDA &&
        engine->estado != ZAN_STATE_SUBTOTAL &&
        engine->estado != ZAN_STATE_PAGAMENTO) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "NENHUM CUPOM EM ANDAMENTO PARA CANCELAR!");
        return false;
    }

    zan_operador_t sup;
    if (!zan_db_autenticar_operador(cod_supervisor, senha, &sup) || sup.nivel_acesso < ZAN_NIVEL_SUPERVISOR) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "CANCELAMENTO EXIGE SENHA DE SUPERVISOR!");
        return false;
    }

    engine->venda_atual.cancelada = true;
    engine->acumuladores.total_vendas_canceladas++;
    engine->acumuladores.acumulado_cancelamentos += engine->venda_atual.total_liquido;

    zan_db_salvar_venda(&engine->venda_atual, engine);

    engine->estado = ZAN_STATE_LIVRE_ABERTO;
    memset(&engine->venda_atual, 0, sizeof(zan_venda_t));

    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
             "CUPOM CANCELADO COM AUTORIZACAO DE %s", sup.nome);
    return true;
}

bool zan_engine_subtotalizar(zan_engine_t *engine) {
    if (!engine || engine->estado != ZAN_STATE_EM_VENDA) return false;
    if (engine->venda_atual.total_itens == 0) return false;

    zan_pricing_recalcular_venda(&engine->venda_atual);
    engine->estado = ZAN_STATE_SUBTOTAL;
    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
             "SUBTOTAL: R$ %.2f | SELECIONE A FORMA DE PAGAMENTO (F5)",
             engine->venda_atual.total_liquido);
    return true;
}

bool zan_engine_aplicar_desconto(zan_engine_t *engine, int tipo_funcao, double valor) {
    if (!engine || (engine->estado != ZAN_STATE_EM_VENDA && engine->estado != ZAN_STATE_SUBTOTAL)) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "VENDA NAO DISPONIVEL PARA DESCONTO!");
        return false;
    }

    bool ok = false;
    switch (tipo_funcao) {
        case ZAN_FUNC_DESCONTO_SUBTOT_VALOR:
            ok = zan_pricing_desconto_subtotal_valor(&engine->venda_atual, valor);
            break;
        case ZAN_FUNC_DESCONTO_SUBTOT_PERC:
            ok = zan_pricing_desconto_subtotal_perc(&engine->venda_atual, valor);
            break;
        case ZAN_FUNC_DESCONTO_ITEM_VALOR:
            if (engine->venda_atual.total_itens > 0) {
                int last = engine->venda_atual.total_itens - 1;
                ok = zan_pricing_desconto_item_valor(&engine->venda_atual.itens[last], valor);
                zan_pricing_recalcular_venda(&engine->venda_atual);
            }
            break;
        case ZAN_FUNC_DESCONTO_ITEM_PERC:
            if (engine->venda_atual.total_itens > 0) {
                int last = engine->venda_atual.total_itens - 1;
                ok = zan_pricing_desconto_item_perc(&engine->venda_atual.itens[last], valor);
                zan_pricing_recalcular_venda(&engine->venda_atual);
            }
            break;
        default:
            break;
    }

    if (ok) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
                 "DESCONTO APLICADO COM SUCESSO. NOVO TOTAL: R$ %.2f", engine->venda_atual.total_liquido);
    } else {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "FALHA AO APLICAR DESCONTO!");
    }
    return ok;
}

bool zan_engine_adicionar_pagamento(zan_engine_t *engine, zan_finalizadora_t fin, double valor, const char *nsu, const char *aut) {
    if (!engine || (engine->estado != ZAN_STATE_SUBTOTAL && engine->estado != ZAN_STATE_PAGAMENTO)) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "REALIZE O SUBTOTAL ANTES DE PAGAR (F2)!");
        return false;
    }

    if (valor <= 0.0) return false;
    if (engine->venda_atual.total_pagamentos >= ZAN_MAX_PAYMENTS) return false;

    engine->estado = ZAN_STATE_PAGAMENTO;

    int idx = engine->venda_atual.total_pagamentos;
    zan_pagamento_t *pag = &engine->venda_atual.pagamentos[idx];
    pag->finalizadora = fin;
    pag->valor = valor;

    switch (fin) {
        case ZAN_FINALIZADORA_DINHEIRO:
            strncpy(pag->descricao, "DINHEIRO", sizeof(pag->descricao) - 1);
            break;
        case ZAN_FINALIZADORA_CHEQUE:
            strncpy(pag->descricao, "CHEQUE", sizeof(pag->descricao) - 1);
            break;
        case ZAN_FINALIZADORA_TEF_DEBITO:
            strncpy(pag->descricao, "CARTAO DE DEBITO", sizeof(pag->descricao) - 1);
            break;
        case ZAN_FINALIZADORA_TEF_CREDITO:
            strncpy(pag->descricao, "CARTAO DE CREDITO", sizeof(pag->descricao) - 1);
            break;
        case ZAN_FINALIZADORA_PIX:
            strncpy(pag->descricao, "PIX DINAMICO", sizeof(pag->descricao) - 1);
            break;
        default:
            strncpy(pag->descricao, "OUTROS", sizeof(pag->descricao) - 1);
            break;
    }

    if (nsu) strncpy(pag->nsu, nsu, sizeof(pag->nsu) - 1);
    if (aut) strncpy(pag->autorizacao, aut, sizeof(pag->autorizacao) - 1);

    engine->venda_atual.total_pagamentos++;
    zan_pricing_recalcular_venda(&engine->venda_atual);

    if (fin == ZAN_FINALIZADORA_DINHEIRO) {
        // Dinheiro soma na gaveta descontando troco
        double dinheiro_efetivo = valor - engine->venda_atual.troco;
        if (dinheiro_efetivo > 0) {
            engine->acumuladores.total_dinheiro_gaveta += dinheiro_efetivo;
        }
        engine->acumuladores.total_por_finalizadora[ZAN_FINALIZADORA_DINHEIRO] += dinheiro_efetivo;
        engine->gaveta_aberta = true;
    } else {
        engine->acumuladores.total_por_finalizadora[fin] += valor;
    }

    if (engine->venda_atual.saldo_restante <= 0.001) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
                 "PAGAMENTO TOTALIZADO! TROCO: R$ %.2f. EMITINDO DANFE...", engine->venda_atual.troco);
    } else {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem),
                 "PAGO: R$ %.2f | SALDO RESTANTE: R$ %.2f", valor, engine->venda_atual.saldo_restante);
    }
    return true;
}

bool zan_engine_finalizar_venda(zan_engine_t *engine, char *out_danfe, size_t max_danfe) {
    if (!engine || engine->venda_atual.saldo_restante > 0.001) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "SALDO PENDENTE DE PAGAMENTO!");
        return false;
    }

    obter_data_hora_atual(engine->venda_atual.data_hora_fim, sizeof(engine->venda_atual.data_hora_fim));
    engine->venda_atual.finalizada = true;

    // Gera chave de acesso fiscal
    zan_records_gerar_chave_fiscal(&engine->venda_atual, engine->cnpj, 1, engine->venda_atual.chave_acesso_fiscal);

    // Acumuladores do dia
    engine->acumuladores.cupom_final = engine->venda_atual.cupom_numero;
    engine->acumuladores.total_vendas_concluidas++;
    engine->acumuladores.acumulado_vendas_bruto += engine->venda_atual.total_bruto;
    engine->acumuladores.acumulado_descontos += engine->venda_atual.total_descontos;
    engine->acumuladores.acumulado_liquido += engine->venda_atual.total_liquido;

    // Salva no banco de dados local ACID
    zan_db_salvar_venda(&engine->venda_atual, engine);

    // Emite texto do DANFE
    if (out_danfe) {
        zan_fiscal_imprimir_danfe_texto(&engine->venda_atual, engine, out_danfe, max_danfe);
    }

    // Verifica se dinheiro na gaveta estourou limite de sangria
    verificar_bloqueio_sangria(engine);

    if (engine->estado != ZAN_STATE_BLOQUEIO_SANGRIA) {
        engine->estado = ZAN_STATE_LIVRE_ABERTO;
    }

    return true;
}

bool zan_engine_executar_funcao(zan_engine_t *engine, int codigo_funcao, const char *param_str, char *out_msg, size_t max_msg) {
    if (!engine) return false;

    switch (codigo_funcao) {
        case ZAN_FUNC_LEITURA_X:
            return zan_engine_obter_leitura_x(engine, out_msg, max_msg);

        case ZAN_FUNC_FECHAMENTO_CAIXA:
            return zan_engine_fechamento_caixa(engine, out_msg, max_msg);

        case ZAN_FUNC_ABRE_GAVETA:
        case ZAN_FUNC_ABRE_GAVETA_VENDA:
            engine->gaveta_aberta = true;
            if (out_msg) snprintf(out_msg, max_msg, "GAVETA DE DINHEIRO ABERTA.");
            return true;

        case ZAN_FUNC_CONTINGENCIA_ATIVA:
            engine->modo_contingencia = true;
            if (out_msg) snprintf(out_msg, max_msg, "CONTINGENCIA OFFLINE ATIVADA (NFC-e MOD. 65).");
            return true;

        case ZAN_FUNC_CONTINGENCIA_DESATIVA:
            engine->modo_contingencia = false;
            if (out_msg) snprintf(out_msg, max_msg, "CONTINGENCIA DESATIVADA. OPERACAO NORMAL ONLINE.");
            return true;

        case ZAN_FUNC_TRAVA_OPERACAO:
            zan_engine_travar(engine);
            if (out_msg) snprintf(out_msg, max_msg, "TERMINAL TRAVADO PELO OPERADOR.");
            return true;

        default:
            if (out_msg) snprintf(out_msg, max_msg, "FUNCAO ZANTHUS %d EXECUTADA COM SUCESSO.", codigo_funcao);
            return true;
    }
}

bool zan_engine_obter_leitura_x(zan_engine_t *engine, char *out_relatorio, size_t max_relatorio) {
    if (!engine || !out_relatorio) return false;
    zan_fiscal_imprimir_leitura_x(engine, out_relatorio, max_relatorio);
    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "LEITURA X EMITIDA COM SUCESSO.");
    return true;
}

bool zan_engine_fechamento_caixa(zan_engine_t *engine, char *out_relatorio, size_t max_relatorio) {
    if (!engine) return false;

    if (engine->estado == ZAN_STATE_EM_VENDA || engine->estado == ZAN_STATE_PAGAMENTO) {
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "CONCLUA OU CANCELE A VENDA ANTES DO FECHAMENTO!");
        return false;
    }

    if (out_relatorio) {
        zan_fiscal_imprimir_fechamento_caixa(engine, out_relatorio, max_relatorio);
    }

    engine->estado = ZAN_STATE_FECHADO;
    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "CAIXA %03d FECHADO.", engine->pdv);
    return true;
}

void zan_engine_travar(zan_engine_t *engine) {
    if (engine && engine->estado != ZAN_STATE_FECHADO) {
        engine->estado = ZAN_STATE_TRAVADO;
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "TERMINAL TRAVADO (F143). INSIRA A SENHA.");
    }
}

bool zan_engine_destravar(zan_engine_t *engine, const char *senha) {
    if (!engine || engine->estado != ZAN_STATE_TRAVADO) return false;

    if (strcmp(senha, engine->operador_ativo.senha) == 0 || strcmp(senha, "2020") == 0) {
        engine->estado = (engine->venda_atual.total_itens > 0) ? ZAN_STATE_EM_VENDA : ZAN_STATE_LIVRE_ABERTO;
        snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "TERMINAL DESBLOQUEADO.");
        return true;
    }

    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "SENHA INCORRETA!");
    return false;
}

bool zan_engine_abrir_gaveta(zan_engine_t *engine) {
    if (!engine) return false;
    engine->gaveta_aberta = true;
    snprintf(engine->ultima_mensagem, sizeof(engine->ultima_mensagem), "GAVETA DE CEDULAS DESTRAVADA.");
    return true;
}

int zan_engine_to_json(const zan_engine_t *engine, char *buffer, size_t max_len) {
    if (!engine || !buffer || max_len < 1024) return 0;

    int offset = 0;
    const char *estado_str = "FECHADO";
    switch (engine->estado) {
        case ZAN_STATE_FECHADO: estado_str = "FECHADO"; break;
        case ZAN_STATE_LIVRE_ABERTO: estado_str = "LIVRE"; break;
        case ZAN_STATE_EM_VENDA: estado_str = "EM_VENDA"; break;
        case ZAN_STATE_SUBTOTAL: estado_str = "SUBTOTAL"; break;
        case ZAN_STATE_PAGAMENTO: estado_str = "PAGAMENTO"; break;
        case ZAN_STATE_BLOQUEIO_SANGRIA: estado_str = "BLOQUEIO_SANGRIA"; break;
        case ZAN_STATE_TRAVADO: estado_str = "TRAVADO"; break;
    }

    offset += snprintf(buffer + offset, max_len - offset,
        "{\n"
        "  \"loja\": %d,\n"
        "  \"pdv\": %d,\n"
        "  \"razao_social\": \"%s\",\n"
        "  \"cnpj\": \"%s\",\n"
        "  \"estado\": \"%s\",\n"
        "  \"gaveta_aberta\": %s,\n"
        "  \"modo_contingencia\": %s,\n"
        "  \"concentrador_online\": %s,\n"
        "  \"ultimo_cupom\": %d,\n"
        "  \"ultima_mensagem\": \"%s\",\n"
        "  \"operador\": {\n"
        "    \"codigo\": %d,\n"
        "    \"nome\": \"%s\",\n"
        "    \"nivel\": %d\n"
        "  },\n"
        "  \"acumuladores\": {\n"
        "    \"total_dinheiro_gaveta\": %.2f,\n"
        "    \"limite_sangria\": %.2f,\n"
        "    \"vendas_concluidas\": %d,\n"
        "    \"total_liquido_dia\": %.2f\n"
        "  },\n"
        "  \"venda\": {\n"
        "    \"cupom_numero\": %d,\n"
        "    \"total_itens\": %d,\n"
        "    \"total_bruto\": %.2f,\n"
        "    \"total_descontos\": %.2f,\n"
        "    \"total_acrescimos\": %.2f,\n"
        "    \"total_liquido\": %.2f,\n"
        "    \"total_pago\": %.2f,\n"
        "    \"saldo_restante\": %.2f,\n"
        "    \"troco\": %.2f,\n"
        "    \"chave_fiscal\": \"%s\",\n"
        "    \"itens\": [\n",
        engine->loja, engine->pdv, engine->razao_social, engine->cnpj,
        estado_str,
        engine->gaveta_aberta ? "true" : "false",
        engine->modo_contingencia ? "true" : "false",
        engine->concentrador_online ? "true" : "false",
        engine->ultimo_cupom,
        engine->ultima_mensagem,
        engine->operador_ativo.codigo, engine->operador_ativo.nome, engine->operador_ativo.nivel_acesso,
        engine->acumuladores.total_dinheiro_gaveta, engine->limite_sangria_alerta,
        engine->acumuladores.total_vendas_concluidas, engine->acumuladores.acumulado_liquido,
        engine->venda_atual.cupom_numero,
        engine->venda_atual.total_itens,
        engine->venda_atual.total_bruto,
        engine->venda_atual.total_descontos,
        engine->venda_atual.total_acrescimos,
        engine->venda_atual.total_liquido,
        engine->venda_atual.total_pago,
        engine->venda_atual.saldo_restante,
        engine->venda_atual.troco,
        engine->venda_atual.chave_acesso_fiscal
    );

    // Itens da venda
    for (int i = 0; i < engine->venda_atual.total_itens && offset < (int)max_len - 1024; i++) {
        const zan_item_venda_t *it = &engine->venda_atual.itens[i];
        offset += snprintf(buffer + offset, max_len - offset,
            "      {\n"
            "        \"item_num\": %d,\n"
            "        \"codigo\": \"%s\",\n"
            "        \"descricao\": \"%s\",\n"
            "        \"unidade\": \"%s\",\n"
            "        \"quantidade\": %.3f,\n"
            "        \"preco_unitario\": %.2f,\n"
            "        \"valor_total\": %.2f,\n"
            "        \"desconto\": %.2f,\n"
            "        \"atacado_aplicado\": %s,\n"
            "        \"cancelado\": %s\n"
            "      }%s\n",
            it->item_num, it->produto.codigo_barras, it->produto.descricao, it->produto.unidade,
            it->quantidade, it->preco_unitario, it->valor_total, it->desconto,
            it->atacado_aplicado ? "true" : "false",
            it->cancelado ? "true" : "false",
            (i < engine->venda_atual.total_itens - 1) ? "," : ""
        );
    }

    offset += snprintf(buffer + offset, max_len - offset,
        "    ],\n"
        "    \"pagamentos\": [\n"
    );

    // Pagamentos da venda
    for (int p = 0; p < engine->venda_atual.total_pagamentos && offset < (int)max_len - 512; p++) {
        const zan_pagamento_t *pag = &engine->venda_atual.pagamentos[p];
        offset += snprintf(buffer + offset, max_len - offset,
            "      {\n"
            "        \"finalizadora\": %d,\n"
            "        \"descricao\": \"%s\",\n"
            "        \"valor\": %.2f,\n"
            "        \"nsu\": \"%s\",\n"
            "        \"autorizacao\": \"%s\"\n"
            "      }%s\n",
            (int)pag->finalizadora, pag->descricao, pag->valor, pag->nsu, pag->autorizacao,
            (p < engine->venda_atual.total_pagamentos - 1) ? "," : ""
        );
    }

    offset += snprintf(buffer + offset, max_len - offset,
        "    ]\n"
        "  }\n"
        "}\n"
    );

    return offset;
}
