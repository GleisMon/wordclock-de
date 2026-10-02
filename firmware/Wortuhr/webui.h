#pragma once
// Settings page (served from flash). Talks to /api/state, /api/config and /api/action.
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Wortuhr</title>
<style>
:root{--bg:#f4f1ec;--card:#fff;--fg:#1d1b18;--mut:#6f6a62;--acc:#c0632b;--line:#e4dfd6}
@media(prefers-color-scheme:dark){:root{--bg:#141311;--card:#1e1c19;--fg:#efe9df;--mut:#a39c90;--acc:#e08a52;--line:#2f2c27}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.4 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
main{max-width:560px;margin:0 auto;padding:16px}
header{display:flex;align-items:baseline;justify-content:space-between;gap:8px;margin:8px 0 16px}
h1{font-size:28px;margin:0;letter-spacing:.04em}#clock{font-size:15px;color:var(--mut)}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px 16px;margin-bottom:12px}
.card h2{font-size:13px;text-transform:uppercase;letter-spacing:.08em;color:var(--mut);margin:0 0 10px}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:8px 0;border-top:1px solid var(--line)}
.row:first-of-type{border-top:0}.row label{flex:1}
input[type=range]{width:100%;accent-color:var(--acc)}input[type=checkbox]{width:22px;height:22px;accent-color:var(--acc)}
select,input[type=text],input[type=time],input[type=number]{font:inherit;padding:6px 8px;border:1px solid var(--line);border-radius:8px;background:var(--bg);color:var(--fg);max-width:60%}
button{font:inherit;padding:9px 14px;border-radius:10px;border:1px solid var(--line);background:var(--bg);color:var(--fg);cursor:pointer}
button.pri{background:var(--acc);border-color:var(--acc);color:#fff}
.btns{display:flex;flex-wrap:wrap;gap:8px;margin-top:6px}
details summary{cursor:pointer;color:var(--mut);padding:6px 0}
.mut{color:var(--mut);font-size:13px}.lang{display:flex;gap:4px}.lang button{padding:4px 8px;font-size:13px}
.lang button.on{border-color:var(--acc);color:var(--acc)}#toast{position:fixed;left:50%;bottom:16px;transform:translateX(-50%);background:var(--fg);color:var(--bg);padding:8px 14px;border-radius:10px;opacity:0;transition:.3s}
</style></head><body><main>
<header><div><h1>WORTUHR</h1><div id="clock">…</div></div>
<div class="lang"><button data-l="de">DE</button><button data-l="ru">RU</button><button data-l="en">EN</button></div></header>

<div class="card"><h2 data-t="disp">Anzeige</h2>
<div class="row"><label data-t="bright">Helligkeit</label><input id="brightness" type="range" min="1" max="100"></div>
<div class="row"><label data-t="colors">Farben</label><select id="colorMode"><option value="0" data-t="cRand">Zufällig</option><option value="1" data-t="cOne">Einfarbig</option></select></div>
<div class="row" id="colorRow"><label data-t="color">Farbe</label><input id="color" type="color"></div>
<div class="row"><label data-t="minuten">Wort „MINUTEN“ anzeigen</label><input id="minuten" type="checkbox"></div>
<div class="row"><label data-t="heart">Herz zur vollen Stunde</label><input id="heart" type="checkbox"></div>
<div class="btns"><button class="pri" data-a="reroll" data-t="reroll">Neue Farben</button><button data-a="heartnow" data-t="heartNow">Herz zeigen</button></div></div>

<div class="card"><h2 data-t="night">Nachtmodus</h2>
<div class="row"><label data-t="nightOn">Nachts dimmen</label><input id="nightOn" type="checkbox"></div>
<div class="row"><label data-t="from">Von</label><input id="nightFrom" type="time"></div>
<div class="row"><label data-t="to">Bis</label><input id="nightTo" type="time"></div>
<div class="row"><label data-t="nightB">Helligkeit nachts (0 = aus)</label><input id="nightBrightness" type="range" min="0" max="100"></div></div>

<div class="card"><h2 data-t="sys">System</h2>
<div class="row"><span data-t="ver">Version</span><span id="ver"></span></div>
<div class="row"><span data-t="upd">Update</span><span id="upd" class="mut"></span></div>
<div class="btns"><button data-a="check" data-t="check">Nach Update suchen</button><button class="pri" id="installBtn" data-a="install" data-t="install" hidden>Update installieren</button></div>
<details><summary data-t="adv">Erweiterte Einstellungen</summary>
<div class="row"><label data-t="v20">Um :20</label><select id="v20"><option value="0">zwanzig nach drei</option><option value="1">zehn vor halb vier</option></select></div>
<div class="row"><label data-t="v40">Um :40</label><select id="v40"><option value="0">zwanzig vor vier</option><option value="1">zehn nach halb vier</option></select></div>
<div class="row"><label data-t="esist">„ES IST“ immer anzeigen</label><input id="esIstAlways" type="checkbox"></div>
<div class="row"><label data-t="fade">Weich überblenden</label><input id="fade" type="checkbox"></div>
<div class="row"><label data-t="tz">Zeitzone</label><select id="tz">
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Berlin / Wien / Zürich</option>
<option value="GMT0BST,M3.5.0/1,M10.5.0">London</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Kyiv / Helsinki</option>
<option value="MSK-3">Moskau</option>
<option value="&lt;+03&gt;-3">Istanbul</option>
<option value="EST5EDT,M3.2.0,M11.1.0">New York</option></select></div>
<div class="row"><label data-t="ntp">Zeitserver</label><input id="ntp" type="text"></div>
<div class="row"><label data-t="limit">Strombegrenzung LEDs (mA)</label><input id="limitMa" type="number" min="300" max="3000" step="100"></div>
<div class="row"><label data-t="auto">Updates automatisch installieren (nachts)</label><input id="autoUpdate" type="checkbox"></div>
<div class="row"><span data-t="net">Netzwerk</span><span id="net" class="mut"></span></div>
<div class="btns"><button data-a="test" data-t="test">LED-Test</button><a href="/update"><button data-t="upload">Firmware hochladen</button></a>
<button data-a="reboot" data-t="reboot">Neustart</button><button data-a="wifireset" data-t="wifi" data-c="1">WLAN vergessen</button><button data-a="factory" data-t="factory" data-c="1">Werkseinstellungen</button></div>
</details></div>
<p class="mut" style="text-align:center">wortuhr.local · github.com/GleisMon/wordclock-de</p>
</main><div id="toast"></div>
<script>
const T={
de:{},
ru:{disp:"Отображение",bright:"Яркость",colors:"Цвета",cRand:"Случайные",cOne:"Один цвет",color:"Цвет",minuten:"Показывать слово «MINUTEN»",heart:"Сердце в начале часа",reroll:"Новые цвета",heartNow:"Показать сердце",night:"Ночной режим",nightOn:"Приглушать ночью",from:"С",to:"До",nightB:"Яркость ночью (0 = выкл.)",sys:"Система",ver:"Версия",upd:"Обновление",check:"Проверить обновление",install:"Установить обновление",adv:"Расширенные настройки",v20:"В :20",v40:"В :40",esist:"Всегда показывать «ES IST»",fade:"Плавная смена",tz:"Часовой пояс",ntp:"Сервер времени",limit:"Лимит тока светодиодов (мА)",auto:"Ставить обновления автоматически (ночью)",net:"Сеть",test:"Тест светодиодов",upload:"Загрузить прошивку",reboot:"Перезагрузка",wifi:"Забыть Wi-Fi",factory:"Сброс настроек",saved:"Сохранено",sure:"Точно?",upToDate:"Актуальная версия",avail:"Доступна версия ",never:"ещё не проверялось",err:"Ошибка: "},
en:{disp:"Display",bright:"Brightness",colors:"Colours",cRand:"Random",cOne:"Single colour",color:"Colour",minuten:"Show the word “MINUTEN”",heart:"Heart at the full hour",reroll:"New colours",heartNow:"Show heart",night:"Night mode",nightOn:"Dim at night",from:"From",to:"To",nightB:"Night brightness (0 = off)",sys:"System",ver:"Version",upd:"Update",check:"Check for update",install:"Install update",adv:"Advanced settings",v20:"At :20",v40:"At :40",esist:"Always show “ES IST”",fade:"Smooth fade",tz:"Time zone",ntp:"Time server",limit:"LED current limit (mA)",auto:"Install updates automatically (at night)",net:"Network",test:"LED test",upload:"Upload firmware",reboot:"Restart",wifi:"Forget WiFi",factory:"Factory reset",saved:"Saved",sure:"Are you sure?",upToDate:"Up to date",avail:"Available: ",never:"not checked yet",err:"Error: "}};
const DE={saved:"Gespeichert",sure:"Sicher?",upToDate:"Aktuell",avail:"Verfügbar: ",never:"noch nicht geprüft",err:"Fehler: "};
let cfg={},lang="de",timer;const $=id=>document.getElementById(id);
const tr=k=>(T[lang]&&T[lang][k])||DE[k]||k;
function applyLang(){document.querySelectorAll("[data-t]").forEach(e=>{if(!e.dataset.de)e.dataset.de=e.textContent;const v=T[lang][e.dataset.t];e.textContent=v||e.dataset.de});
document.querySelectorAll(".lang button").forEach(b=>b.classList.toggle("on",b.dataset.l==lang));document.documentElement.lang=lang}
function toast(m){const t=$("toast");t.textContent=m;t.style.opacity=1;clearTimeout(t._h);t._h=setTimeout(()=>t.style.opacity=0,1500)}
const hm=m=>String(Math.floor(m/60)).padStart(2,"0")+":"+String(m%60).padStart(2,"0");
const mins=s=>{const[a,b]=s.split(":").map(Number);return a*60+b};
function fill(){for(const k in cfg){const e=$(k);if(!e)continue;
if(e.type=="checkbox")e.checked=cfg[k];else if(e.type=="time")e.value=hm(cfg[k]);else if(e.type=="color")e.value="#"+cfg[k].toString(16).padStart(6,"0");else e.value=cfg[k]}
$("colorRow").hidden=cfg.colorMode!=1}
function read(){const o={};document.querySelectorAll("input[id],select[id]").forEach(e=>{const k=e.id;if(!(k in cfg))return;
o[k]=e.type=="checkbox"?e.checked:e.type=="time"?mins(e.value):e.type=="color"?parseInt(e.value.slice(1),16):(typeof cfg[k]=="number"?Number(e.value):e.value)});return o}
function save(){clearTimeout(timer);timer=setTimeout(async()=>{cfg=Object.assign(cfg,read());$("colorRow").hidden=cfg.colorMode!=1;
const r=await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(cfg)});if(r.ok)toast(tr("saved"))},350)}
async function state(){try{const s=await (await fetch("/api/state")).json();
if(!Object.keys(cfg).length){cfg=s.cfg;lang=cfg.lang||"de";applyLang();fill()}
$("clock").textContent=s.st.synced?s.st.time+" · "+s.st.date:"…";$("ver").textContent=s.st.ver;
$("net").textContent=s.st.ssid+" · "+s.st.ip+" · "+s.st.rssi+" dBm";
$("upd").textContent=s.st.updErr?tr("err")+s.st.updErr:!s.st.checked?tr("never"):s.st.updAvail?tr("avail")+s.st.latest:tr("upToDate");
$("installBtn").hidden=!s.st.updAvail}catch(e){}}
document.querySelectorAll("input,select").forEach(e=>e.addEventListener(e.type=="range"?"input":"change",save));
document.querySelectorAll("[data-a]").forEach(b=>b.addEventListener("click",async ev=>{ev.preventDefault();if(b.dataset.c&&!confirm(tr("sure")))return;
await fetch("/api/action",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:"a="+b.dataset.a});setTimeout(state,b.dataset.a=="check"?4000:300)}));
document.querySelectorAll(".lang button").forEach(b=>b.addEventListener("click",()=>{lang=b.dataset.l;cfg.lang=lang;applyLang();save()}));
state();setInterval(state,5000);
</script></body></html>)HTML";

static const char RESCUE_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Wortuhr Rescue</title></head>
<body style="font-family:system-ui;max-width:480px;margin:40px auto;padding:0 16px"><h1>Wortuhr – Rettungsmodus</h1>
<p>Die Uhr ist mehrmals hintereinander neu gestartet. Bitte eine funktionierende Firmware (<code>wortuhr-de.bin</code> von GitHub Releases) hochladen.</p>
<p><a href="/update">Firmware hochladen</a> · <a href="/normal">Normal starten</a></p></body></html>)HTML";
