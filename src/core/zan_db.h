#ifndef ZAN_DB_H
#define ZAN_DB_H

#include "zan_types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa a conexão com o banco SQLite local e cria as tabelas se não existirem
bool zan_db_init(const char *db_path);

// Fecha a conexão com o banco
void zan_db_close(void);

// Busca produto por código de barras ou código interno (PLU)
bool zan_db_buscar_produto(const char *codigo, zan_produto_t *out_prod);

// Busca produto pesável pelo código interno (PLU)
bool zan_db_buscar_produto_por_id(int id, zan_produto_t *out_prod);

// Autentica operador pelo código e senha
bool zan_db_autenticar_operador(int codigo, const char *senha, zan_operador_t *out_op);

// Salva uma venda finalizada no banco SQLite
bool zan_db_salvar_venda(const zan_venda_t *venda, const zan_engine_t *engine);

// Registra movimentação de gaveta (Suprimento / Sangria)
bool zan_db_registrar_movimento_gaveta(int loja, int pdv, int operador, int tipo_funcao, double valor, const char *motivo);

// Obtém o próximo número de cupom sequencial
int zan_db_obter_proximo_cupom(int pdv);

#ifdef __cplusplus
}
#endif

#endif // ZAN_DB_H
