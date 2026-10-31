#include "zan_db.h"
#include "../../third_party/sqlite3/sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sqlite3 *g_db = NULL;

static void seed_dados_iniciais(void) {
    if (!g_db) return;

    // Tabela de Operadores (Zanthus TH16 / TH17)
    const char *sql_ops =
        "INSERT OR IGNORE INTO operadores (codigo, nome, senha, nivel_acesso, ativo) VALUES "
        "(101, 'MARIA SILVA (OPERADORA)', '1234', 1, 1),"
        "(102, 'JOAO SANTOS (OPERADOR)', '1234', 1, 1),"
        "(201, 'CARLOS SOUZA (SUPERVISOR)', '2020', 2, 1),"
        "(301, 'ANA LIMA (GERENTE)', '3030', 3, 1),"
        "(401, 'SUPORTE ZANTHUS', '9999', 4, 1);";
    sqlite3_exec(g_db, sql_ops, NULL, NULL, NULL);

    // Tabela de Produtos (Zanthus TH06 / TH24 / TH25)
    const char *sql_prods =
        "INSERT OR IGNORE INTO produtos (codigo_interno, codigo_barras, descricao, unidade, preco_venda, aliquota_icms, ncm, permite_fracionado, atacado_qtd_minima, atacado_preco) VALUES "
        "('101', '7891000100103', 'ARROZ TIPO 1 BRANCO 5KG', 'UN', 29.90, 7.0, '10063021', 0, 5, 26.50),"
        "('102', '7891000200204', 'FEIJAO CARIOCA 1KG', 'UN', 7.80, 7.0, '07133399', 0, 10, 6.99),"
        "('103', '7891000300305', 'OLEO DE SOJA REFINADO 900ML', 'UN', 6.45, 12.0, '15079011', 0, 12, 5.80),"
        "('104', '7891000400406', 'ACUCAR REFINADO 1KG', 'UN', 4.29, 7.0, '17019900', 0, 10, 3.89),"
        "('105', '7891000500507', 'CAFE TORRADO E MOIDO 500G', 'UN', 18.90, 18.0, '09012100', 0, 6, 16.90),"
        "('106', '7891000600608', 'LEITE INTEGRAL UHT 1L', 'UN', 4.99, 12.0, '04012010', 0, 12, 4.39),"
        "('107', '7891000700709', 'REFRIGERANTE COLA 2L', 'UN', 9.50, 18.0, '22021000', 0, 6, 8.20),"
        "('108', '7891000800800', 'CERVEJA PURO MALTE LATA 350ML', 'UN', 4.19, 25.0, '22030000', 0, 12, 3.69),"
        "('109', '7891000900901', 'SABAO EM PO CONCENTRADO 1.6KG', 'UN', 22.90, 18.0, '34022000', 0, 3, 19.90),"
        "('110', '7891000011002', 'DETERGENTE LIQUIDO 500ML', 'UN', 2.39, 18.0, '34022000', 0, 6, 1.99),"
        "('50',  '200050000000', 'QUEIJO PRATO KG (BALANCA)', 'KG', 45.00, 12.0, '04069020', 1, 0, 0.00),"
        "('51',  '200051000000', 'BANANA NANICA KG (BALANCA)', 'KG', 6.99, 0.0, '08039000', 1, 0, 0.00),"
        "('52',  '200052000000', 'ALCATRA BOVINA KG (BALANCA)', 'KG', 39.90, 7.0, '02013000', 1, 0, 0.00);";
    sqlite3_exec(g_db, sql_prods, NULL, NULL, NULL);
}

