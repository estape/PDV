// LÓGICA PRINCIPAL DA INTERFACE PDV ZANTHUS ZEUS RETAIL

let g_state = null;
let g_selectedFin = 1; // 1: Dinheiro por padrão
let g_activeGavetaTab = 'sangria';
let g_tefStep = 0;

// Formata valores em Real brasileiro (R$ 1.234,56)
function formatCurrency(val) {
  if (typeof val !== 'number' || isNaN(val)) val = 0.0;
  return 'R$ ' + val.toFixed(2).replace('.', ',');
}

// Relógio em tempo real
function startClock() {
  const clockEl = document.getElementById('lblClock');
  setInterval(() => {
    const d = new Date();
    clockEl.innerText = d.toLocaleTimeString('pt-BR');
  }, 1000);
}

// Atualização da interface com o estado completo recebido do backend C++
function updateUI(state) {
  if (!state) return;
  g_state = state;

  // Cabeçalho
  document.getElementById('lblLoja').innerText = String(state.loja).padStart(3, '0');
  document.getElementById('lblPdv').innerText = String(state.pdv).padStart(3, '0');
  document.getElementById('lblOperador').innerText = `${state.operador.codigo} - ${state.operador.nome}`;
  document.getElementById('lblStoreName').innerText = state.razao_social;
  document.getElementById('lblStoreCnpj').innerText = state.cnpj;

  // Pills de status
  const pillEstado = document.getElementById('pillEstado');
  pillEstado.className = 'pill';
  switch (state.estado) {
    case 'LIVRE':
      pillEstado.classList.add('pill-livre');
      pillEstado.innerText = 'LIVRE';
      break;
    case 'EM_VENDA':
      pillEstado.classList.add('pill-em-venda');
      pillEstado.innerText = 'EM VENDA';
      break;
    case 'SUBTOTAL':
      pillEstado.classList.add('pill-subtotal');
      pillEstado.innerText = 'SUBTOTAL';
      break;
    case 'PAGAMENTO':
      pillEstado.classList.add('pill-pagamento');
      pillEstado.innerText = 'PAGAMENTO';
      break;
    case 'BLOQUEIO_SANGRIA':
      pillEstado.classList.add('pill-bloqueio');
      pillEstado.innerText = 'BLOQUEIO SANGRIA';
      break;
    case 'TRAVADO':
      pillEstado.classList.add('pill-bloqueio');
      pillEstado.innerText = 'TRAVADO';
      break;
    default:
      pillEstado.classList.add('pill-fechado');
      pillEstado.innerText = 'FECHADO';
      break;
  }

  const pillFiscal = document.getElementById('pillFiscal');
  pillFiscal.className = 'pill ' + (state.modo_contingencia ? 'pill-offline' : 'pill-online');
  pillFiscal.innerText = state.modo_contingencia ? 'CONTINGÊNCIA OFFLINE' : 'NFC-e ONLINE';

  const pillGaveta = document.getElementById('pillGaveta');
  pillGaveta.className = 'pill ' + (state.gaveta_aberta ? 'pill-gaveta-aberta' : 'pill-gaveta-fechada');
  pillGaveta.innerText = state.gaveta_aberta ? 'GAVETA ABERTA' : 'GAVETA FECHADA';

  // Visor de mensagens
  document.getElementById('lblVisorTexto').innerText = state.ultima_mensagem || '';

  // Cupom Número
  document.getElementById('lblCupom').innerText = String(state.venda ? state.venda.cupom_numero : state.ultimo_cupom).padStart(6, '0');

  // Rolo de fita do cupom (itens)
  renderReceiptTape(state.venda);

  // Painel lateral e Totais
  const venda = state.venda;
  if (venda && venda.total_itens > 0) {
    // Último item ativo
    let lastActiveItem = null;
    for (let i = venda.itens.length - 1; i >= 0; i--) {
      if (!venda.itens[i].cancelado) {
        lastActiveItem = venda.itens[i];
        break;
      }
    }

    if (lastActiveItem) {
      document.getElementById('lblCurrentItemDesc').innerText = lastActiveItem.descricao;
      document.getElementById('lblCurrentItemCode').innerText = lastActiveItem.codigo;
      document.getElementById('lblCurrentItemQty').innerText = `${lastActiveItem.quantidade.toFixed(3)} ${lastActiveItem.unidade}`;
      document.getElementById('lblCurrentItemPrice').innerText = formatCurrency(lastActiveItem.preco_unitario);
      document.getElementById('lblCurrentItemTotal').innerText = formatCurrency(lastActiveItem.valor_total);
    }

    document.getElementById('lblSubtotalLarge').innerText = (venda.total_liquido || 0).toFixed(2).replace('.', ',');
    document.getElementById('lblQtdItensTotal').innerText = venda.total_itens;
    document.getElementById('lblSaldoRestante').innerText = formatCurrency(venda.saldo_restante);
  } else {
    document.getElementById('lblCurrentItemDesc').innerText = (state.estado === 'FECHADO') ? 'CAIXA FECHADO' : 'CAIXA LIVRE';
    document.getElementById('lblCurrentItemCode').innerText = '---';
    document.getElementById('lblCurrentItemQty').innerText = '---';
    document.getElementById('lblCurrentItemPrice').innerText = 'R$ 0,00';
    document.getElementById('lblCurrentItemTotal').innerText = 'R$ 0,00';
    document.getElementById('lblSubtotalLarge').innerText = '0,00';
    document.getElementById('lblQtdItensTotal').innerText = '0';
    document.getElementById('lblSaldoRestante').innerText = 'R$ 0,00';
  }

  // Saldo na gaveta para modal de sangria
  if (state.acumuladores) {
    document.getElementById('lblSaldoGaveta').innerText = formatCurrency(state.acumuladores.total_dinheiro_gaveta);
  }
}

