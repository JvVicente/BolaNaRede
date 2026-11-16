#include <WiFi.h>
#include <WebServer.h>
#include <DFRobotDFPlayerMini.h>

#include "secrets.h"

const int IR_PINS[3] = {32, 33, 25};

const bool IR_CORTADO_EM_HIGH[3] = {false, true, false};

const int TRIG_PIN = 13;
const int ECHO_PIN = 34;
const int DFPLAYER_TX_PIN = 14;

const int TOTAL_SONS = 3;
const unsigned long BLOQUEIO_MS = 5000;

const unsigned long DURACAO_BOLA_MS = 100;
const unsigned long MIN_PULSO_US = 6000;
const unsigned long GAP_UNIAO_US = 8000;
const unsigned long SILENCIO_MS = 800;

const unsigned long ESPERA_US_MS = 80;
const unsigned long AVALIACAO_MS = 220;
const bool USAR_US_NO_VETO = false;

const float MARGEM_VAZIO_CM = 5;
const float DIST_VAZIO_PADRAO_CM = 46.4;
const bool DEBUG_US = true;
const bool MODO_CALIBRACAO = true;
const int MAX_GOLS = 100;

const unsigned long DURACAO_BOLA_US = DURACAO_BOLA_MS * 1000UL;

WebServer servidor(80);
DFRobotDFPlayerMini player;

enum Estado { LIVRE, AVALIANDO, AGUARDANDO_LIVRE };
Estado estado = LIVRE;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile bool irCortado[3] = {false, false, false};
volatile bool pendente[3] = {false, false, false};
volatile bool pulsoPronto[3] = {false, false, false};
volatile unsigned long corteUs[3] = {0, 0, 0};
volatile unsigned long liberaUs[3] = {0, 0, 0};
volatile unsigned long inicioUs[3] = {0, 0, 0};
volatile unsigned long duracaoUs[3] = {0, 0, 0};

unsigned long silencioAte = 0;
unsigned long inicioAvaliacao = 0;
unsigned long bloqueioAte = 0;

float distanciaVazio = DIST_VAZIO_PADRAO_CM;
unsigned long proximaLeitura = 0;
int leiturasOcupadas = 0;
int leiturasVazias = 0;

bool cortado = false;
unsigned long contagem = 0;
unsigned long ultimoCorte = 0;
unsigned long golsMs[MAX_GOLS];