bool zan_db_init(const char *db_path) {
    if (g_db) return true;

    int rc = sqlite3_open(db_path ? db_path : "pdv_local.db", &g_db);
    if (rc != SQLITE_OK) {
        if (g_db) sqlite3_close(g_db);
        g_db = NULL;
        return false;
    }

    // Configurações para alto desempenho em disco de PDV
    sqlite3_exec(g_db, "PRAGMA journal_mode = WAL;", NULL, NULL, NULL);
    sqlite3_exec(g_db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);

    const char *schema =
        "CREATE TABLE IF NOT EXISTS operadores ("
        "  codigo INTEGER PRIMARY KEY,"
        "  nome TEXT NOT NULL,"
        "  senha TEXT NOT NULL,"
        "  nivel_acesso INTEGER DEFAULT 1,"
        "  ativo INTEGER DEFAULT 1"
        ");"
        "CREATE TABLE IF NOT EXISTS produtos ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  codigo_interno TEXT UNIQUE,"
        "  codigo_barras TEXT UNIQUE,"
        "  descricao TEXT NOT NULL,"
        "  unidade TEXT DEFAULT 'UN',"
        "  preco_venda REAL NOT NULL,"
        "  aliquota_icms REAL DEFAULT 0.0,"
        "  ncm TEXT DEFAULT '00000000',"
        "  permite_fracionado INTEGER DEFAULT 0,"
        "  atacado_qtd_minima INTEGER DEFAULT 0,"
        "  atacado_preco REAL DEFAULT 0.0"
        ");"
        "CREATE TABLE IF NOT EXISTS vendas ("
        "  cupom_numero INTEGER PRIMARY KEY,"
        "  pdv INTEGER NOT NULL,"
        "  loja INTEGER NOT NULL,"
        "  operador_cod INTEGER NOT NULL,"
        "  data_hora TEXT NOT NULL,"
        "  total_bruto REAL NOT NULL,"
        "  total_descontos REAL DEFAULT 0.0,"
        "  total_acrescimos REAL DEFAULT 0.0,"
        "  total_liquido REAL NOT NULL,"
        "  cpf_cnpj TEXT,"
        "  chave_fiscal TEXT,"
        "  contingencia INTEGER DEFAULT 0,"
        "  cancelada INTEGER DEFAULT 0,"
        "  sincronizado INTEGER DEFAULT 0"
        ");"
        "CREATE TABLE IF NOT EXISTS movimentos_gaveta ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  loja INTEGER,"
        "  pdv INTEGER,"
        "  operador_cod INTEGER,"
        "  tipo_funcao INTEGER,"
        "  valor REAL,"
        "  motivo TEXT,"
        "  data_hora DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    rc = sqlite3_exec(g_db, schema, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(g_db);
        g_db = NULL;
        return false;
    }

    seed_dados_iniciais();
    return true;
}