// Renderiza a fita de cupom contínuo
function renderReceiptTape(venda) {
  const listEl = document.getElementById('receiptItemsList');
  const payListEl = document.getElementById('receiptPaymentsList');

  if (!venda || !venda.itens || venda.itens.length === 0) {
    listEl.innerHTML = '<div class="empty-receipt-msg">CAIXA LIVRE<br>AGUARDANDO PRÓXIMO CLIENTE</div>';
    document.getElementById('lblRcptBruto').innerText = 'R$ 0,00';
    document.getElementById('lblRcptLiquido').innerText = 'R$ 0,00';
    document.getElementById('rowRcptDesconto').style.display = 'none';
    document.getElementById('rowRcptTroco').style.display = 'none';
    document.getElementById('receiptFiscalFooter').style.display = 'none';
    payListEl.innerHTML = '';
    return;
  }

  let html = '';
  for (let it of venda.itens) {
    const canceladoClass = it.cancelado ? 'item-cancelado' : '';
    const atacadoBadge = it.atacado_aplicado ? '<span class="badge-atacado">ATACAREJO</span>' : '';
    const canceladoLabel = it.cancelado ? ' [CANCELADO]' : '';

    html += `
      <div class="receipt-item-row ${canceladoClass}">
        <div class="receipt-item-top">
          <span>${String(it.item_num).padStart(3, '0')} ${it.descricao}${atacadoBadge}${canceladoLabel}</span>
          <span>${formatCurrency(it.valor_total)}</span>
        </div>
        <div class="receipt-item-sub">
          <span>${it.codigo}</span>
          <span>${it.quantidade.toFixed(3)} ${it.unidade} x ${formatCurrency(it.preco_unitario)}</span>
        </div>
      </div>
    `;
  }
  listEl.innerHTML = html;

  // Auto-scroll para o final da fita
  const paper = document.getElementById('receiptPaper');
  paper.scrollTop = paper.scrollHeight;

  // Totais
  document.getElementById('lblRcptBruto').innerText = formatCurrency(venda.total_bruto);
  document.getElementById('lblRcptLiquido').innerText = formatCurrency(venda.total_liquido);

  if (venda.total_descontos > 0) {
    document.getElementById('rowRcptDesconto').style.display = 'flex';
    document.getElementById('lblRcptDesconto').innerText = '- ' + formatCurrency(venda.total_descontos);
  } else {
    document.getElementById('rowRcptDesconto').style.display = 'none';
  }

  // Pagamentos
  let payHtml = '';
  if (venda.pagamentos && venda.pagamentos.length > 0) {
    for (let p of venda.pagamentos) {
      payHtml += `
        <div class="receipt-total-row">
          <span>${p.descricao}:</span>
          <span>${formatCurrency(p.valor)}</span>
        </div>
      `;
    }
  }
  payListEl.innerHTML = payHtml;

  if (venda.troco > 0) {
    document.getElementById('rowRcptTroco').style.display = 'flex';
    document.getElementById('lblRcptTroco').innerText = formatCurrency(venda.troco);
  } else {
    document.getElementById('rowRcptTroco').style.display = 'none';
  }

  // Rodapé fiscal se houver chave
  if (venda.chave_fiscal && venda.chave_fiscal.length > 0) {
    document.getElementById('receiptFiscalFooter').style.display = 'block';
    document.getElementById('lblRcptChave').innerText = venda.chave_fiscal;
  }
}