const char PAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Bola na Rede</title>
  <style>
    :root {
      --bg: #0b1410;
      --card: #12201a;
      --borda: #1f3a2d;
      --texto: #e8f5ec;
      --suave: #7fa08c;
      --verde: #2ee27a;
      --vermelho: #ff4d4d;
      --ouro: #ffd23f;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
      color: var(--texto);
      background:
        repeating-linear-gradient(90deg, rgba(255,255,255,.025) 0 60px, transparent 60px 120px),
        radial-gradient(circle at 50% 0%, #17382a 0%, var(--bg) 60%);
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 24px 16px;
    }
    header {
      width: 100%;
      max-width: 480px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
    }
    h1 { font-size: 1.1rem; margin: 0; letter-spacing: .08em; text-transform: uppercase; }
    h1 span { color: var(--verde); }
    .conexao {
      font-size: .75rem;
      color: var(--suave);
      display: flex;
      align-items: center;
      gap: 6px;
    }
    .ponto {
      width: 9px; height: 9px; border-radius: 50%;
      background: var(--vermelho);
    }
    .ponto.on { background: var(--verde); box-shadow: 0 0 8px var(--verde); }

    .placar {
      width: 100%;
      max-width: 480px;
      background: var(--card);
      border: 1px solid var(--borda);
      border-radius: 20px;
      padding: 28px 20px;
      text-align: center;
      box-shadow: 0 10px 40px rgba(0,0,0,.4);
    }
    .rotulo {
      font-size: .8rem;
      letter-spacing: .25em;
      text-transform: uppercase;
      color: var(--suave);
    }
    #gols {
      font-size: clamp(6rem, 32vw, 10rem);
      font-weight: 800;
      line-height: 1;
      margin: 8px 0 4px;
      font-variant-numeric: tabular-nums;
      color: var(--ouro);
      text-shadow: 0 0 30px rgba(255,210,63,.35);
      transition: transform .2s;
    }
    #gols.pulo { transform: scale(1.18); }

    .sensor {
      margin: 22px auto 0;
      padding: 12px 18px;
      border-radius: 999px;
      font-weight: 600;
      font-size: .95rem;
      display: inline-flex;
      align-items: center;
      gap: 10px;
      background: rgba(46,226,122,.12);
      color: var(--verde);
      border: 1px solid rgba(46,226,122,.35);
      transition: all .15s;
    }
    .sensor.ativo {
      background: rgba(255,77,77,.15);
      color: var(--vermelho);
      border-color: rgba(255,77,77,.5);
    }
    .sensor i {
      width: 10px; height: 10px; border-radius: 50%;
      background: currentColor;
    }

    .info {
      display: flex;
      gap: 12px;
      margin-top: 22px;
    }
    .info div {
      flex: 1;
      background: rgba(0,0,0,.25);
      border: 1px solid var(--borda);
      border-radius: 12px;
      padding: 12px 8px;
    }
    .info b { display: block; font-size: 1.15rem; margin-top: 4px; }

    .botoes {
      display: flex;
      gap: 10px;
      justify-content: center;
      flex-wrap: wrap;
      margin-top: 22px;
    }
    button, .botao {
      background: transparent;
      color: var(--suave);
      border: 1px solid var(--borda);
      border-radius: 10px;
      padding: 10px 18px;
      font-size: .85rem;
      cursor: pointer;
      text-decoration: none;
      font-family: inherit;
    }
    button:hover, .botao:hover { color: var(--texto); border-color: var(--suave); }
    .botao.destaque {
      color: var(--bg);
      background: var(--ouro);
      border-color: var(--ouro);
      font-weight: 700;
    }
    .botao.destaque:hover { color: var(--bg); filter: brightness(1.1); }

    #overlay {
      position: fixed; inset: 0;
      display: flex; align-items: center; justify-content: center;
      background: rgba(11,20,16,.85);
      font-size: clamp(4rem, 25vw, 9rem);
      font-weight: 900;
      color: var(--ouro);
      text-shadow: 0 0 50px rgba(255,210,63,.6);
      letter-spacing: .05em;
      opacity: 0;
      pointer-events: none;
      transition: opacity .25s;
    }
    #overlay.show { opacity: 1; animation: tremer .5s 2; }
    @keyframes tremer {
      0%, 100% { transform: translate(0); }
      25% { transform: translate(-6px, 4px) rotate(-1deg); }
      75% { transform: translate(6px, -4px) rotate(1deg); }
    }
  </style>
