#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <filesystem>

// Core C Zanthus
#include "core/zan_types.h"
#include "core/zan_constants.h"
#include "core/zan_engine.h"
#include "core/zan_fiscal.h"

// cpp-httplib & JSON
#include "../third_party/httplib/httplib.h"
#include "../third_party/nlohmann/json.hpp"

// Photino Native
#include "../third_party/photino/Photino.Native/Photino.h"

using json = nlohmann::json;

static zan_engine_t g_engine;
static std::mutex g_engine_mutex;
static Photino *g_photino = nullptr;
static int g_server_port = 8088;

#ifdef _WIN32
static std::string WideToUtf8(const wchar_t *wstr) {
    if (!wstr) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static std::wstring Utf8ToWide(const std::string &str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
#endif

// Envia mensagem JSON para a interface gráfica
static void BroadcastToUI(const std::string &json_str) {
    if (!g_photino) return;

#ifdef _WIN32
    std::wstring wmsg = Utf8ToWide(json_str);
    g_photino->SendWebMessage((AutoString)wmsg.c_str());
#else
    g_photino->SendWebMessage((AutoString)json_str.c_str());
#endif
}

// Processa pacotes de comandos recebidos da interface
static std::string ProcessCommand(const std::string &json_cmd) {
    std::lock_guard<std::mutex> lock(g_engine_mutex);

    try {
        auto j = json::parse(json_cmd);
        std::string action = j.value("action", "");
        auto payload = j.value("payload", json::object());

        if (action == "GET_STATE") {
            char buffer[16384];
            zan_engine_to_json(&g_engine, buffer, sizeof(buffer));
            return std::string(buffer);
        }

        if (action == "REGISTRAR_ITEM") {
            std::string input = payload.value("input", "");
            zan_engine_registrar_item(&g_engine, input.c_str());
        }
        else if (action == "SUBTOTAL") {
            zan_engine_subtotalizar(&g_engine);
        }
        else if (action == "CANCELA_ULTIMO_ITEM") {
            zan_engine_cancelar_ultimo_item(&g_engine);
        }
        else if (action == "CANCELA_ITEM_NUM") {
            int num = payload.value("item_num", 0);
            zan_engine_cancelar_item_num(&g_engine, num);
        }
        else if (action == "CANCELA_CUPOM") {
            int sup = payload.value("supervisor", 201);
            std::string senha = payload.value("senha", "2020");
            zan_engine_cancelar_cupom(&g_engine, sup, senha.c_str());
        }
        else if (action == "PAGAMENTO") {
            int fin = payload.value("finalizadora", 1);
            double valor = payload.value("valor", 0.0);
            std::string nsu = payload.value("nsu", "");
            std::string aut = payload.value("autorizacao", "");
            zan_engine_adicionar_pagamento(&g_engine, (zan_finalizadora_t)fin, valor, nsu.c_str(), aut.c_str());

            // Se o saldo restante for zero, finaliza a venda automaticamente e emite o DANFE
            if (g_engine.venda_atual.saldo_restante <= 0.001 && g_engine.venda_atual.total_itens > 0) {
                char danfe[8192];
                zan_engine_finalizar_venda(&g_engine, danfe, sizeof(danfe));

                // Notifica a UI para abrir modal de impressão do DANFE
                json printEvt;
                printEvt["event"] = "PRINT_DOCUMENT";
                printEvt["titulo"] = "DANFE NFC-e - EXTRATO ELETRÔNICO";
                printEvt["texto"] = danfe;
                BroadcastToUI(printEvt.dump());
            }
        }
        else if (action == "SANGRIA") {
            double valor = payload.value("valor", 0.0);
            std::string motivo = payload.value("motivo", "SANGRIA DE ROTINA");
            char recibo[4096];
            if (zan_engine_sangria(&g_engine, valor, motivo.c_str(), recibo, sizeof(recibo))) {
                json printEvt;
                printEvt["event"] = "PRINT_DOCUMENT";
                printEvt["titulo"] = "COMPROVANTE DE SANGRIA";
                printEvt["texto"] = recibo;
                BroadcastToUI(printEvt.dump());
            }
        }
        else if (action == "SUPRIMENTO") {
            double valor = payload.value("valor", 0.0);
            std::string motivo = payload.value("motivo", "FUNDO DE TROCO");
            char recibo[4096];
            if (zan_engine_suprimento(&g_engine, valor, motivo.c_str(), recibo, sizeof(recibo))) {
                json printEvt;
                printEvt["event"] = "PRINT_DOCUMENT";
                printEvt["titulo"] = "COMPROVANTE DE SUPRIMENTO";
                printEvt["texto"] = recibo;
                BroadcastToUI(printEvt.dump());
            }
        }
        else if (action == "EXECUTAR_FUNCAO") {
            int cod = payload.value("codigo", 0);
            char msg[4096];
            zan_engine_executar_funcao(&g_engine, cod, "", msg, sizeof(msg));

            if (cod == ZAN_FUNC_LEITURA_X) {
                json printEvt;
                printEvt["event"] = "PRINT_DOCUMENT";
                printEvt["titulo"] = "LEITURA X (FUNÇÃO 201)";
                printEvt["texto"] = msg;
                BroadcastToUI(printEvt.dump());
            } else if (cod == ZAN_FUNC_FECHAMENTO_CAIXA) {
                json printEvt;
                printEvt["event"] = "PRINT_DOCUMENT";
                printEvt["titulo"] = "FECHAMENTO DE CAIXA (FUNÇÃO 159)";
                printEvt["texto"] = msg;
                BroadcastToUI(printEvt.dump());
            }
        }
        else if (action == "LOGIN") {
            int op = payload.value("operador", 101);
            std::string senha = payload.value("senha", "1234");
            zan_engine_login_operador(&g_engine, op, senha.c_str());
        }

        char buffer[16384];
        zan_engine_to_json(&g_engine, buffer, sizeof(buffer));
        std::string stateStr(buffer);
        BroadcastToUI(stateStr);
        return stateStr;
    }
    catch (const std::exception &e) {
        std::cerr << "Erro no processamento do comando: " << e.what() << std::endl;
        return "{\"error\":\"" + std::string(e.what()) + "\"}";
    }
}

// Callback de mensagem recebida do Photino.Native via IPC
static void OnPhotinoWebMessage(AutoString message) {
    if (!message) return;
#ifdef _WIN32
    std::string utf8_cmd = WideToUtf8(message);
#else
    std::string utf8_cmd = message;
#endif
    ProcessCommand(utf8_cmd);
}

// Servidor HTTP embutido para entrega de assets locais e endpoints REST
static void RunHttpServer(int port) {
    httplib::Server svr;

    // Detecta localização dos arquivos estáticos da UI
    std::string ui_dir = "./ui";
    if (std::filesystem::exists("./ui")) {
        ui_dir = "./ui";
    } else if (std::filesystem::exists("../ui")) {
        ui_dir = "../ui";
    } else if (std::filesystem::exists("../../ui")) {
        ui_dir = "../../ui";
    }

    svr.set_mount_point("/", ui_dir);
    std::cout << "[HTTP Server] Servindo pasta estática UI de: " << ui_dir << std::endl;

    svr.Get("/api/state", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(g_engine_mutex);
        char buffer[16384];
        zan_engine_to_json(&g_engine, buffer, sizeof(buffer));
        res.set_content(buffer, "application/json");
    });

    svr.Post("/api/command", [](const httplib::Request& req, httplib::Response& res) {
        std::string resp = ProcessCommand(req.body);
        res.set_content(resp, "application/json");
    });

    std::cout << "[HTTP Server] Servidor local PDV iniciado em http://127.0.0.1:" << port << std::endl;
    svr.listen("127.0.0.1", port);
}

int main(int argc, char **argv) {
    std::cout << "======================================================" << std::endl;
    std::cout << "  ZANTHUS ZEUS RETAIL - REPLICA DE PDV COMERCIAL C/C++" << std::endl;
    std::cout << "======================================================" << std::endl;

    // 1. Inicializa o motor Zanthus em C Puro
    if (!zan_engine_init(&g_engine, 1, 1, "pdv_local.db")) {
        std::cerr << "Erro fatal: Falha ao inicializar o banco de dados do PDV!" << std::endl;
        return 1;
    }

    // Login inicial padrão do operador 101
    zan_engine_login_operador(&g_engine, 101, "1234");
    // Suprimento inicial de abertura de caixa
    zan_engine_suprimento(&g_engine, 150.00, "FUNDO DE TROCO INICIAL", nullptr, 0);

    // 2. Inicia o servidor HTTP em thread separada
    std::thread httpThread(RunHttpServer, g_server_port);
    httpThread.detach();

    // Aguarda 200ms para garantir que o socket local abriu
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // 3. Inicializa o Photino.Native Desktop Window
#ifdef _WIN32
    Photino::Register(GetModuleHandle(NULL));
#elif __APPLE__
    Photino::Register();
#endif

    std::string startUrl = "http://127.0.0.1:" + std::to_string(g_server_port) + "/index.html";
    std::string titleStr = "Zanthus Zeus Retail - PDV Caixa 01";

#ifdef _WIN32
    std::wstring wStartUrl = Utf8ToWide(startUrl);
    std::wstring wTitle = Utf8ToWide(titleStr);
#endif

    PhotinoInitParams params = {};
    params.Size = sizeof(PhotinoInitParams);
#ifdef _WIN32
    params.StartUrl = (AutoString)wStartUrl.c_str();
    params.Title = (AutoString)wTitle.c_str();
#else
    params.StartUrl = (AutoString)startUrl.c_str();
    params.Title = (AutoString)titleStr.c_str();
#endif
    params.Width = 1280;
    params.Height = 820;
    params.MinWidth = 1024;
    params.MinHeight = 700;
    params.CenterOnInitialize = true;
    params.Resizable = true;
    params.DevToolsEnabled = true;
    params.WebMessageReceivedHandler = (WebMessageReceivedCallback*)&OnPhotinoWebMessage;

    std::cout << "[Photino] Criando janela nativa..." << std::endl;
    Photino window(&params);
    g_photino = &window;

    std::cout << "[Photino] Entrando no loop de eventos principal..." << std::endl;
    window.WaitForExit();

    std::cout << "[PDV] Encerrando aplicacao..." << std::endl;
    return 0;
}