// ==========================================================================
// AÇÕES DO OPERADOR
// ==========================================================================

function handleEnterBarcode() {
  const inputEl = document.getElementById('txtBarcode');
  const text = inputEl.value.trim();
  if (!text) return;

  PDV_IPC.send('REGISTRAR_ITEM', { input: text });
  inputEl.value = '';
}

function handleSubtotal() {
  PDV_IPC.send('SUBTOTAL', {});
}

function handleCancelaItem() {
  PDV_IPC.send('CANCELA_ULTIMO_ITEM', {});
}

function handleCancelaCupom() {
  if (confirm("Confirma o cancelamento de todo o cupom em andamento?")) {
    PDV_IPC.send('CANCELA_CUPOM', { supervisor: 201, senha: '2020' });
  }
}

function abrirModalPagamento() {
  if (!g_state || !g_state.venda || g_state.venda.total_itens === 0) {
    alert("Nenhuma venda em andamento para pagar!");
    return;
  }

  // Subtotaliza primeiro se ainda estiver em venda
  if (g_state.estado === 'EM_VENDA') {
    handleSubtotal();
  }

  const saldo = g_state.venda.saldo_restante || g_state.venda.total_liquido;
  document.getElementById('lblModalTotalPagar').innerText = formatCurrency(saldo);
  document.getElementById('txtValorPago').value = saldo.toFixed(2);
  document.getElementById('boxTrocoCalculado').style.display = 'none';

  fecharModais();
  document.getElementById('modalPagamento').style.display = 'flex';
  document.getElementById('txtValorPago').focus();
}

function confirmarPagamento() {
  const valor = parseFloat(document.getElementById('txtValorPago').value);
  if (isNaN(valor) || valor <= 0) {
    alert("Informe um valor válido!");
    return;
  }

  if (g_selectedFin === 3 || g_selectedFin === 4) {
    // TEF: Abre fluxo do Pinpad
    fecharModais();
    iniciarFluxoTef(g_selectedFin, valor);
    return;
  }

  PDV_IPC.send('PAGAMENTO', {
    finalizadora: g_selectedFin,
    valor: valor,
    nsu: '',
    autorizacao: ''
  });

  fecharModais();
}

function iniciarFluxoTef(modalidade, valor) {
  g_tefStep = 1;
  document.getElementById('modalTef').style.display = 'flex';
  document.getElementById('lblTefMsg').innerText = "INSIRA OU APROXIME O CARTÃO";
  document.getElementById('lblTefInput').innerText = "";

  setTimeout(() => {
    document.getElementById('lblTefMsg').innerText = "DIGITE A SENHA DO CARTÃO";
    document.getElementById('lblTefInput').innerText = "****";
  }, 1200);
}