</head>
<body>
  <header>
    <h1>&#9917; Bola na <span>Rede</span></h1>
    <div class="conexao"><div class="ponto" id="ponto"></div><span id="conexao">offline</span></div>
  </header>

  <main class="placar">
    <div class="rotulo">Gols</div>
    <div id="gols">0</div>

    <div class="sensor" id="sensor"><i></i><span id="sensorTxt">Aguardando bola</span></div>

    <div class="info">
      <div><span class="rotulo">Ultimo gol</span><b id="ultimo">-</b></div>
    </div>

    <div class="botoes">
      <a class="botao destaque" href="/historico">Ver todos os gols</a>
      <button onclick="zerar()">Zerar placar</button>
    </div>
  </main>

  <div id="overlay">GOOOL!</div>

  <script>
    const $ = id => document.getElementById(id);
    let anterior = null;
    let timerOverlay = null;

    function apito() {
      try {
        const ctx = new (window.AudioContext || window.webkitAudioContext)();
        const o = ctx.createOscillator();
        const g = ctx.createGain();
        o.type = 'square';
        o.frequency.value = 880;
        g.gain.value = 0.08;
        o.connect(g); g.connect(ctx.destination);
        o.start();
        o.stop(ctx.currentTime + 0.35);
      } catch (e) {}
    }

    function gol() {
      $('overlay').classList.add('show');
      $('gols').classList.add('pulo');
      apito();
      clearTimeout(timerOverlay);
      timerOverlay = setTimeout(() => {
        $('overlay').classList.remove('show');
        $('gols').classList.remove('pulo');
      }, 2200);
    }

    function tempo(ms) {
      if (ms < 0) return '-';
      const s = Math.floor(ms / 1000);
      if (s < 60) return s + ' s';
      return Math.floor(s / 60) + ' min ' + (s % 60) + ' s';
    }

    async function atualizar() {
      try {
        const r = await fetch('/dados');
        const d = await r.json();

        $('ponto').classList.add('on');
        $('conexao').textContent = 'online';

        $('gols').textContent = d.contagem;
        $('ultimo').textContent = tempo(d.ultimo_ms);

        $('sensor').classList.toggle('ativo', d.cortado);
        $('sensorTxt').textContent = d.cortado ? 'Objeto detectado!' : 'Aguardando bola';

        if (anterior !== null && d.contagem > anterior) gol();
        anterior = d.contagem;
      } catch (err) {
        $('ponto').classList.remove('on');
        $('conexao').textContent = 'offline';
      }
    }

    async function zerar() {
      if (!confirm('Zerar o placar?')) return;
      try { await fetch('/zerar'); anterior = 0; atualizar(); } catch (e) {}
    }

    setInterval(atualizar, 150);
    atualizar();
  </script>
</body>
</html>
)rawliteral";

