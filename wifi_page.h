// wifi_page.h
// WLAN-Einrichtungsseite. Erscheint automatisch im Hotspot "AthanClock-XXXX"
// (Captive Portal) und ist im Normalbetrieb unter /wifi erreichbar.
#ifndef WIFI_PAGE_H
#define WIFI_PAGE_H

#include <Arduino.h>

static const char WIFI_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="light dark">
<title>Athan Clock &ndash; WLAN</title>
<style>
:root{--bg:#f3f0e8;--card:#fffdf8;--text:#1d2a23;--muted:#66756c;--line:#e4ded1;--accent:#1f6f54;--on-accent:#fff;--soft:#e4efe9;--hero:#1f6f54;--on-hero:#fff;--danger:#b3261e;--r:16px}
@media (prefers-color-scheme:dark){:root{--bg:#101512;--card:#18201c;--text:#e6ede8;--muted:#93a49a;--line:#28322c;--accent:#5cc79d;--on-accent:#0c1a13;--soft:#1f332a;--hero:#173a2c;--on-hero:#e6ede8;--danger:#ff8a80}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:16px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
main{max-width:520px;margin:0 auto;padding:16px 16px 48px}
.hero{background:var(--hero);color:var(--on-hero);border-radius:var(--r);padding:20px 18px}
.hero h1{margin:0 0 4px;font-size:1.4rem}
.hero p{margin:0;opacity:.9}
.card{background:var(--card);border:1px solid var(--line);border-radius:var(--r);padding:16px;margin-top:16px}
h2{margin:0 0 10px;font-size:.8rem;font-weight:650;letter-spacing:.08em;text-transform:uppercase;color:var(--muted)}
.nets{list-style:none;margin:0;padding:0}
.nets li+li{border-top:1px solid var(--line)}
.nets button{display:flex;justify-content:space-between;align-items:center;width:100%;min-height:44px;padding:8px 4px;background:transparent;border:0;color:inherit;font:inherit;text-align:left;cursor:pointer}
.nets button:hover,.nets button:focus-visible{background:var(--soft);outline:none}
.sig{color:var(--muted);font-size:.85rem;white-space:nowrap}
.lock{display:inline-block;width:1.7em}
label{display:block;margin:12px 0 4px;color:var(--muted);font-size:.9rem}
input{width:100%;min-height:44px;padding:8px 10px;font:inherit;color:inherit;background:var(--bg);border:1px solid var(--line);border-radius:10px}
.pw{display:flex;gap:8px}
button.btn{min-height:44px;padding:8px 14px;font:inherit;color:var(--text);background:var(--soft);border:0;border-radius:10px;cursor:pointer;white-space:nowrap}
button.primary{width:100%;margin-top:16px;background:var(--accent);color:var(--on-accent);font-weight:600}
button:disabled{opacity:.6;cursor:default}
.hint{margin:8px 0 0;font-size:.85rem;color:var(--muted)}
.msg{margin-top:16px;padding:12px;border-radius:12px;display:none}
.msg.ok{display:block;background:var(--soft)}
.msg.err{display:block;background:var(--danger);color:#fff}
.back{display:inline-block;margin-top:16px;color:var(--accent)}
[hidden]{display:none!important}
</style>
</head>
<body>
<main>
  <section class="hero">
    <h1>WLAN einrichten</h1>
    <p id="intro">Verbinde die Athan Clock mit deinem WLAN.</p>
  </section>

  <section class="card">
    <h2>Netzwerke in der N&auml;he</h2>
    <ul class="nets" id="nets"></ul>
    <p class="hint" id="scanHint">Suche Netzwerke&hellip;</p>
    <button type="button" class="btn" id="rescan">Erneut suchen</button>
  </section>

  <section class="card">
    <h2>Zugangsdaten</h2>
    <form id="form" autocomplete="off">
      <label for="ssid">WLAN-Name (SSID)</label>
      <input id="ssid" maxlength="32" required autocapitalize="off" autocorrect="off" spellcheck="false">
      <label for="pass">WLAN-Passwort</label>
      <div class="pw">
        <input id="pass" type="password" maxlength="63" autocomplete="new-password">
        <button type="button" class="btn" id="toggle" aria-label="Passwort anzeigen">Zeigen</button>
      </div>
      <div id="authBox" hidden>
        <label for="devpw">Ger&auml;te-Passwort</label>
        <input id="devpw" type="password" maxlength="32" autocomplete="off">
        <p class="hint">Steht im Men&uuml; der Uhr unter &bdquo;Info&ldquo;.</p>
      </div>
      <button type="submit" class="btn primary" id="save">Verbinden &amp; speichern</button>
    </form>
    <div class="msg" id="msg" role="status" aria-live="polite"></div>
  </section>

  <a class="back" id="back" href="/" hidden>&larr; Zur&uuml;ck zur &Uuml;bersicht</a>
</main>
<script>
const $ = (id) => document.getElementById(id);
let status = { setupMode: true };

function msg(text, ok) {
  const m = $("msg");
  m.textContent = text;
  m.className = "msg " + (ok ? "ok" : "err");
}

function bars(rssi) {
  if (rssi >= -55) return "▂▄▆█";
  if (rssi >= -67) return "▂▄▆";
  if (rssi >= -78) return "▂▄";
  return "▂";
}

async function loadStatus() {
  try {
    status = await (await fetch("/api/wifi/status")).json();
  } catch (e) {}
  $("authBox").hidden = status.setupMode;
  $("back").hidden = status.setupMode;
  $("intro").textContent = status.setupMode
    ? "Wähle dein WLAN und gib das Passwort ein. Danach verbindet sich die Uhr selbst."
    : "Aktuell verbunden mit „" + (status.ssid || "?") + "“. Hier kannst du ein anderes WLAN eintragen.";
  if (status.ssid && !$("ssid").value) $("ssid").value = status.ssid;
}

async function scan(refresh) {
  $("scanHint").hidden = false;
  $("scanHint").textContent = "Suche Netzwerke…";
  $("rescan").disabled = true;
  try {
    // Die Uhr sucht im Hintergrund; so lange nachfragen, bis die Liste da ist.
    let data = await (await fetch("/api/wifi/scan" + (refresh ? "?refresh=1" : ""))).json();
    for (let i = 0; i < 20 && data.scanning; i++) {
      await new Promise((r) => setTimeout(r, 1000));
      data = await (await fetch("/api/wifi/scan")).json();
    }
    const list = data.networks || [];
    const ul = $("nets");
    ul.innerHTML = "";
    list.forEach((n) => {
      const li = document.createElement("li");
      const b = document.createElement("button");
      b.type = "button";
      const name = document.createElement("span");
      const lock = document.createElement("span");
      lock.className = "lock";
      lock.textContent = n.secure ? "🔒" : "";
      lock.setAttribute("aria-label", n.secure ? "verschlüsselt" : "offen");
      name.append(lock, n.ssid);
      const sig = document.createElement("span");
      sig.className = "sig";
      sig.textContent = bars(n.rssi);
      b.append(name, sig);
      b.addEventListener("click", () => { $("ssid").value = n.ssid; $("pass").focus(); });
      li.appendChild(b);
      ul.appendChild(li);
    });
    $("scanHint").textContent = list.length ? "" : "Keine Netzwerke gefunden.";
    $("scanHint").hidden = list.length > 0;
  } catch (e) {
    $("scanHint").textContent = "Suche fehlgeschlagen.";
  } finally {
    $("rescan").disabled = false;
  }
}

$("rescan").addEventListener("click", () => scan(true));
$("toggle").addEventListener("click", () => {
  const p = $("pass");
  const show = p.type === "password";
  p.type = show ? "text" : "password";
  $("toggle").textContent = show ? "Verbergen" : "Zeigen";
});

$("form").addEventListener("submit", async (e) => {
  e.preventDefault();
  const ssid = $("ssid").value;
  const pass = $("pass").value;
  if (pass.length > 0 && pass.length < 8) {
    msg("WLAN-Passwörter haben mindestens 8 Zeichen.", false);
    return;
  }
  const btn = $("save");
  btn.disabled = true;
  btn.textContent = status.setupMode ? "Verbinde… (bis zu 15 s)" : "Speichere…";
  try {
    const headers = { "Content-Type": "application/json" };
    if (!status.setupMode) headers["X-Device-Password"] = $("devpw").value;
    const res = await fetch("/api/wifi", { method: "POST", headers, body: JSON.stringify({ ssid, pass }) });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data.error || "Fehler " + res.status);
    if (status.setupMode) {
      msg("Verbunden mit „" + ssid + "“! Verbinde dein Handy wieder mit deinem WLAN. Die Uhr ist dann unter http://athanclock.local oder http://" + data.ip + " erreichbar.", true);
    } else {
      msg("Gespeichert. Die Uhr startet neu und verbindet sich mit „" + ssid + "“. Klappt das nicht, öffnet sie den Hotspot " + status.apName + ".", true);
    }
  } catch (err) {
    msg(err.message, false);
  } finally {
    btn.disabled = false;
    btn.textContent = "Verbinden & speichern";
  }
});

loadStatus().then(() => scan(false));
</script>
</body>
</html>
)rawliteral";

#endif