function tefConfirma() {
  document.getElementById('lblTefMsg').innerText = "PROCESSANDO TRANSAÇÃO...";
  document.getElementById('lblTefInput').innerText = "";

  setTimeout(() => {
    document.getElementById('lblTefMsg').innerText = "AUTORIZADA! RETIRE O CARTÃO";
    setTimeout(() => {
      fecharModais();
      const valor = parseFloat(document.getElementById('txtValorPago').value) || 0.0;
      PDV_IPC.send('PAGAMENTO', {
        finalizadora: g_selectedFin,
        valor: valor,
        nsu: String(Math.floor(100000 + Math.random() * 900000)),
        autorizacao: String(Math.floor(200000 + Math.random() * 800000))
      });
    }, 1000);
  }, 1200);
}

function tefCancela() {
  fecharModais();
  alert("Transação TEF cancelada pelo operador!");
}

function tefKey(ch) {
  const el = document.getElementById('lblTefInput');
  el.innerText += '*';
}

function abrirModalFuncoes() {
  fecharModais();
  document.getElementById('modalFuncoes').style.display = 'flex';
  document.getElementById('txtNumFuncao').value = '';
  document.getElementById('txtNumFuncao').focus();
}

function executarFuncaoDigitada() {
  const cod = parseInt(document.getElementById('txtNumFuncao').value);
  if (isNaN(cod)) return;

  PDV_IPC.send('EXECUTAR_FUNCAO', { codigo: cod });
  fecharModais();
}

function abrirModalGaveta() {
  fecharModais();
  document.getElementById('modalGaveta').style.display = 'flex';
  document.getElementById('txtGavetaValor').value = '';
  document.getElementById('txtGavetaMotivo').value = '';
  document.getElementById('txtGavetaValor').focus();
}

function switchGavetaTab(tab) {
  g_activeGavetaTab = tab;
  document.getElementById('tabSangria').className = 'tab-btn ' + (tab === 'sangria' ? 'active' : '');
  document.getElementById('tabSuprimento').className = 'tab-btn ' + (tab === 'suprimento' ? 'active' : '');
}

function confirmarGaveta() {
  const val = parseFloat(document.getElementById('txtGavetaValor').value);
  const motivo = document.getElementById('txtGavetaMotivo').value;
  if (isNaN(val) || val <= 0) {
    alert("Informe um valor válido!");
    return;
  }

  if (g_activeGavetaTab === 'sangria') {
    PDV_IPC.send('SANGRIA', { valor: val, motivo: motivo });
  } else {
    PDV_IPC.send('SUPRIMENTO', { valor: val, motivo: motivo });
  }
  fecharModais();
}

function abrirModalBalanca() {
  fecharModais();
  document.getElementById('modalBalanca').style.display = 'flex';
}

function confirmarBalanca() {
  const plu = document.getElementById('selProdutoBalanca').value;
  const peso = parseFloat(document.getElementById('txtPesoKg').value);
  if (isNaN(peso) || peso <= 0) return;

  // Monta entrada multiplicada: peso * PLU (ex: 0.500 * 50)
  PDV_IPC.send('REGISTRAR_ITEM', { input: `${peso} * ${plu}` });
  fecharModais();
}

function abrirModalImpressao(titulo, texto) {
  fecharModais();
  document.getElementById('lblPrinterDocTitle').innerText = titulo || "COMPROVANTE FISCAL";
  document.getElementById('printerText').innerText = texto || "";
  document.getElementById('modalImpressao').style.display = 'flex';
}

function fecharModais() {
  const modais = document.querySelectorAll('.modal-overlay');
  modais.forEach(m => m.style.display = 'none');
  const txt = document.getElementById('txtBarcode');
  if (txt) txt.focus();
}