const char PAGINA_HISTORICO[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Historico de Gols</title>
  <style>
    :root {
      --bg: #0b1410;
      --card: #12201a;
      --borda: #1f3a2d;
      --texto: #e8f5ec;
      --suave: #7fa08c;
      --verde: #2ee27a;
      --ouro: #ffd23f;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
      color: var(--texto);
      background:
        repeating-linear-gradient(90deg, rgba(255,255,255,.025) 0 60px, transparent 60px 120px),
        radial-gradient(circle at 50% 0%, #17382a 0%, var(--bg) 60%);
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 24px 16px;
    }
    header {
      width: 100%;
      max-width: 480px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
    }
    h1 { font-size: 1.1rem; margin: 0; letter-spacing: .08em; text-transform: uppercase; }
    h1 span { color: var(--verde); }
    .botao {
      color: var(--suave);
      border: 1px solid var(--borda);
      border-radius: 10px;
      padding: 8px 14px;
      font-size: .85rem;
      text-decoration: none;
    }
    .botao:hover { color: var(--texto); border-color: var(--suave); }

    .painel {
      width: 100%;
      max-width: 480px;
      background: var(--card);
      border: 1px solid var(--borda);
      border-radius: 20px;
      padding: 20px;
      box-shadow: 0 10px 40px rgba(0,0,0,.4);
    }
    .total {
      text-align: center;
      font-size: .8rem;
      letter-spacing: .25em;
      text-transform: uppercase;
      color: var(--suave);
      margin-bottom: 16px;
    }
    .total b { color: var(--ouro); font-size: 1.4rem; letter-spacing: 0; margin-left: 6px; }

    ul { list-style: none; margin: 0; padding: 0; display: flex; flex-direction: column; gap: 8px; }
    li {
      display: flex;
      align-items: center;
      gap: 14px;
      background: rgba(0,0,0,.25);
      border: 1px solid var(--borda);
      border-radius: 12px;
      padding: 12px 14px;
    }
    li.novo { border-color: var(--ouro); }
    .num {
      min-width: 44px;
      text-align: center;
      font-weight: 800;
      color: var(--ouro);
      font-variant-numeric: tabular-nums;
    }
    .hora {
      flex: 1;
      font-size: 1.15rem;
      font-weight: 600;
      font-variant-numeric: tabular-nums;
    }
    .data { font-size: .75rem; color: var(--suave); }
    .vazio { text-align: center; color: var(--suave); padding: 30px 0; }
  </style>
</head>
<body>
  <header>
    <h1>&#9917; Todos os <span>Gols</span></h1>
    <a class="botao" href="/">&larr; Voltar</a>
  </header>

  <main class="painel">
    <div class="total">Total de gols<b id="total">0</b></div>
    <ul id="lista"></ul>
    <div class="vazio" id="vazio">Nenhum gol ainda.</div>
  </main>

  <script>
    const $ = id => document.getElementById(id);
    let ultimoTotal = null;

    function dois(n) { return String(n).padStart(2, '0'); }

    async function atualizar() {
      try {
        const r = await fetch('/gols');
        const d = await r.json();
        const agoraBrowser = Date.now();

        $('total').textContent = d.total;
        $('vazio').style.display = d.gols.length ? 'none' : 'block';

        const lista = $('lista');
        lista.innerHTML = '';

        for (let i = d.gols.length - 1; i >= 0; i--) {
          const quando = new Date(agoraBrowser - (d.agora - d.gols[i]));
          const hora = dois(quando.getHours()) + ':' + dois(quando.getMinutes()) + ':' + dois(quando.getSeconds());
          const data = dois(quando.getDate()) + '/' + dois(quando.getMonth() + 1);

          const li = document.createElement('li');
          if (i === d.gols.length - 1 && ultimoTotal !== null && d.total > ultimoTotal) {
            li.className = 'novo';
          }
          li.innerHTML =
            '<div class="num">#' + (d.inicio + i) + '</div>' +
            '<div class="hora">' + hora + '</div>' +
            '<div class="data">' + data + '</div>';
          lista.appendChild(li);
        }
        ultimoTotal = d.total;
      } catch (err) {}
    }

    setInterval(atualizar, 1000);
    atualizar();
  </script>
</body>
</html>
)rawliteral";

void paginaInicial() { servidor.send(200, "text/html", PAGINA); }
void paginaHistorico() { servidor.send(200, "text/html", PAGINA_HISTORICO); }

void enviarDados() {
  long ultimoMs = (ultimoCorte == 0) ? -1 : (long)(millis() - ultimoCorte);
  String json = "{\"cortado\":";
  json += cortado ? "true" : "false";
  json += ",\"contagem\":" + String(contagem);
  json += ",\"ultimo_ms\":" + String(ultimoMs) + "}";
  servidor.send(200, "application/json", json);
}

void enviarGols() {
  unsigned long n = (contagem < (unsigned long)MAX_GOLS) ? contagem : MAX_GOLS;
  unsigned long inicio = contagem - n;

  String json = "{\"agora\":" + String(millis());
  json += ",\"total\":" + String(contagem);
  json += ",\"inicio\":" + String(inicio + 1);
  json += ",\"gols\":[";
  for (unsigned long i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += String(golsMs[(inicio + i) % MAX_GOLS]);
  }
  json += "]}";
  servidor.send(200, "application/json", json);
}

void zerarPlacar() {
  contagem = 0;
  ultimoCorte = 0;
  servidor.send(200, "text/plain", "ok");
}

void IRAM_ATTR fecharPulso(int i) {
  pendente[i] = false;
  unsigned long dur = liberaUs[i] - corteUs[i];
  if (dur >= MIN_PULSO_US) {
    inicioUs[i] = corteUs[i];
    duracaoUs[i] = dur;
    pulsoPronto[i] = true;
  }
}

void IRAM_ATTR tratarBorda(int i) {
  unsigned long agora = micros();
  int leitura = digitalRead(IR_PINS[i]);
  bool cortadoAgora = IR_CORTADO_EM_HIGH[i] ? (leitura == HIGH) : (leitura == LOW);

  portENTER_CRITICAL_ISR(&mux);
  if (cortadoAgora != irCortado[i]) {
    irCortado[i] = cortadoAgora;
    if (cortadoAgora) {
      if (pendente[i] && (agora - liberaUs[i]) < GAP_UNIAO_US) {
        pendente[i] = false;
      } else {
        if (pendente[i]) fecharPulso(i);
        corteUs[i] = agora;
      }
    } else {
      liberaUs[i] = agora;
      pendente[i] = true;
    }
  }
  portEXIT_CRITICAL_ISR(&mux);
}

void IRAM_ATTR isr0() { tratarBorda(0); }
void IRAM_ATTR isr1() { tratarBorda(1); }
void IRAM_ATTR isr2() { tratarBorda(2); }

float lerDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 12000);
  if (duracao == 0) return -1;
  return duracao / 58.0;
}