void zan_db_close(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

bool zan_db_buscar_produto(const char *codigo, zan_produto_t *out_prod) {
    if (!g_db || !codigo || !out_prod) return false;

    const char *sql = "SELECT id, codigo_interno, codigo_barras, descricao, unidade, preco_venda, "
                      "aliquota_icms, ncm, permite_fracionado, atacado_qtd_minima, atacado_preco "
                      "FROM produtos WHERE codigo_barras = ? OR codigo_interno = ? LIMIT 1;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, codigo, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, codigo, -1, SQLITE_STATIC);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        memset(out_prod, 0, sizeof(zan_produto_t));
        out_prod->id = sqlite3_column_int(stmt, 0);
        strncpy(out_prod->codigo_interno, (const char *)sqlite3_column_text(stmt, 1), sizeof(out_prod->codigo_interno) - 1);
        strncpy(out_prod->codigo_barras, (const char *)sqlite3_column_text(stmt, 2), sizeof(out_prod->codigo_barras) - 1);
        strncpy(out_prod->descricao, (const char *)sqlite3_column_text(stmt, 3), sizeof(out_prod->descricao) - 1);
        strncpy(out_prod->unidade, (const char *)sqlite3_column_text(stmt, 4), sizeof(out_prod->unidade) - 1);
        out_prod->preco_venda = sqlite3_column_double(stmt, 5);
        out_prod->aliquota_icms = sqlite3_column_double(stmt, 6);
        strncpy(out_prod->ncm, (const char *)sqlite3_column_text(stmt, 7), sizeof(out_prod->ncm) - 1);
        out_prod->permite_fracionado = sqlite3_column_int(stmt, 8) != 0;
        out_prod->atacado_qtd_minima = sqlite3_column_int(stmt, 9);
        out_prod->atacado_preco = sqlite3_column_double(stmt, 10);
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool zan_db_buscar_produto_por_id(int id, zan_produto_t *out_prod) {
    if (!g_db || !out_prod) return false;

    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%d", id);
    return zan_db_buscar_produto(id_str, out_prod);
}

bool zan_db_autenticar_operador(int codigo, const char *senha, zan_operador_t *out_op) {
    if (!g_db || !out_op) return false;

    const char *sql = "SELECT codigo, nome, nivel_acesso, ativo FROM operadores WHERE codigo = ? AND senha = ? LIMIT 1;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, codigo);
    sqlite3_bind_text(stmt, 2, senha ? senha : "", -1, SQLITE_STATIC);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        memset(out_op, 0, sizeof(zan_operador_t));
        out_op->codigo = sqlite3_column_int(stmt, 0);
        strncpy(out_op->nome, (const char *)sqlite3_column_text(stmt, 1), sizeof(out_op->nome) - 1);
        out_op->nivel_acesso = sqlite3_column_int(stmt, 2);
        out_op->ativo = sqlite3_column_int(stmt, 3) != 0;
        ok = out_op->ativo;
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool zan_db_salvar_venda(const zan_venda_t *venda, const zan_engine_t *engine) {
    if (!g_db || !venda) return false;

    const char *sql = "INSERT INTO vendas (cupom_numero, pdv, loja, operador_cod, data_hora, "
                      "total_bruto, total_descontos, total_acrescimos, total_liquido, cpf_cnpj, "
                      "chave_fiscal, contingencia, cancelada) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, venda->cupom_numero);
    sqlite3_bind_int(stmt, 2, engine ? engine->pdv : 1);
    sqlite3_bind_int(stmt, 3, engine ? engine->loja : 1);
    sqlite3_bind_int(stmt, 4, venda->operador.codigo);
    sqlite3_bind_text(stmt, 5, venda->data_hora_inicio, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 6, venda->total_bruto);
    sqlite3_bind_double(stmt, 7, venda->total_descontos);
    sqlite3_bind_double(stmt, 8, venda->total_acrescimos);
    sqlite3_bind_double(stmt, 9, venda->total_liquido);
    sqlite3_bind_text(stmt, 10, venda->cpf_cnpj_cliente, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 11, venda->chave_acesso_fiscal, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 12, venda->contingencia ? 1 : 0);
    sqlite3_bind_int(stmt, 13, venda->cancelada ? 1 : 0);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool zan_db_registrar_movimento_gaveta(int loja, int pdv, int operador, int tipo_funcao, double valor, const char *motivo) {
    if (!g_db) return false;

    const char *sql = "INSERT INTO movimentos_gaveta (loja, pdv, operador_cod, tipo_funcao, valor, motivo) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, loja);
    sqlite3_bind_int(stmt, 2, pdv);
    sqlite3_bind_int(stmt, 3, operador);
    sqlite3_bind_int(stmt, 4, tipo_funcao);
    sqlite3_bind_double(stmt, 5, valor);
    sqlite3_bind_text(stmt, 6, motivo ? motivo : "", -1, SQLITE_STATIC);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

int zan_db_obter_proximo_cupom(int pdv) {
    if (!g_db) return 1;
    const char *sql = "SELECT COALESCE(MAX(cupom_numero), 0) + 1 FROM vendas WHERE pdv = ?;";
    sqlite3_stmt *stmt = NULL;
    int proximo = 1;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, pdv);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            proximo = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return proximo;
}
