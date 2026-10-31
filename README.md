# Réplica do ERP / PDV Zanthus Zeus Retail (C/C++ & Photino.Native)

Uma réplica funcional e modular de alta performance do sistema de automação comercial e frente de caixa **Zanthus Zeus Retail (PDV / ERP)**, desenvolvida como exercício arquitetural:
- **Core de Negócio:** Escrito em estilo **C puro**, reproduzindo as estruturas de dados oficiais Zanthus (`ZAN_M00`, `ZAN_M01`, `ZAN_M03`, `ZAN_M04`, `ZAN_M10`, `ZAN_M43`, `ZAN_M45`), a máquina de estados finitos do caixa, cálculo de tributos (ICMS/IBPT), regras de atacarejo (TH24) e parser de código de barras.
- **Harness e Comunicação:** Aplicação hospedeira em **C++20** com servidor HTTP de loopback embutido (`cpp-httplib`) e ponte IPC bidirecional.
- **Frontend / Interface Gráfica:** **Photino.Native** com HTML5, CSS3 moderno e JavaScript nativo, reproduzindo a experiência de um terminal de checkout de supermercado profissional.
- **Banco de Dados Local:** **SQLite3** embutido para persistência local transacional ACID (resiliência offline).
- **Sistema de Build:** **CMake** multiplataforma (suporte a Windows 11 com WebView2 e Linux Debian com WebKitGTK).

---

## 🚀 Requisitos e Compilação

### Windows 11 (Ambiente de Desenvolvimento e Testes)
- Visual Studio 2022 (Build Tools ou Community com C++ Desktop)
- CMake 3.20+
- Microsoft Edge WebView2 (nativo no Windows 10/11)

**Comandos de compilação no Windows (PowerShell):**
```powershell
# 1. Configurar o projeto com CMake
cmake -B build -S .

# 2. Compilar todos os alvos em Release (pdv_core, test_core e pdv_app)
cmake --build build --config Release

# 3. Executar o teste automatizado do Core C
.\build\Release\test_core.exe

# 4. Executar a aplicação Desktop com Photino.Native
.\build\Release\pdv_app.exe
```

---

### Linux Debian (Ambiente Alvo)
Instale os pacotes de desenvolvimento necessários:
```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
    libgtk-3-dev libwebkit2gtk-4.1-dev libnotify-dev
```

**Comandos de compilação no Debian:**
```bash
cmake -B build -S .
cmake --build build --config Release
./build/pdv_app
```

---

## ⌨️ Atalhos de Teclado do Terminal PDV

| Tecla | Função | Descrição |
| :--- | :--- | :--- |
| **`ENTER`** | Registra Item | Processa o código digitado ou lido pelo leitor de código de barras. |
| **`F1`** | Ajuda | Exibe tabela de funções Zanthus e códigos. |
| **`F2`** | Subtotal | Subtotaliza a venda em andamento e calcula os totais. |
| **`F3`** | Cancela Item | Cancela o item imediatamente anterior da venda. |
| **`F4`** | Cancela Cupom | Cancela todo o cupom aberto (requer autorização de supervisor). |
| **`F5`** | Pagamento | Abre a seleção de finalizadoras (Dinheiro, TEF Débito, TEF Crédito, Pix). |
| **`F6`** | Gaveta / Sangria | Abre diálogo para registrar Sangria (158) ou Suprimento (157). |
| **`F7`** | Balança | Abre simulador de balança de checkout para itens pesáveis. |
| **`F8` / `F9`** | Menu de Funções | Executa funções avançadas digitando o código numérico Zanthus (ex: 201, 159). |
| **`F10`** | Fechamento | Executa o fechamento diário do caixa (Função 159). |
| **`ESC`** | Cancelar / Fechar | Fecha modais abertos e retorna o foco para a digitação de itens. |

---

## 🛒 Produtos Pré-Cadastrados para Teste

| Código EAN-13 / PLU | Descrição | Unidade | Preço Normal | Regra de Atacarejo (TH24) |
| :--- | :--- | :--- | :--- | :--- |
| `7891000100103` (ou `101`) | ARROZ TIPO 1 BRANCO 5KG | UN | R$ 29,90 | A partir de 5 un: **R$ 26,50** |
| `7891000200204` (ou `102`) | FEIJAO CARIOCA 1KG | UN | R$ 7,80 | A partir de 10 un: **R$ 6,99** |
| `7891000300305` (ou `103`) | OLEO DE SOJA REFINADO 900ML | UN | R$ 6,45 | A partir de 12 un: **R$ 5,80** |
| `7891000600608` (ou `106`) | LEITE INTEGRAL UHT 1L | UN | R$ 4,99 | A partir de 12 un: **R$ 4,39** |
| `7891000700709` (ou `107`) | REFRIGERANTE COLA 2L | UN | R$ 9,50 | A partir de 6 un: **R$ 8,20** |
| `7891000800800` (ou `108`) | CERVEJA PURO MALTE LATA 350ML | UN | R$ 4,19 | A partir de 12 un: **R$ 3,69** |
| `200050004500` | QUEIJO PRATO KG (BALANÇA) | KG | R$ 45,00/Kg | Etiqueta de balança padrão `2`: R$ 4,50 (100g) |
| `200051000350` | BANANA NANICA KG (BALANÇA) | KG | R$ 6,99/Kg | Etiqueta de balança padrão `2`: R$ 3,50 (500g) |

### Formatos de Digitação Aceitos:
- Código direto: `7891000100103`
- Multiplicador por teclado: `6 * 7891000100103` (ou `6*101`)
- Balança com prefixo 2: `200050004500`

---

## 📋 Códigos de Funções Zanthus Homologados (Manual M008)

- **`151`**: Abertura de Caixa (Supervisor)
- **`154`**: Login / Entrada do Operador
- **`157`**: Suprimento de Caixa (Fundo de Troco)
- **`158`**: Sangria de Caixa (Retirada)
- **`159`**: Fechamento de Caixa com Conferência
- **`201`**: Leitura X Fiscal/Gerencial
- **`143`**: Travar / Bloquear Operação
- **`3701`**: Cancela Último Item
- **`3702`**: Cancela Item por Número Ordinal
- **`3703`**: Cancela Cupom Aberto
- **`3713`**: Desconto em R$ no Subtotal
- **`3714`**: Desconto em % no Subtotal
- **`3511`**: Ativa Contingência Offline (NFC-e)
- **`3512`**: Desativa Contingência Offline
- **`9999`**: Abertura Manual da Gaveta

---

## 👥 Operadores Padrão para Teste

- **Operador 101:** `MARIA SILVA` (Senha: `1234` | Nível 1 - Operador)
- **Operador 102:** `JOAO SANTOS` (Senha: `1234` | Nível 1 - Operador)
- **Supervisor 201:** `CARLOS SOUZA` (Senha: `2020` | Nível 2 - Supervisor)
- **Gerente 301:** `ANA LIMA` (Senha: `3030` | Nível 3 - Gerente)
- **Manutenção 401:** `SUPORTE ZANTHUS` (Senha: `9999` | Nível 4 - Manutenção)