bool estaOcupado(float d) {
  if (d <= 0) return false;
  return d < distanciaVazio - MARGEM_VAZIO_CM;
}

void atualizarUltrassonico() {
  float d = lerDistancia();
  bool ocupado = estaOcupado(d);

  if (estado == AVALIANDO && millis() - inicioAvaliacao >= ESPERA_US_MS) {
    if (ocupado) leiturasOcupadas++;
    else leiturasVazias++;
  }

  if (DEBUG_US && ocupado) {
    Serial.print("US: ");
    Serial.print(d);
    Serial.println(" cm");
  }
}

void calibrarVazio() {
  float leituras[20];
  int n = 0;
  for (int i = 0; i < 20; i++) {
    float d = lerDistancia();
    if (d > 0) leituras[n++] = d;
    delay(60);
  }
  if (n < 5) {
    Serial.println("Calibracao falhou, usando distancia padrao do gol vazio");
    return;
  }

  for (int i = 0; i < n - 1; i++)
    for (int j = i + 1; j < n; j++)
      if (leituras[j] < leituras[i]) { float t = leituras[i]; leituras[i] = leituras[j]; leituras[j] = t; }
  distanciaVazio = leituras[n / 2];
  Serial.print("Gol vazio calibrado em ");
  Serial.print(distanciaVazio);
  Serial.println(" cm");
}

void tocarSomAleatorio() {
  int som = random(1, TOTAL_SONS + 1);
  player.play(som);
  Serial.print("Tocando som ");
  Serial.println(som);
}

void registrarGol() {
  golsMs[contagem % MAX_GOLS] = millis();
  contagem++;
  ultimoCorte = millis();
  bloqueioAte = millis() + BLOQUEIO_MS;
  Serial.print("GOL confirmado! Numero ");
  Serial.println(contagem);
  tocarSomAleatorio();
}