// Configuração dos atalhos de teclado (F1 - F10, Enter, Esc)
function setupKeymap() {
  window.addEventListener('keydown', (e) => {
    // Se algum modal estiver aberto e for ESC
    if (e.key === 'Escape') {
      fecharModais();
      e.preventDefault();
      return;
    }

    // Atalhos globais
    switch (e.key) {
      case 'F1':
        abrirModalFuncoes();
        e.preventDefault();
        break;
      case 'F2':
        handleSubtotal();
        e.preventDefault();
        break;
      case 'F3':
        handleCancelaItem();
        e.preventDefault();
        break;
      case 'F4':
        handleCancelaCupom();
        e.preventDefault();
        break;
      case 'F5':
        abrirModalPagamento();
        e.preventDefault();
        break;
      case 'F6':
        abrirModalGaveta();
        e.preventDefault();
        break;
      case 'F7':
        abrirModalBalanca();
        e.preventDefault();
        break;
      case 'F8':
        abrirModalFuncoes();
        e.preventDefault();
        break;
      case 'F9':
        abrirModalFuncoes();
        e.preventDefault();
        break;
      case 'F10':
        if (confirm("Deseja executar o Fechamento do Caixa (Função 159)?")) {
          PDV_IPC.send('EXECUTAR_FUNCAO', { codigo: 159 });
        }
        e.preventDefault();
        break;
      case 'Enter':
        // Se o foco estiver no input principal
        if (document.activeElement.id === 'txtBarcode') {
          handleEnterBarcode();
          e.preventDefault();
        } else if (document.activeElement.id === 'txtNumFuncao') {
          executarFuncaoDigitada();
          e.preventDefault();
        } else if (document.activeElement.id === 'txtValorPago') {
          confirmarPagamento();
          e.preventDefault();
        }
        break;
    }
  });

  // Botões de finalizadora no modal de pagamento
  document.querySelectorAll('.pay-method-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      document.querySelectorAll('.pay-method-btn').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      g_selectedFin = parseInt(btn.dataset.fin);
    });
  });

  // Cálculo dinâmico de troco ao digitar valor
  document.getElementById('txtValorPago').addEventListener('input', (e) => {
    const val = parseFloat(e.target.value) || 0;
    const saldo = (g_state && g_state.venda) ? (g_state.venda.saldo_restante || g_state.venda.total_liquido) : 0;
    if (val > saldo && g_selectedFin === 1) {
      document.getElementById('boxTrocoCalculado').style.display = 'block';
      document.getElementById('lblTrocoCalculado').innerText = formatCurrency(val - saldo);
    } else {
      document.getElementById('boxTrocoCalculado').style.display = 'none';
    }
  });

  // Botões rápidos de rodapé
  document.querySelectorAll('.key-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const k = btn.dataset.key;
      window.dispatchEvent(new KeyboardEvent('keydown', { key: k }));
    });
  });

  // Botão Entra principal
  document.getElementById('btnEnter').addEventListener('click', handleEnterBarcode);
  document.getElementById('btnConfirmarPagamento').addEventListener('click', confirmarPagamento);
  document.getElementById('btnExecutarFuncao').addEventListener('click', executarFuncaoDigitada);
  document.getElementById('btnConfirmarGaveta').addEventListener('click', confirmarGaveta);
  document.getElementById('btnConfirmarBalanca').addEventListener('click', confirmarBalanca);
}

// Inicialização
document.addEventListener('DOMContentLoaded', () => {
  startClock();
  setupKeymap();

  // Escuta atualizações do backend C++
  PDV_IPC.onMessage((data) => {
    if (data.event === 'STATE_CHANGED' || data.loja) {
      updateUI(data.payload || data);
    } else if (data.event === 'PRINT_DOCUMENT') {
      abrirModalImpressao(data.titulo, data.texto);
    }
  });

  // Solicita estado inicial
  PDV_IPC.send('GET_STATE', {});
});
