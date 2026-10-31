// PONTE IPC ENTRE JAVASCRIPT E O BACKEND C/C++ (PHOTINO.NATIVE)

const PDV_IPC = (function() {
  let messageHandlers = [];

  // Registra listener no Photino Native se disponível
  function initPhotino() {
    if (window.external && typeof window.external.receiveMessage === 'function') {
      window.external.receiveMessage(function(msg) {
        try {
          const data = JSON.parse(msg);
          dispatch(data);
        } catch(e) {
          console.error("Erro ao fazer parse da mensagem recebida do Photino:", e, msg);
        }
      });
      console.log("Ponte Photino.Native IPC inicializada com sucesso.");
    } else {
      console.warn("Photino.Native não detectado. Usando fallback HTTP loopback (/api).");
      // Inicia polling leve como fallback para desenvolvimento em navegador comum
      startHttpPolling();
    }
  }

  function dispatch(data) {
    for (let handler of messageHandlers) {
      handler(data);
    }
  }

  // Envia comando para o backend C++
  function sendCommand(action, payload = {}) {
    const packet = {
      action: action,
      payload: payload
    };

    const packetStr = JSON.stringify(packet);

    // Tentativa 1: Via Photino Native Message Bridge
    if (window.external && typeof window.external.sendMessage === 'function') {
      window.external.sendMessage(packetStr);
      return;
    }

    // Tentativa 2: Fallback via HTTP POST
    fetch('/api/command', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: packetStr
    })
    .then(res => res.json())
    .then(data => {
      dispatch(data);
    })
    .catch(err => {
      console.error("Erro na comunicação HTTP com o backend PDV:", err);
    });
  }

  function startHttpPolling() {
    setInterval(() => {
      fetch('/api/state')
        .then(res => res.json())
        .then(state => {
          dispatch({ event: 'STATE_CHANGED', payload: state });
        })
        .catch(() => {});
    }, 1000);
  }

  function onMessage(callback) {
    messageHandlers.push(callback);
  }

  // Auto-inicializa quando o DOM estiver pronto
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initPhotino);
  } else {
    initPhotino();
  }

  return {
    send: sendCommand,
    onMessage: onMessage
  };
})();