void processarFeixes() {
  unsigned long agoraMs = millis();

  bool cortadoLocal[3];
  unsigned long corteLocal[3];
  bool pronto[3];
  unsigned long inicioLocal[3];
  unsigned long duracaoLocal[3];
  unsigned long agoraUs;

  portENTER_CRITICAL(&mux);
  agoraUs = micros();
  for (int i = 0; i < 3; i++) {

    if (pendente[i] && !irCortado[i] && (agoraUs - liberaUs[i]) >= GAP_UNIAO_US) fecharPulso(i);
    cortadoLocal[i] = irCortado[i];
    corteLocal[i] = corteUs[i];
    pronto[i] = pulsoPronto[i];
    inicioLocal[i] = inicioUs[i];
    duracaoLocal[i] = duracaoUs[i];
    pulsoPronto[i] = false;
  }
  portEXIT_CRITICAL(&mux);

  bool algumCortado = false;
  bool longoEmAndamento = false;
  for (int i = 0; i < 3; i++) {
    if (cortadoLocal[i]) {
      algumCortado = true;
      if (agoraUs - corteLocal[i] > DURACAO_BOLA_US) longoEmAndamento = true;
    }
  }
  cortado = algumCortado;

  bool pulsoCurto = false;
  bool pulsoLongo = false;
  for (int i = 0; i < 3; i++) {
    if (!pronto[i]) continue;
    if (MODO_CALIBRACAO) {
      Serial.print("Feixe ");
      Serial.print(i);
      Serial.print(": inicio_us=");
      Serial.print(inicioLocal[i]);
      Serial.print(", corte de ");
      Serial.print(duracaoLocal[i] / 1000.0);
      Serial.println(" ms");
    }
    if (duracaoLocal[i] <= DURACAO_BOLA_US) pulsoCurto = true;
    else pulsoLongo = true;
  }
  bool qualquerPulso = pulsoCurto || pulsoLongo;

  switch (estado) {
    case LIVRE:
      if (pulsoLongo) {

        silencioAte = agoraMs + SILENCIO_MS;
        estado = AGUARDANDO_LIVRE;
      } else if (pulsoCurto) {
        inicioAvaliacao = agoraMs;
        leiturasOcupadas = 0;
        leiturasVazias = 0;
        estado = AVALIANDO;
        Serial.println("Corte curto, avaliando...");
      }
      break;

    case AVALIANDO:
      if (pulsoLongo) {
        Serial.println("Descartado: corte longo (pessoa/objeto parado)");
        silencioAte = agoraMs + SILENCIO_MS;
        estado = AGUARDANDO_LIVRE;
        break;
      }
      if (pulsoCurto) {

        inicioAvaliacao = agoraMs;
        leiturasOcupadas = 0;
        leiturasVazias = 0;
        break;
      }
      if (agoraMs - inicioAvaliacao >= AVALIACAO_MS) {
        if (algumCortado) {
          if (longoEmAndamento) {
            Serial.println("Descartado: feixe continua cortado (pessoa)");
            silencioAte = agoraMs + SILENCIO_MS;
            estado = AGUARDANDO_LIVRE;
          }
          break;
        }
        Serial.print("Ultrassonico depois: ocupado=");
        Serial.print(leiturasOcupadas);
        Serial.print(" vazio=");
        Serial.println(leiturasVazias);
        if (USAR_US_NO_VETO && leiturasOcupadas > leiturasVazias) {
          Serial.println("Descartado: ainda tem algo no gol (pessoa)");
        } else {
          registrarGol();
        }
        estado = AGUARDANDO_LIVRE;
      }
      break;

    case AGUARDANDO_LIVRE:

      if (qualquerPulso || algumCortado) silencioAte = agoraMs + SILENCIO_MS;
      if (!algumCortado && agoraMs >= bloqueioAte && agoraMs >= silencioAte) estado = LIVRE;
      break;
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 3; i++) pinMode(IR_PINS[i], INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.print("Limite de bola (ms): ");
  Serial.println(DURACAO_BOLA_MS);

  Serial.println("Calibrando: deixe o gol vazio...");
  calibrarVazio();

  Serial2.begin(9600, SERIAL_8N1, -1, DFPLAYER_TX_PIN);
  delay(1000);
  player.begin(Serial2, false, false);
  player.volume(20);
  randomSeed(micros());

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SENHA);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! Abra no navegador: http://");
  Serial.println(WiFi.localIP());

  servidor.on("/", paginaInicial);
  servidor.on("/historico", paginaHistorico);
  servidor.on("/dados", enviarDados);
  servidor.on("/gols", enviarGols);
  servidor.on("/zerar", zerarPlacar);
  servidor.begin();

  Serial.print("Feixes no inicio: ");
  for (int i = 0; i < 3; i++) {
    int leitura = digitalRead(IR_PINS[i]);
    irCortado[i] = IR_CORTADO_EM_HIGH[i] ? (leitura == HIGH) : (leitura == LOW);
    Serial.print(i);
    Serial.print("=");
    Serial.print(irCortado[i] ? "cortado" : "livre");
    Serial.print(" ");
  }
  Serial.println();
  attachInterrupt(digitalPinToInterrupt(IR_PINS[0]), isr0, CHANGE);
  attachInterrupt(digitalPinToInterrupt(IR_PINS[1]), isr1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(IR_PINS[2]), isr2, CHANGE);
}

void loop() {
  servidor.handleClient();
  processarFeixes();

  if (millis() >= proximaLeitura) {
    atualizarUltrassonico();
    proximaLeitura = millis() + 60;
  }
}
