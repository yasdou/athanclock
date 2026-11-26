// web_page.h
// Die Einstellungsseite. Liegt im Flash (PROGMEM) und wird mit send_P()
// direkt von dort gesendet -> braucht praktisch keinen RAM.
// Die Daten holt die Seite per JavaScript von /api/state (siehe html.cpp).
//
// Zum Bearbeiten einfach den HTML-Teil zwischen den rawliteral-Markern ändern.
#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

static const char WEB_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="light dark">
<title>Athan Clock</title>
<style>
:root{--bg:#f3f0e8;--card:#fffdf8;--text:#1d2a23;--muted:#66756c;--line:#e4ded1;--accent:#1f6f54;--on-accent:#fff;--soft:#e4efe9;--hero:#1f6f54;--on-hero:#fff;--danger:#b3261e;--r:16px}
@media (prefers-color-scheme:dark){:root{--bg:#101512;--card:#18201c;--text:#e6ede8;--muted:#93a49a;--line:#28322c;--accent:#5cc79d;--on-accent:#0c1a13;--soft:#1f332a;--hero:#173a2c;--on-hero:#e6ede8;--danger:#ff8a80}}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{margin:0;background:var(--bg);color:var(--text);font:16px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
main{max-width:560px;margin:0 auto;padding:16px 16px 48px}
.hero{background:var(--hero);color:var(--on-hero);border-radius:var(--r);padding:22px 18px 20px;text-align:center}
.city{font-size:.95rem;opacity:.85;letter-spacing:.02em}
.clock{font-size:3.4rem;font-weight:700;line-height:1.1;margin:6px 0 2px;font-variant-numeric:tabular-nums}
.clock small{font-size:1.4rem;font-weight:500;opacity:.7}
.date{opacity:.85}
.next{margin-top:14px;padding-top:12px;border-top:1px solid rgba(255,255,255,.2);font-size:1.05rem}
.next b{font-variant-numeric:tabular-nums}
.card{background:var(--card);border:1px solid var(--line);border-radius:var(--r);padding:16px;margin-top:16px}
h2{margin:0 0 10px;font-size:.8rem;font-weight:650;letter-spacing:.08em;text-transform:uppercase;color:var(--muted)}
.grid{--cols:1fr auto 52px 92px;--gap:10px}
.head,.prow{display:grid;grid-template-columns:var(--cols);column-gap:var(--gap);align-items:center}
.head{padding-bottom:4px}
.head>span{font-size:.75rem;color:var(--muted)}
.head>span:nth-child(n+3){text-align:center}
.prow{margin:0 -8px;padding:9px 8px;border-top:1px solid var(--line);border-radius:10px}
.prow.next{background:var(--soft);border-top-color:transparent}
.prow.next+.prow{border-top-color:transparent}
.pname{font-weight:600}
.tmr{display:block;font-size:.7rem;line-height:1;color:var(--muted)}
.ptime{font-size:1.1rem;font-variant-numeric:tabular-nums;text-align:right}
.cell{display:flex;justify-content:center}
.none{color:var(--muted);text-align:center}
select,input[type=text],input[type=search]{width:100%;min-height:40px;padding:7px 10px;font:inherit;color:inherit;background:var(--bg);border:1px solid var(--line);border-radius:10px}
.grid select{padding:6px 4px;font-size:.9rem}
.switch{position:relative;display:inline-block;width:44px;height:26px}
.switch input{opacity:0;width:0;height:0}
.slider{position:absolute;inset:0;background:var(--line);border-radius:26px;transition:.2s;cursor:pointer}
.slider:before{content:"";position:absolute;width:20px;height:20px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.2s;box-shadow:0 1px 2px rgba(0,0,0,.3)}
.switch input:checked+.slider{background:var(--accent)}
.switch input:checked+.slider:before{transform:translateX(18px)}
.switch input:focus-visible+.slider{outline:2px solid var(--accent);outline-offset:2px}
.row{display:flex;gap:8px;align-items:center;margin-bottom:12px}
.row:last-child{margin-bottom:0}
.row>label{flex:0 0 96px;color:var(--muted)}
.row>select,.row>input{flex:1;min-width:0}
button{min-height:40px;padding:8px 14px;font:inherit;color:var(--text);background:var(--soft);border:0;border-radius:10px;cursor:pointer;white-space:nowrap}
button.primary{background:var(--accent);color:var(--on-accent);font-weight:600;width:100%}
button:disabled{opacity:.6;cursor:default}
input[type=range]{flex:1;accent-color:var(--accent)}
.vol{min-width:2.2em;text-align:right;font-variant-numeric:tabular-nums}
.hint{margin:8px 0 0;font-size:.85rem;color:var(--muted)}
.hint:empty{display:none}
.current{margin:0 0 12px}
.grow{flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
a.btn{display:inline-flex;align-items:center;min-height:40px;padding:8px 14px;border-radius:10px;background:var(--soft);color:var(--text);text-decoration:none;white-space:nowrap}
code{font-size:.85em}
.orow{display:grid;grid-template-columns:1fr 40px 72px 40px;gap:8px;align-items:center;padding:6px 0}
.orow+.orow{border-top:1px solid var(--line)}
.orow button{min-height:36px;padding:0;font-size:1.2rem;line-height:1}
.oval{text-align:center;font-variant-numeric:tabular-nums}
.oval.changed{color:var(--accent);font-weight:600}
.results{list-style:none;margin:-4px 0 4px;padding:0;border:1px solid var(--line);border-radius:10px;overflow:hidden}
.results li+li{border-top:1px solid var(--line)}
.results button{display:block;width:100%;min-height:44px;padding:8px 12px;text-align:left;background:transparent;border-radius:0;white-space:normal}
.results button:hover:not(:disabled),.results button:focus-visible{background:var(--soft);outline:none}
.results small{display:block;color:var(--muted);font-size:.8rem}
.results button:disabled{opacity:.5}
.picked{color:var(--accent);font-weight:600}
.meta{margin-top:18px;font-size:.8rem;color:var(--muted);text-align:center}
.offline{display:none;margin-top:16px;padding:12px;border-radius:12px;background:var(--danger);color:#fff;text-align:center}
.toast{position:fixed;left:50%;bottom:20px;transform:translate(-50%,20px);width:max-content;max-width:min(90vw,420px);padding:10px 16px;border-radius:12px;box-shadow:0 4px 16px rgba(0,0,0,.2);background:var(--text);color:var(--bg);opacity:0;transition:.25s;pointer-events:none;text-align:center}
.toast.show{opacity:1;transform:translate(-50%,0)}
.toast.err{background:var(--danger);color:#fff}
@media (max-width:380px){.grid{--cols:1fr auto 48px 80px;--gap:6px}.clock{font-size:2.8rem}}
</style>
</head>
<body>
<main>
  <section class="hero">
    <div class="city" id="city">Athan Clock</div>
    <div class="clock" id="clock">--:--</div>
    <div class="date" id="date">&nbsp;</div>
    <div class="next" id="next">Verbinde mit der Uhr&hellip;</div>
  </section>

  <div class="offline" id="offline">Keine Verbindung zur Uhr. Neuer Versuch l&auml;uft&hellip;</div>

  <section class="card">
    <h2>N&auml;chste Gebetszeiten</h2>
    <div class="grid" id="prayers">
      <div class="head"><span></span><span></span><span>Athan</span><span>Erinnerung</span></div>
    </div>
    <p class="hint">Erinnerung: Ton und Countdown auf dem Display vor dem Gebet.</p>
  </section>

  <section class="card">
    <h2>Feinabstimmung</h2>
    <div class="offsets" id="offsets"></div>
    <p class="hint">Verschiebt einzelne Zeiten um bis zu 30 Minuten, z. B. damit sie zum Kalender deiner Moschee passen. Gilt f&uuml;r Anzeige, Erinnerung und Athan.</p>
  </section>

  <section class="card">
    <h2>Ton &amp; Lautst&auml;rke</h2>
    <div class="row">
      <label for="athanTone">Athan</label>
      <select id="athanTone"></select>
      <button type="button" data-preview="athan" aria-label="Athan probeh&ouml;ren">&#9654;</button>
    </div>
    <div class="row">
      <label for="fajrTone">Fajr-Athan</label>
      <select id="fajrTone"></select>
      <button type="button" data-preview="fajr" aria-label="Fajr-Athan probeh&ouml;ren">&#9654;</button>
    </div>
    <div class="row">
      <label for="reminderTone">Erinnerung</label>
      <select id="reminderTone"></select>
      <button type="button" data-preview="reminder" aria-label="Erinnerung probeh&ouml;ren">&#9654;</button>
    </div>
    <div class="row">
      <label for="volume">Lautst&auml;rke</label>
      <input type="range" id="volume" min="0" max="30" step="1">
      <span class="vol" id="volumeValue">&ndash;</span>
    </div>
    <div class="row">
      <label for="fajrVolume">Fajr (Nacht)</label>
      <input type="range" id="fajrVolume" min="0" max="30" step="1">
      <span class="vol" id="fajrVolumeValue">&ndash;</span>
    </div>
    <p class="hint" id="fajrHint">Die Fajr-Lautst&auml;rke gilt f&uuml;r Fajr-Athan und die Erinnerung davor.</p>
    <div class="row">
      <label></label>
      <button type="button" id="stop">&#9632; Wiedergabe stoppen</button>
    </div>
  </section>

  <section class="card">
    <h2>Ort &amp; Berechnung</h2>
    <p class="current">Aktuell: <b id="curLoc">&ndash;</b></p>
    <form id="locationForm" autocomplete="off">
      <div class="row">
        <label for="cityInput">Ort suchen</label>
        <input type="search" id="cityInput" maxlength="60" placeholder="z. B. Bad Kreuznach"
               role="combobox" aria-autocomplete="list" aria-controls="results" aria-expanded="false">
      </div>
      <ul class="results" id="results" role="listbox" aria-label="Suchergebnisse" hidden></ul>
      <p class="hint" id="searchHint" aria-live="polite"></p>
      <div class="row">
        <label for="method">Methode</label>
        <select id="method"></select>
      </div>
      <div class="row">
        <label for="school">Asr</label>
        <select id="school">
          <option value="0">Standard</option>
          <option value="1">Hanafi</option>
        </select>
      </div>
      <button type="submit" class="primary" id="locationSave">Speichern &amp; Zeiten laden</button>
      <p class="hint">Asr &bdquo;Standard&ldquo; gilt f&uuml;r die Rechtsschulen Schafi&#39;i, Maliki und Hanbali; bei &bdquo;Hanafi&ldquo; ist Asr etwas sp&auml;ter.</p>
      <p class="hint">Ortssuche: Open-Meteo (OpenStreetMap/GeoNames). Gebetszeiten: aladhan.com. Das Laden dauert einige Sekunden.</p>
    </form>
  </section>

  <section class="card">
    <h2>System</h2>
    <div class="row">
      <label>WLAN</label>
      <span id="wifiName" class="grow">&ndash;</span>
      <a class="btn" href="/wifi">&Auml;ndern</a>
    </div>
    <div class="row">
      <label>Firmware</label>
      <span id="version" class="grow">&ndash;</span>
      <a class="btn" href="/update">Update</a>
    </div>
    <p class="hint">F&uuml;r Update und WLAN-Wechsel braucht es das Ger&auml;te-Passwort (Men&uuml; der Uhr &rarr; Info). Beim Update: Benutzer <b>admin</b>, Datei <code>firmware.bin</code> aus PlatformIO.</p>
  </section>

  <div class="meta" id="meta"></div>
</main>
<div class="toast" id="toast" role="status" aria-live="polite"></div>

<script>
const NAMES = ["Fajr", "Shuruk", "Dhuhr", "Asr", "Maghrib", "Isha"];
const SHURUK = 1;
const TONES = 6;
const WEEKDAYS = ["Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"];
const METHODS = [
  [13, "Diyanet (Türkei)"], [3, "Muslim World League"], [2, "ISNA (Nordamerika)"],
  [12, "UOIF (Frankreich)"], [5, "Ägyptische Behörde"], [4, "Umm al-Qura (Mekka)"],
  [1, "Karachi"], [15, "Moonsighting Committee"], [8, "Golfregion"], [9, "Kuwait"],
  [10, "Katar"], [16, "Dubai"], [7, "Teheran"], [0, "Schia Ithna-Ashari"]
];
const REMINDER_OPTIONS = [[0, "Aus"], [1, "15 min"], [2, "30 min"]];
// Zeitzonen mit denselben Regeln wie die Uhr (MEZ/MESZ). Die Uhr prüft das zusätzlich selbst.
const CET_ZONES = ["Europe/Berlin", "Europe/Busingen", "Europe/Vienna", "Europe/Zurich", "Europe/Vaduz",
  "Europe/Paris", "Europe/Monaco", "Europe/Luxembourg", "Europe/Brussels", "Europe/Amsterdam",
  "Europe/Copenhagen", "Europe/Oslo", "Europe/Stockholm", "Europe/Rome", "Europe/Vatican",
  "Europe/San_Marino", "Europe/Malta", "Europe/Madrid", "Europe/Andorra", "Europe/Gibraltar",
  "Europe/Prague", "Europe/Bratislava", "Europe/Warsaw", "Europe/Budapest", "Europe/Ljubljana",
  "Europe/Zagreb", "Europe/Sarajevo", "Europe/Belgrade", "Europe/Podgorica", "Europe/Skopje",
  "Europe/Tirane", "Arctic/Longyearbyen", "Africa/Ceuta", "CET"];
const GEOCODER = "https://geocoding-api.open-meteo.com/v1/search?count=8&language=de&format=json&name=";

const $ = (id) => document.getElementById(id);
let state = null;
let syncedAt = 0;
let toastTimer = null;
let rows = [];
let offsetRows = [];
let offsets = [0, 0, 0, 0, 0, 0];
let offsetTimer = null;
const MAX_OFFSET = 30;

function fmt(min) {
  if (min == null || min < 0) return "--:--";
  return String(Math.floor(min / 60)).padStart(2, "0") + ":" + String(min % 60).padStart(2, "0");
}

function toast(msg, isError) {
  const t = $("toast");
  t.textContent = msg;
  t.className = "toast show" + (isError ? " err" : "");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { t.className = "toast" + (isError ? " err" : ""); }, isError ? 4500 : 1800);
}

async function api(path, body) {
  const opts = body === undefined ? {} :
    { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(body) };
  const res = await fetch(path, opts);
  let data = {};
  try { data = await res.json(); } catch (e) {}
  if (!res.ok) throw new Error(data.error || ("Fehler " + res.status));
  return data;
}

function option(value, label) {
  const o = document.createElement("option");
  o.value = value;
  o.textContent = label;
  return o;
}

function buildStatic() {
  for (let i = 0; i < TONES; i++) {
    $("athanTone").appendChild(option(i, "Ton " + (i + 1)));
    $("fajrTone").appendChild(option(i, "Ton " + (i + 1)));
    $("reminderTone").appendChild(option(i, "Ton " + (i + 1)));
  }
  METHODS.forEach(([v, label]) => $("method").appendChild(option(v, label)));

  const grid = $("prayers");
  NAMES.forEach((name, i) => {
    const row = document.createElement("div");
    row.className = "prow";

    const n = document.createElement("div");
    n.className = "pname";
    n.textContent = name;
    const t = document.createElement("div");
    t.className = "ptime";
    t.textContent = "--:--";

    const a = document.createElement("div");
    a.className = "cell";
    let athan = null;
    if (i === SHURUK) {
      a.className = "none";
      a.textContent = "–";
      a.title = "Sonnenaufgang: kein Athan";
    } else {
      const sw = document.createElement("label");
      sw.className = "switch";
      athan = document.createElement("input");
      athan.type = "checkbox";
      athan.setAttribute("aria-label", "Athan " + name);
      athan.addEventListener("change", saveModes);
      const sl = document.createElement("span");
      sl.className = "slider";
      sw.append(athan, sl);
      a.appendChild(sw);
    }

    const r = document.createElement("div");
    const sel = document.createElement("select");
    sel.setAttribute("aria-label", "Erinnerung " + name);
    REMINDER_OPTIONS.forEach(([v, label]) => sel.appendChild(option(v, label)));
    sel.addEventListener("change", saveModes);
    r.appendChild(sel);

    row.append(n, t, a, r);
    grid.appendChild(row);
    rows.push({ row, time: t, athan, reminder: sel });
  });

  const box = $("offsets");
  NAMES.forEach((name, i) => {
    const row = document.createElement("div");
    row.className = "orow";
    const label = document.createElement("span");
    label.className = "pname";
    label.textContent = name;
    const minus = document.createElement("button");
    minus.type = "button";
    minus.textContent = "−";
    minus.setAttribute("aria-label", name + " eine Minute früher");
    const val = document.createElement("span");
    val.className = "oval";
    const plus = document.createElement("button");
    plus.type = "button";
    plus.textContent = "+";
    plus.setAttribute("aria-label", name + " eine Minute später");
    minus.addEventListener("click", () => changeOffset(i, -1));
    plus.addEventListener("click", () => changeOffset(i, 1));
    row.append(label, minus, val, plus);
    box.appendChild(row);
    offsetRows.push(val);
  });
  renderOffsets();
}

function renderOffsets() {
  offsetRows.forEach((el, i) => {
    const v = offsets[i];
    el.textContent = v === 0 ? "±0 min" : (v > 0 ? "+" : "−") + Math.abs(v) + " min";
    el.classList.toggle("changed", v !== 0);
  });
}

function changeOffset(i, delta) {
  offsets[i] = Math.max(-MAX_OFFSET, Math.min(MAX_OFFSET, offsets[i] + delta));
  renderOffsets();
  // mehrere Klicks bündeln, dann einmal speichern
  clearTimeout(offsetTimer);
  offsetTimer = setTimeout(() => save({ offsets: offsets.slice() }, "Korrektur gespeichert"), 700);
}

// Felder, die der Nutzer bedient, nur beim ersten Laden und nach eigenen Änderungen setzen.
function applyControls() {
  if (!state) return;
  $("athanTone").value = state.athanTone;
  $("reminderTone").value = state.reminderTone;
  $("fajrTone").value = state.fajrTone;
  $("volume").value = state.volume;
  $("volumeValue").textContent = state.volume;
  $("fajrVolume").value = state.fajrVolume;
  $("fajrVolumeValue").textContent = state.fajrVolume;
  if (![...$("method").options].some((o) => +o.value === state.method)) {
    $("method").appendChild(option(state.method, "Methode " + state.method));
  }
  $("method").value = state.method;
  $("school").value = state.school || 0;
  if (Array.isArray(state.offsets)) {
    offsets = state.offsets.slice();
    renderOffsets();
  }
  rows.forEach((r, i) => {
    if (r.athan) r.athan.checked = state.athanModes[i] === 1;
    r.reminder.value = state.reminderModes[i];
  });
}

function render() {
  if (!state) return;
  $("city").textContent = state.city;
  $("curLoc").textContent = state.city + (state.region ? ", " + state.region : "") +
    (state.hasCoords ? "" : " (alter Stand ohne Koordinaten – bitte neu auswählen)");
  $("date").textContent = state.synced ? WEEKDAYS[state.weekday] + ", " + state.date : "Uhrzeit wird synchronisiert…";
  // Wie auf dem Display: bevorstehende Gebete mit heutiger Zeit, vergangene schon mit der von morgen
  rows.forEach((r, i) => {
    r.time.textContent = fmt(state.upcoming[i]);
    if (state.upcoming[i] >= 0 && state.today[i] >= 0 && isTomorrow(i)) {
      const tag = document.createElement("small");
      tag.className = "tmr";
      tag.textContent = "morgen";
      r.time.appendChild(tag);
    }
  });
  $("wifiName").textContent = state.wifi || "–";
  $("version").textContent = "Version " + state.version;
  $("fajrHint").textContent = "Die Fajr-Lautstärke gilt für Fajr-Athan und die Erinnerung davor." +
    (state.ownFajr ? "" : " Kein Ordner 03 auf der SD-Karte: Fajr nutzt die normalen Athan-Töne.");

  const up = Math.floor(state.uptime / 3600);
  $("meta").textContent = "IP " + state.ip + " · WLAN " + state.rssi + " dBm · Heap " +
    Math.round(state.heap / 1024) + " KB · Laufzeit " + (up >= 24 ? Math.floor(up / 24) + " d" : up + " h");
  tick();
}

// Ist das heutige Gebet schon vorbei (dann zeigt "upcoming" die Zeit von morgen)?
// Gleiche Regel wie auf der Uhr: ab der Gebetsminute gilt die Zeit von morgen.
function isTomorrow(i) {
  const s = nowSec();
  if (s === null || state.today[i] < 0) return false;
  return Math.floor(s / 60) >= state.today[i];
}

function nowSec() {
  if (!state || !state.synced) return null;
  return (state.sec + Math.floor((Date.now() - syncedAt) / 1000)) % 86400;
}

function tick() {
  const s = nowSec();
  if (s === null) {
    $("clock").textContent = "--:--";
    $("next").textContent = state ? "Warte auf Uhrzeit…" : "Verbinde mit der Uhr…";
    return;
  }
  const h = Math.floor(s / 3600), m = Math.floor(s / 60) % 60, sec = s % 60;
  $("clock").innerHTML = String(h).padStart(2, "0") + ":" + String(m).padStart(2, "0") +
    "<small>:" + String(sec).padStart(2, "0") + "</small>";

  // Nächstes Gebet (ohne Shuruk), auch über Mitternacht
  let best = -1, bestLeft = Infinity;
  state.upcoming.forEach((min, i) => {
    if (i === SHURUK || min < 0) return;
    let left = (min * 60 - s) % 86400;
    if (left <= 0) left += 86400;
    if (left < bestLeft) { bestLeft = left; best = i; }
  });

  rows.forEach((r, i) => r.row.classList.toggle("next", i === best));

  if (best < 0) {
    $("next").textContent = "Keine Gebetszeiten geladen";
    return;
  }
  let left;
  if (bestLeft >= 3600) {
    left = Math.floor(bestLeft / 3600) + " h " + String(Math.floor(bestLeft / 60) % 60).padStart(2, "0") + " min";
  } else {
    left = String(Math.floor(bestLeft / 60)).padStart(2, "0") + ":" + String(bestLeft % 60).padStart(2, "0");
  }
  $("next").innerHTML = "";
  $("next").append("Nächstes Gebet: ", Object.assign(document.createElement("b"), { textContent: NAMES[best] }),
    " um " + fmt(state.upcoming[best]) + " · in ", Object.assign(document.createElement("b"), { textContent: left }));
}

function setState(data) {
  state = data;
  syncedAt = Date.now();
  $("offline").style.display = "none";
  render();
}

async function load(first) {
  try {
    setState(await api("/api/state"));
    if (first) applyControls();
  } catch (e) {
    $("offline").style.display = "block";
  }
}

async function save(body, okMsg) {
  try {
    setState(await api("/api/settings", body));
    applyControls();
    toast(okMsg || "Gespeichert");
  } catch (e) {
    toast(e.message, true);
    load(true);
  }
}

function saveModes() {
  save({
    athanModes: rows.map((r) => (r.athan && r.athan.checked ? 1 : 0)),
    reminderModes: rows.map((r) => +r.reminder.value)
  });
}

function bindEvents() {
  $("athanTone").addEventListener("change", (e) => save({ athanTone: +e.target.value }));
  $("reminderTone").addEventListener("change", (e) => save({ reminderTone: +e.target.value }));
  $("volume").addEventListener("input", (e) => { $("volumeValue").textContent = e.target.value; });
  $("volume").addEventListener("change", (e) => save({ volume: +e.target.value }, "Lautstärke " + e.target.value));
  $("fajrTone").addEventListener("change", (e) => save({ fajrTone: +e.target.value }));
  $("fajrVolume").addEventListener("input", (e) => { $("fajrVolumeValue").textContent = e.target.value; });
  $("fajrVolume").addEventListener("change", (e) => save({ fajrVolume: +e.target.value }, "Fajr-Lautstärke " + e.target.value));

  const PREVIEW_SELECT = { athan: "athanTone", fajr: "fajrTone", reminder: "reminderTone" };
  document.querySelectorAll("[data-preview]").forEach((btn) => btn.addEventListener("click", async () => {
    const kind = btn.dataset.preview;
    try {
      await api("/api/preview", { kind, tone: +$(PREVIEW_SELECT[kind]).value });
      toast("Spiele ab…");
    } catch (e) { toast(e.message, true); }
  }));

  $("stop").addEventListener("click", async () => {
    try { await api("/api/stop", {}); toast("Gestoppt"); } catch (e) { toast(e.message, true); }
  });

  bindSearch();

  $("locationForm").addEventListener("submit", async (e) => {
    e.preventDefault();
    const typed = $("cityInput").value.trim();
    if (typed && !picked) {
      // Freitext wird nie gesendet: nur Orte aus der Liste haben geprüfte Koordinaten.
      toast("Bitte einen Ort aus der Trefferliste auswählen.", true);
      $("cityInput").focus();
      return;
    }
    const body = { method: +$("method").value, school: +$("school").value };
    if (picked) {
      Object.assign(body, { name: picked.name, region: picked.admin1 || "", lat: picked.latitude, lon: picked.longitude });
    }
    const btn = $("locationSave");
    btn.disabled = true;
    btn.textContent = "Lade Zeiten für " + (picked ? picked.name : state.city) + "…";
    try {
      setState(await api("/api/location", body));
      applyControls();
      clearSearch();
      toast("Zeiten für " + state.city + " geladen");
    } catch (err) {
      toast(err.message, true);
    } finally {
      btn.disabled = false;
      btn.textContent = "Speichern & Zeiten laden";
    }
  });
}

// ---- Ortssuche ----
let picked = null;
let searchTimer = null;
let searchSeq = 0;

function placeLabel(r) {
  return [r.admin1, r.country].filter(Boolean).join(", ");
}

function clearSearch() {
  picked = null;
  $("cityInput").value = "";
  showResults([]);
  $("searchHint").textContent = "";
}

function showResults(list) {
  const ul = $("results");
  ul.innerHTML = "";
  list.forEach((r) => {
    const ok = CET_ZONES.includes(r.timezone);
    const li = document.createElement("li");
    const b = document.createElement("button");
    b.type = "button";
    b.setAttribute("role", "option");
    b.disabled = !ok;
    const strong = document.createElement("strong");
    strong.textContent = r.name;
    const small = document.createElement("small");
    small.textContent = placeLabel(r) + (ok ? "" : " · andere Zeitzone, nicht möglich");
    b.append(strong, small);
    b.addEventListener("click", () => pick(r));
    li.appendChild(b);
    ul.appendChild(li);
  });
  ul.hidden = list.length === 0;
  $("cityInput").setAttribute("aria-expanded", list.length ? "true" : "false");
}

function pick(r) {
  picked = r;
  $("cityInput").value = r.name;
  showResults([]);
  const hint = $("searchHint");
  hint.textContent = "";
  const s = document.createElement("span");
  s.className = "picked";
  s.textContent = "✓ " + r.name + ", " + placeLabel(r);
  hint.append("Ausgewählt: ", s, " – jetzt speichern.");
}

async function runSearch(q) {
  const seq = ++searchSeq;
  try {
    const res = await fetch(GEOCODER + encodeURIComponent(q));
    if (!res.ok) throw new Error();
    const data = await res.json();
    if (seq !== searchSeq) return;  // inzwischen weitergetippt
    const list = data.results || [];
    showResults(list);
    if (!list.length) {
      $("searchHint").textContent = "Kein Ort gefunden. Bitte Schreibweise prüfen.";
    } else if (!list.some((r) => CET_ZONES.includes(r.timezone))) {
      $("searchHint").textContent = "Gefundene Orte liegen nicht in der mitteleuropäischen Zeitzone.";
    } else {
      $("searchHint").textContent = "";
    }
  } catch (e) {
    if (seq !== searchSeq) return;
    showResults([]);
    $("searchHint").textContent = "Ortssuche nicht erreichbar. Hat dieses Gerät Internet?";
  }
}

function bindSearch() {
  const input = $("cityInput");
  input.addEventListener("input", () => {
    picked = null;
    clearTimeout(searchTimer);
    const q = input.value.trim();
    if (q.length < 2) {
      searchSeq++;
      showResults([]);
      $("searchHint").textContent = "";
      return;
    }
    $("searchHint").textContent = "Suche…";
    searchTimer = setTimeout(() => runSearch(q), 350);
  });
  input.addEventListener("keydown", (e) => {
    if (e.key === "Escape") showResults([]);
    if (e.key === "ArrowDown") {
      const first = $("results").querySelector("button:not(:disabled)");
      if (first) { e.preventDefault(); first.focus(); }
    }
  });
  $("results").addEventListener("keydown", (e) => {
    const btns = [...$("results").querySelectorAll("button:not(:disabled)")];
    const i = btns.indexOf(document.activeElement);
    if (e.key === "ArrowDown" && i < btns.length - 1) { e.preventDefault(); btns[i + 1].focus(); }
    if (e.key === "ArrowUp") { e.preventDefault(); (i > 0 ? btns[i - 1] : input).focus(); }
    if (e.key === "Escape") { showResults([]); input.focus(); }
  });
}

buildStatic();
bindEvents();
load(true);
setInterval(tick, 1000);
setInterval(() => load(false), 30000);
</script>
</body>
</html>
)rawliteral";

#endif
