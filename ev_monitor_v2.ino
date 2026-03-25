#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "OPPO F21 Pro 5G";
const char* password = "12345678";

WebServer server(80);
Adafruit_INA219 ina219;

#define TEMP_PIN 4
#define MOTOR_VOLT_PIN 34
#define ENA 33
#define IN1 26
#define IN2 27

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

float voltage;
float current;
float temperature;
float motorVoltage;
float rpm;
int pwmValue = 0;
float MAX_RPM = 3500;
float VOLT_MIN = 10.0;
float VOLT_MAX = 16.8;

void handleRoot() {

  // Battery % from voltage (10V=0%, 16.8V=100%)
  float battPct = constrain((voltage - VOLT_MIN) / (VOLT_MAX - VOLT_MIN) * 100.0, 0, 100);
  bool tempWarn  = temperature >= 60.0;
  bool tempCrit  = temperature >= 75.0;
  bool battLow   = battPct <= 20.0;
  bool battCrit  = battPct <= 10.0;

  String page = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>EV Battery Monitor</title>
  <link href="https://fonts.googleapis.com/css2?family=Exo+2:ital,wght@0,300;0,600;0,900;1,300&family=Share+Tech+Mono&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg0:#020510; --bg1:#060c1f; --panel:rgba(8,14,38,0.88);
      --volt:#00f5d4; --amp:#ff9f1c; --temp:#ff3f6e;
      --mvolt:#b57bee; --rpm:#2de08e; --batt:#f9ca24;
      --warn:#ff9f1c; --crit:#ff3f6e;
      --white:#e8f0ff; --dim:rgba(232,240,255,0.32);
      --r:18px;
    }
    *,*::before,*::after{margin:0;padding:0;box-sizing:border-box;}

    body {
      font-family:'Exo 2',sans-serif;
      background:var(--bg0);
      color:var(--white);
      min-height:100vh;
      overflow-x:hidden;
    }

    /* ── Animated BG ── */
    .bg {
      position:fixed;inset:0;z-index:0;
      background:
        radial-gradient(ellipse 80% 55% at 10% 15%, rgba(0,245,212,.1) 0%,transparent 55%),
        radial-gradient(ellipse 55% 60% at 90% 10%, rgba(181,123,238,.1) 0%,transparent 55%),
        radial-gradient(ellipse 45% 45% at 50% 95%, rgba(255,63,110,.08) 0%,transparent 55%),
        radial-gradient(ellipse 60% 35% at 75% 55%, rgba(45,224,142,.06) 0%,transparent 55%);
      animation:bgFloat 10s ease-in-out infinite alternate;
    }
    @keyframes bgFloat{from{opacity:.6}to{opacity:1}}

    .grid-lines {
      position:fixed;inset:0;z-index:0;pointer-events:none;
      background-image:
        linear-gradient(rgba(0,245,212,.025) 1px,transparent 1px),
        linear-gradient(90deg,rgba(0,245,212,.025) 1px,transparent 1px);
      background-size:44px 44px;
      animation:gridDrift 20s linear infinite;
    }
    @keyframes gridDrift{from{background-position:0 0}to{background-position:44px 44px}}

    .scanlines {
      position:fixed;inset:0;z-index:300;pointer-events:none;
      background:repeating-linear-gradient(0deg,transparent,transparent 3px,rgba(0,0,0,.055) 3px,rgba(0,0,0,.055) 4px);
    }

    /* floating particles */
    .particles { position:fixed;inset:0;z-index:1;pointer-events:none;overflow:hidden; }
    .p {
      position:absolute; border-radius:50%;
      animation:floatUp linear infinite;
      opacity:0;
    }
    @keyframes floatUp {
      0%  { transform:translateY(110vh) scale(0); opacity:0; }
      10% { opacity:.7; }
      90% { opacity:.3; }
      100%{ transform:translateY(-10vh) scale(1.5); opacity:0; }
    }

    /* ── Layout ── */
    .wrap { position:relative;z-index:10;max-width:980px;margin:0 auto;padding:0 16px 36px; }

    /* ── Header ── */
    header { text-align:center; padding:30px 0 22px; }
    .sys-tag {
      display:inline-flex;align-items:center;gap:8px;
      background:rgba(0,245,212,.08);
      border:1px solid rgba(0,245,212,.22);
      border-radius:99px; padding:5px 20px;
      font-family:'Share Tech Mono',monospace;
      font-size:11px;letter-spacing:3px;color:var(--volt);
      text-transform:uppercase;margin-bottom:14px;
      animation:fadeSlideDown .6s ease both;
    }
    .live-dot { width:8px;height:8px;border-radius:50%;background:var(--volt);box-shadow:0 0 10px var(--volt);animation:blink 1.4s infinite; }
    @keyframes blink{0%,100%{opacity:1;transform:scale(1)}50%{opacity:.2;transform:scale(.6)}}

    h1 {
      font-size:clamp(22px,5.5vw,42px);font-weight:900;letter-spacing:5px;text-transform:uppercase;
      background:linear-gradient(110deg,var(--volt) 0%,#fff 35%,var(--mvolt) 65%,var(--amp) 100%);
      -webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text;
      filter:drop-shadow(0 0 22px rgba(0,245,212,.45));
      animation:fadeSlideDown .7s .1s ease both;
    }
    .subtitle {
      margin-top:8px;font-size:12px;letter-spacing:3px;color:var(--dim);
      text-transform:uppercase;font-weight:300;
      animation:fadeSlideDown .7s .2s ease both;
    }

    @keyframes fadeSlideDown { from{opacity:0;transform:translateY(-14px)} to{opacity:1;transform:translateY(0)} }
    @keyframes fadeSlideUp   { from{opacity:0;transform:translateY(14px)}  to{opacity:1;transform:translateY(0)} }
    @keyframes fadeIn        { from{opacity:0} to{opacity:1} }

    /* ── WARNING BANNER ── */
    .warn-banner {
      border-radius:14px; padding:13px 20px;
      display:flex;align-items:center;gap:12px;
      margin-bottom:14px; border:1px solid;
      animation:fadeSlideUp .4s ease both, warnPulse 1.8s ease-in-out infinite;
      font-size:13px;font-weight:600;letter-spacing:1.5px;text-transform:uppercase;
    }
    .warn-banner.hide { display:none; }
    .warn-banner.warn { background:rgba(255,159,28,.1);border-color:rgba(255,159,28,.4);color:var(--warn); }
    .warn-banner.crit { background:rgba(255,63,110,.12);border-color:rgba(255,63,110,.5);color:var(--crit); }
    @keyframes warnPulse { 0%,100%{box-shadow:0 0 0 0 transparent} 50%{box-shadow:0 0 18px 2px rgba(255,63,110,.25)} }
    .warn-icon { font-size:20px; animation:shake .4s ease-in-out infinite alternate; }
    @keyframes shake { from{transform:rotate(-8deg)} to{transform:rotate(8deg)} }

    /* ── Cards Grid ── */
    .cards {
      display:grid;
      grid-template-columns:repeat(auto-fit,minmax(152px,1fr));
      gap:13px; margin-bottom:14px;
    }

    .card {
      position:relative;
      background:var(--panel);
      border-radius:var(--r);
      padding:20px 16px 16px;
      border:1px solid rgba(255,255,255,.055);
      backdrop-filter:blur(16px);
      overflow:hidden;
      transition:transform .25s,box-shadow .25s;
      animation:cardIn .5s ease both;
    }
    .card:hover { transform:translateY(-6px) scale(1.018); box-shadow:0 18px 48px rgba(0,0,0,.5); }

    /* stagger */
    .card:nth-child(1){animation-delay:.15s}
    .card:nth-child(2){animation-delay:.22s}
    .card:nth-child(3){animation-delay:.29s}
    .card:nth-child(4){animation-delay:.36s}
    .card:nth-child(5){animation-delay:.43s}
    .card:nth-child(6){animation-delay:.50s}
    @keyframes cardIn { from{opacity:0;transform:translateY(22px)} to{opacity:1;transform:translateY(0)} }

    /* glowing top bar */
    .card::before {
      content:'';position:absolute;top:0;left:12%;right:12%;height:2px;
      border-radius:0 0 6px 6px;
      background:var(--c);box-shadow:0 0 22px 3px var(--c);
    }
    /* ambient blob */
    .card::after {
      content:'';position:absolute;top:-50px;right:-40px;
      width:130px;height:130px;border-radius:50%;
      background:var(--c);opacity:.07;filter:blur(38px);pointer-events:none;
    }
    .card .corner {
      position:absolute;bottom:0;left:0;width:32px;height:32px;
      border-left:2px solid var(--c);border-bottom:2px solid var(--c);
      border-radius:0 0 0 var(--r);opacity:.22;
    }

    .card.volt  {--c:var(--volt)}
    .card.amp   {--c:var(--amp)}
    .card.temp  {--c:var(--temp)}
    .card.mvolt {--c:var(--mvolt)}
    .card.rpm   {--c:var(--rpm)}
    .card.batt  {--c:var(--batt)}

    .card-lbl {
      font-size:9.5px;letter-spacing:2.5px;text-transform:uppercase;
      color:var(--c);font-weight:700;margin-bottom:9px;
      display:flex;align-items:center;gap:6px;
    }
    .card-lbl::after { content:'';flex:1;height:1px;background:var(--c);opacity:.18; }

    .card-icon { font-size:19px;margin-bottom:5px;filter:drop-shadow(0 0 6px var(--c)); }

    .card-val {
      font-family:'Share Tech Mono',monospace;
      font-size:clamp(26px,5vw,38px);font-weight:400;
      color:var(--c);line-height:1;
      text-shadow:0 0 22px var(--c),0 0 55px var(--c);
      letter-spacing:-1px;
    }
    .card-unit { font-size:10px;letter-spacing:2px;color:var(--dim);margin-top:4px;text-transform:uppercase; }

    /* SVG gauge */
    .gauge-wrap { margin-top:12px;display:flex;justify-content:center; }
    .gauge-svg  { width:100%;max-width:96px; }

    /* ── Battery Card special ── */
    .batt-body { margin-top:8px; }
    .batt-shell {
      position:relative;height:26px;border-radius:8px;
      border:2px solid rgba(249,202,36,.35);
      background:rgba(249,202,36,.06);
      overflow:hidden;
    }
    .batt-shell::after {
      content:'';position:absolute;right:-8px;top:50%;transform:translateY(-50%);
      width:6px;height:12px;border-radius:0 4px 4px 0;
      background:rgba(249,202,36,.35);
    }
    .batt-fill {
      position:absolute;inset:3px;border-radius:5px;
      background:linear-gradient(90deg,var(--batt-fill-a),var(--batt-fill-b));
      box-shadow:0 0 14px var(--batt-fill-a);
      transition:width .6s ease;
      animation:battShimmer 2.5s ease-in-out infinite;
    }
    @keyframes battShimmer {
      0%,100%{opacity:1}
      50%{opacity:.75}
    }
    .batt-segments {
      position:absolute;inset:0;display:flex;align-items:center;padding:0 4px;
      gap:4px;pointer-events:none;
    }
    .batt-seg { flex:1;height:60%;border-right:1px solid rgba(0,0,0,.25); }
    .batt-seg:last-child{border-right:none;}
    .batt-pct-label {
      position:absolute;inset:0;display:flex;align-items:center;justify-content:center;
      font-family:'Share Tech Mono',monospace;font-size:11px;font-weight:700;
      color:#fff;text-shadow:0 1px 4px rgba(0,0,0,.8);letter-spacing:1px;
    }
    .batt-status {
      display:flex;justify-content:space-between;align-items:center;
      margin-top:7px;
    }
    .batt-stat-txt { font-size:9px;letter-spacing:2px;text-transform:uppercase;color:var(--dim); }
    .batt-stat-val { font-family:'Share Tech Mono',monospace;font-size:12px;color:var(--batt); }

    /* low batt flicker */
    .batt-low .batt-fill { animation:lowFlicker 0.9s ease-in-out infinite; }
    @keyframes lowFlicker {0%,100%{opacity:1}40%{opacity:.3}60%{opacity:.8}}

    /* ── Temp warning glow on card ── */
    .card.temp.temp-warn  { box-shadow:0 0 0 2px rgba(255,159,28,.4),0 0 30px rgba(255,159,28,.15); }
    .card.temp.temp-crit  { animation:cardIn .5s ease both,critPulse 1s ease-in-out infinite; }
    @keyframes critPulse { 0%,100%{box-shadow:0 0 0 2px rgba(255,63,110,.5),0 0 40px rgba(255,63,110,.2)} 50%{box-shadow:0 0 0 4px rgba(255,63,110,.9),0 0 60px rgba(255,63,110,.4)} }

    /* ── Control Panel ── */
    .control {
      position:relative;background:var(--panel);border-radius:20px;
      padding:26px 24px 24px;
      border:1px solid rgba(255,255,255,.055);
      backdrop-filter:blur(16px);overflow:hidden;
      animation:fadeSlideUp .6s .55s ease both;
    }
    .control::before {
      content:'';position:absolute;top:0;left:0;right:0;height:2px;
      background:linear-gradient(90deg,var(--volt),var(--mvolt) 50%,var(--rpm));
      box-shadow:0 0 20px rgba(0,245,212,.5);
    }
    .control::after {
      content:'';position:absolute;bottom:-90px;right:-90px;
      width:220px;height:220px;border-radius:50%;
      background:var(--rpm);opacity:.04;filter:blur(55px);
    }

    .ctrl-head {
      display:flex;align-items:center;justify-content:space-between;margin-bottom:22px;
    }
    .ctrl-title {
      font-family:'Share Tech Mono',monospace;font-size:12px;
      letter-spacing:3px;text-transform:uppercase;color:var(--dim);
    }
    .ctrl-badge {
      font-family:'Share Tech Mono',monospace;font-size:10px;letter-spacing:2px;
      color:var(--rpm);background:rgba(45,224,142,.1);
      border:1px solid rgba(45,224,142,.28);border-radius:99px;padding:3px 14px;
      animation:badgePulse 2s ease-in-out infinite;
    }
    @keyframes badgePulse{0%,100%{opacity:1}50%{opacity:.5}}

    .throttle-display { text-align:center;margin-bottom:22px; }
    .throttle-ring {
      position:relative;display:inline-flex;
      align-items:center;justify-content:center;
      width:130px;height:130px;margin-bottom:6px;
    }
    .throttle-ring svg { position:absolute;inset:0;width:100%;height:100%;transform:rotate(-90deg); }
    .ring-bg   { stroke:rgba(255,255,255,.06);fill:none;stroke-width:10;stroke-linecap:round; }
    .ring-fill {
      fill:none;stroke-width:10;stroke-linecap:round;
      stroke:url(#rg);
      stroke-dasharray:339.3;
      transition:stroke-dashoffset .5s cubic-bezier(.4,0,.2,1);
    }
    .throttle-num {
      font-family:'Share Tech Mono',monospace;
      font-size:38px;font-weight:400;
      color:var(--rpm);
      text-shadow:0 0 28px var(--rpm),0 0 70px rgba(45,224,142,.35);
      position:relative;z-index:1;
    }
    .throttle-lbl {
      font-size:9px;letter-spacing:3px;text-transform:uppercase;color:var(--dim);margin-top:2px;
    }

    .slider-track { display:flex;align-items:center;gap:14px; }
    .slider-end { font-family:'Share Tech Mono',monospace;font-size:11px;color:var(--dim);min-width:26px;text-align:center; }

    input[type=range] {
      flex:1;-webkit-appearance:none;height:8px;border-radius:8px;
      background:linear-gradient(90deg,rgba(45,224,142,.18),rgba(0,245,212,.22) 50%,rgba(255,63,110,.22));
      outline:none;cursor:pointer;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance:none;width:28px;height:28px;border-radius:50%;
      background:radial-gradient(circle at 35% 35%,#fff,var(--rpm));
      box-shadow:0 0 18px var(--rpm),0 0 45px rgba(45,224,142,.35),0 2px 8px rgba(0,0,0,.6);
      cursor:pointer;transition:transform .12s;
    }
    input[type=range]:active::-webkit-slider-thumb{transform:scale(1.3)}

    .pwm-row {
      display:flex;align-items:center;justify-content:space-between;
      margin-top:16px;padding-top:16px;
      border-top:1px solid rgba(255,255,255,.05);
    }
    .pwm-kv{display:flex;flex-direction:column;gap:2px;}
    .pwm-k{font-size:9px;letter-spacing:2px;text-transform:uppercase;color:var(--dim);}
    .pwm-v{font-family:'Share Tech Mono',monospace;font-size:22px;color:var(--volt);text-shadow:0 0 12px var(--volt);}

    /* ── Footer ── */
    footer {
      text-align:center;margin-top:24px;
      font-family:'Share Tech Mono',monospace;font-size:10px;
      letter-spacing:3px;color:rgba(232,240,255,.14);text-transform:uppercase;
    }
    footer span{color:rgba(232,240,255,.22);}
  </style>
</head>
<body>

<div class="bg"></div>
<div class="grid-lines"></div>
<div class="scanlines"></div>
<div class="particles" id="particles"></div>

<div class="wrap">

  <!-- Header -->
  <header>
    <div class="sys-tag"><span class="live-dot"></span> System Online</div>
    <h1>EV Battery Monitor</h1>
    <p class="subtitle">ESP32 · Live Telemetry · Real-Time Control</p>
  </header>

  <!-- Warnings -->)rawhtml";

  // Temperature warning banner
  if (tempCrit) {
    page += R"rawhtml(
  <div class="warn-banner crit">
    <span class="warn-icon">🔥</span>
    <span>CRITICAL: Battery temperature )rawhtml";
    page += String(temperature, 1);
    page += R"rawhtml(°C — OVERHEAT RISK! Reduce load immediately!</span>
  </div>)rawhtml";
  } else if (tempWarn) {
    page += R"rawhtml(
  <div class="warn-banner warn">
    <span class="warn-icon">⚠</span>
    <span>WARNING: High temperature )rawhtml";
    page += String(temperature, 1);
    page += R"rawhtml(°C — Monitor closely</span>
  </div>)rawhtml";
  }

  // Battery low banner
  if (battCrit) {
    page += R"rawhtml(
  <div class="warn-banner crit">
    <span class="warn-icon">🪫</span>
    <span>CRITICAL: Battery at )rawhtml";
    page += String(battPct, 0);
    page += R"rawhtml(% — Charge immediately!</span>
  </div>)rawhtml";
  } else if (battLow) {
    page += R"rawhtml(
  <div class="warn-banner warn">
    <span class="warn-icon">⚡</span>
    <span>LOW BATTERY: )rawhtml";
    page += String(battPct, 0);
    page += R"rawhtml(% remaining — Please charge soon</span>
  </div>)rawhtml";
  }

  page += R"rawhtml(

  <!-- Cards -->
  <div class="cards">

    <!-- BATTERY % -->
    <div class="card batt)rawhtml";
  if (battCrit || battLow) page += " batt-low";
  page += R"rawhtml(">
      <div class="card-lbl">Battery</div>
      <div class="card-icon">🔋</div>
      <div class="card-val">)rawhtml";
  page += String(battPct, 0);
  page += R"rawhtml(</div>
      <div class="card-unit">Percent</div>
      <div class="batt-body">
        <div class="batt-shell">
          <div class="batt-fill" style=")rawhtml";
  // Color gradient based on level
  if (battPct > 50) {
    page += "--batt-fill-a:#2de08e;--batt-fill-b:#f9ca24;";
  } else if (battPct > 20) {
    page += "--batt-fill-a:#f9ca24;--batt-fill-b:#ff9f1c;";
  } else {
    page += "--batt-fill-a:#ff3f6e;--batt-fill-b:#ff9f1c;";
  }
  page += "width:";
  page += String(battPct, 0);
  page += R"rawhtml(%"></div>
          <div class="batt-segments">
            <div class="batt-seg"></div><div class="batt-seg"></div>
            <div class="batt-seg"></div><div class="batt-seg"></div>
          </div>
          <div class="batt-pct-label">)rawhtml";
  page += String(battPct, 0);
  page += R"rawhtml(%</div>
        </div>
        <div class="batt-status">
          <span class="batt-stat-txt">)rawhtml";
  if (battCrit)     page += "CRITICAL";
  else if (battLow) page += "LOW";
  else if (battPct > 70) page += "GOOD";
  else              page += "NORMAL";
  page += R"rawhtml(</span>
          <span class="batt-stat-val">)rawhtml";
  page += String(voltage, 2);
  page += R"rawhtml( V</span>
        </div>
      </div>
      <div class="corner"></div>
    </div>

    <!-- VOLTAGE -->
    <div class="card volt">
      <div class="card-lbl">Battery V</div>
      <div class="card-icon">⚡</div>
      <div class="card-val">)rawhtml";
  page += String(voltage, 2);
  page += R"rawhtml(</div>
      <div class="card-unit">Volts</div>
      <div class="gauge-wrap">
        <svg class="gauge-svg" viewBox="0 0 100 60">
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="rgba(255,255,255,.06)" stroke-width="9" stroke-linecap="round"/>
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="url(#gv)" stroke-width="9" stroke-linecap="round"
            stroke-dasharray="125.6" stroke-dashoffset=")rawhtml";
  page += String(125.6 - constrain(voltage / 16.8, 0, 1) * 125.6, 1);
  page += R"rawhtml("/>
          <defs><linearGradient id="gv" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#00f5d4"/><stop offset="100%" stop-color="#00c8ff"/>
          </linearGradient></defs>
        </svg>
      </div>
      <div class="corner"></div>
    </div>

    <!-- CURRENT -->
    <div class="card amp">
      <div class="card-lbl">Current</div>
      <div class="card-icon">〜</div>
      <div class="card-val">)rawhtml";
  page += String(current, 2);
  page += R"rawhtml(</div>
      <div class="card-unit">Amps</div>
      <div class="gauge-wrap">
        <svg class="gauge-svg" viewBox="0 0 100 60">
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="rgba(255,255,255,.06)" stroke-width="9" stroke-linecap="round"/>
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="url(#ga)" stroke-width="9" stroke-linecap="round"
            stroke-dasharray="125.6" stroke-dashoffset=")rawhtml";
  page += String(125.6 - constrain(abs(current) / 10.0, 0, 1) * 125.6, 1);
  page += R"rawhtml("/>
          <defs><linearGradient id="ga" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#ff9f1c"/><stop offset="100%" stop-color="#ff6b35"/>
          </linearGradient></defs>
        </svg>
      </div>
      <div class="corner"></div>
    </div>

    <!-- TEMP -->
    <div class="card temp)rawhtml";
  if (tempCrit) page += " temp-crit";
  else if (tempWarn) page += " temp-warn";
  page += R"rawhtml(">
      <div class="card-lbl">Temperature</div>
      <div class="card-icon">)rawhtml";
  if (tempCrit) page += "🔥";
  else if (tempWarn) page += "⚠";
  else page += "🌡";
  page += R"rawhtml(</div>
      <div class="card-val" style=")rawhtml";
  if (tempCrit)     page += "color:var(--crit);text-shadow:0 0 22px var(--crit),0 0 55px var(--crit)";
  else if (tempWarn) page += "color:var(--warn);text-shadow:0 0 22px var(--warn),0 0 55px var(--warn)";
  else page += "";
  page += R"rawhtml(">)rawhtml";
  page += String(temperature, 1);
  page += R"rawhtml(</div>
      <div class="card-unit">°Celsius · )rawhtml";
  if (tempCrit) page += "CRITICAL";
  else if (tempWarn) page += "WARNING";
  else page += "Normal";
  page += R"rawhtml(</div>
      <div class="gauge-wrap">
        <svg class="gauge-svg" viewBox="0 0 100 60">
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="rgba(255,255,255,.06)" stroke-width="9" stroke-linecap="round"/>
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="url(#gt)" stroke-width="9" stroke-linecap="round"
            stroke-dasharray="125.6" stroke-dashoffset=")rawhtml";
  page += String(125.6 - constrain(temperature / 80.0, 0, 1) * 125.6, 1);
  page += R"rawhtml("/>
          <defs><linearGradient id="gt" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#ff3f6e"/><stop offset="100%" stop-color="#ff9f1c"/>
          </linearGradient></defs>
        </svg>
      </div>
      <div class="corner"></div>
    </div>

    <!-- MOTOR VOLTAGE -->
    <div class="card mvolt">
      <div class="card-lbl">Motor V</div>
      <div class="card-icon">⚙</div>
      <div class="card-val">)rawhtml";
  page += String(motorVoltage, 2);
  page += R"rawhtml(</div>
      <div class="card-unit">Volts</div>
      <div class="gauge-wrap">
        <svg class="gauge-svg" viewBox="0 0 100 60">
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="rgba(255,255,255,.06)" stroke-width="9" stroke-linecap="round"/>
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="url(#gm)" stroke-width="9" stroke-linecap="round"
            stroke-dasharray="125.6" stroke-dashoffset=")rawhtml";
  page += String(125.6 - constrain(motorVoltage / 12.0, 0, 1) * 125.6, 1);
  page += R"rawhtml("/>
          <defs><linearGradient id="gm" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#b57bee"/><stop offset="100%" stop-color="#7c4dff"/>
          </linearGradient></defs>
        </svg>
      </div>
      <div class="corner"></div>
    </div>

    <!-- RPM -->
    <div class="card rpm">
      <div class="card-lbl">Est. RPM</div>
      <div class="card-icon">↻</div>
      <div class="card-val">)rawhtml";
  page += String((int)rpm);
  page += R"rawhtml(</div>
      <div class="card-unit">RPM</div>
      <div class="gauge-wrap">
        <svg class="gauge-svg" viewBox="0 0 100 60">
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="rgba(255,255,255,.06)" stroke-width="9" stroke-linecap="round"/>
          <path d="M10 55 A40 40 0 0 1 90 55" fill="none" stroke="url(#gr)" stroke-width="9" stroke-linecap="round"
            stroke-dasharray="125.6" stroke-dashoffset=")rawhtml";
  page += String(125.6 - constrain(rpm / 3500.0, 0, 1) * 125.6, 1);
  page += R"rawhtml("/>
          <defs><linearGradient id="gr" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stop-color="#2de08e"/><stop offset="100%" stop-color="#00f5d4"/>
          </linearGradient></defs>
        </svg>
      </div>
      <div class="corner"></div>
    </div>

  </div><!-- /cards -->

  <!-- Control Panel -->
  <div class="control">
    <div class="ctrl-head">
      <div class="ctrl-title">⚙ Motor Speed Control</div>
      <div class="ctrl-badge" id="motor-status">ACTIVE</div>
    </div>

    <div class="throttle-display">
      <div class="throttle-ring">
        <svg viewBox="0 0 120 120">
          <defs>
            <linearGradient id="rg" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" stop-color="#2de08e"/>
              <stop offset="50%" stop-color="#00f5d4"/>
              <stop offset="100%" stop-color="#ff9f1c"/>
            </linearGradient>
          </defs>
          <circle class="ring-bg" cx="60" cy="60" r="54"/>
          <circle class="ring-fill" id="ring-arc" cx="60" cy="60" r="54"
            stroke-dashoffset=")rawhtml";
  page += String(339.3 - (pwmValue / 255.0) * 339.3, 1);
  page += R"rawhtml("/>
        </svg>
        <div style="text-align:center">
          <div class="throttle-num" id="pct-display">)rawhtml";
  page += String((int)(pwmValue / 255.0 * 100));
  page += R"rawhtml(%</div>
          <div class="throttle-lbl">Throttle</div>
        </div>
      </div>
    </div>

    <div class="slider-track">
      <span class="slider-end">0</span>
      <input type="range" min="0" max="255" value=")rawhtml";
  page += String(pwmValue);
  page += R"rawhtml(" oninput="setSpeed(this.value)" id="speedSlider">
      <span class="slider-end">255</span>
    </div>

    <div class="pwm-row">
      <div class="pwm-kv">
        <div class="pwm-k">PWM Value</div>
        <div class="pwm-v" id="pwm-val">)rawhtml";
  page += String(pwmValue);
  page += R"rawhtml(</div>
      </div>
      <div class="pwm-kv" style="text-align:right">
        <div class="pwm-k">Duty Cycle</div>
        <div class="pwm-v" id="duty-val" style="color:var(--amp);text-shadow:0 0 12px var(--amp)">)rawhtml";
  page += String((int)(pwmValue / 255.0 * 100));
  page += R"rawhtml(%</div>
      </div>
    </div>
  </div>

  <footer>ESP32 <span>·</span> INA219 <span>·</span> DS18B20 <span>·</span> L298N</footer>
</div>

<script>
  // Particles
  (function(){
    const colors=['#00f5d4','#b57bee','#ff3f6e','#2de08e','#ff9f1c'];
    const container=document.getElementById('particles');
    for(let i=0;i<28;i++){
      const p=document.createElement('div');
      p.className='p';
      const size=Math.random()*3+1.5;
      p.style.cssText=`
        left:${Math.random()*100}%;
        width:${size}px;height:${size}px;
        background:${colors[Math.floor(Math.random()*colors.length)]};
        box-shadow:0 0 ${size*3}px ${colors[Math.floor(Math.random()*colors.length)]};
        animation-duration:${6+Math.random()*10}s;
        animation-delay:${Math.random()*8}s;
      `;
      container.appendChild(p);
    }
  })();

  // Speed control
  function setSpeed(val){
    fetch('/setSpeed?value='+val);
    const pct=Math.round(val/255*100);
    document.getElementById('pwm-val').textContent=val;
    document.getElementById('pct-display').textContent=pct+'%';
    document.getElementById('duty-val').textContent=pct+'%';
    // update ring
    const arc=document.getElementById('ring-arc');
    arc.style.strokeDashoffset=(339.3-(val/255)*339.3).toFixed(1);
    // update badge
    const badge=document.getElementById('motor-status');
    badge.textContent=val==0?'STOPPED':'ACTIVE';
  }

  // Countdown timer with animated dots
  let countdown=2;
  setInterval(()=>{
    countdown--;
    if(countdown<=0) location.reload();
  },1000);

  setTimeout(()=>location.reload(), 2000);
</script>
</body>
</html>
)rawhtml";

  server.send(200, "text/html", page);
}

void setSpeed() {
  if (server.hasArg("value")) {
    pwmValue = server.arg("value").toInt();
    ledcWrite(ENA, pwmValue);
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (!ina219.begin()) { Serial.println("INA219 not found"); while(1); }
  Serial.println("INA219 OK");
  sensors.begin();
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  ledcAttach(ENA, 5000, 8);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nIP: " + WiFi.localIP().toString());
  server.on("/", handleRoot);
  server.on("/setSpeed", setSpeed);
  server.begin();
}

void loop() {
  voltage = ina219.getBusVoltage_V();
  current = ina219.getCurrent_mA() / 1000.0;
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
  int adc = analogRead(MOTOR_VOLT_PIN);
  motorVoltage = (adc / 4095.0) * 3.3 * 4;
  rpm = (motorVoltage / 12.0) * MAX_RPM;

  Serial.printf("V:%.2f  I:%.2fA  T:%.1fC  RPM:%.0f\n", voltage, current, temperature, rpm);
  server.handleClient();
  delay(1000);
}
