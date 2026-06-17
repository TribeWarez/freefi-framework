#pragma once

#include <Arduino.h>

// TribeWarez explorer-inspired UI (matrix rain + centered responsive layout)

static const char WEB_UI_HEAD_PRE[] PROGMEM = R"WEBPRE(<!DOCTYPE html>
<html lang="en" data-theme="cyberpunk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="dark">
<meta name="theme-color" content="#0d0612">
<link rel="icon" href="/favicon.ico" type="image/svg+xml" sizes="any">
<title>)WEBPRE";

static const char WEB_UI_HEAD_POST[] PROGMEM = R"WEBPOST(</title>
<style>
*,*::before,*::after{box-sizing:border-box}
html,body{margin:0;min-height:100vh}
body{font-family:ui-sans-serif,system-ui,sans-serif;background:#0d0612;color:#f60;
  -webkit-font-smoothing:antialiased;line-height:1.5}
#matrixRainCanvas{position:fixed;inset:0;width:100%;height:100%;z-index:0;display:block;
  pointer-events:none;opacity:.92}
.explorer-content{position:relative;z-index:1;max-width:36rem;margin:0 auto;
  padding:1.25rem 1rem 2rem;width:100%;animation:fadeIn .5s ease-out}
@media(min-width:640px){.explorer-content{padding:2rem 1.5rem 3rem;max-width:42rem}}
@media(min-width:1024px){.explorer-content{max-width:48rem;padding:2.5rem 2rem 3rem}}
@keyframes fadeIn{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:none}}
.page-hero{text-align:center;margin-bottom:1.5rem;padding-bottom:1.25rem;
  border-bottom:1px solid #630}
.page-hero h1{margin:0;font-size:1.5rem;font-weight:700;letter-spacing:-.02em;color:#f60}
@media(min-width:640px){.page-hero h1{font-size:1.75rem}}
.page-hero .sub{margin:.35rem 0 0;font-size:.8rem;color:#94f}
.explorer-card{border:1px solid #630;border-radius:.75rem;background:#1a0d24;
  padding:1.25rem 1rem;box-shadow:0 1px 0 hsla(0,0%,100%,.03),0 8px 24px rgba(0,0,0,.35);
  text-align:center}
@media(min-width:640px){.explorer-card{padding:1.5rem 1.25rem}}
.explorer-card p{margin:.65rem 0;font-size:.9rem;color:#c90;line-height:1.55}
.explorer-card b{color:#f60}
.explorer-card code{font-family:ui-monospace,Menlo,monospace;font-size:.85em;
  color:#93f;background:rgba(13,6,18,.6);padding:.1rem .35rem;border-radius:.2rem;word-break:break-all}
.explorer-card ul{text-align:left;margin:.75rem auto 0;max-width:20rem;padding-left:1.25rem;color:#c90}
.explorer-card li{margin:.25rem 0;font-size:.85rem}
.nav-links{display:flex;flex-wrap:wrap;justify-content:center;gap:.5rem 1rem;margin-top:1rem}
.nav-links a{font-family:ui-monospace,monospace;font-size:.8rem;color:#94f;text-decoration:none;
  border:1px solid #630;border-radius:.25rem;padding:.35rem .65rem;transition:color .2s,border-color .2s}
.nav-links a:hover{color:#f60;border-color:#f60}
form{text-align:left;max-width:20rem;margin:0 auto}
label{display:block;margin:.75rem 0 .25rem;font-size:.75rem;text-transform:uppercase;
  letter-spacing:.05em;color:#94f}
input,button{width:100%;padding:.55rem .65rem;margin:0 0 .5rem;font:inherit;font-size:.9rem;
  border-radius:.25rem;border:1px solid #630;background:#0d0612;color:#f60}
input::placeholder{color:#630}
input:focus,button:focus{outline:none;box-shadow:0 0 0 2px rgba(153,51,255,.45)}
button{cursor:pointer;background:#1a0d24;color:#c90;font-family:ui-monospace,monospace;
  text-transform:uppercase;letter-spacing:.04em;font-size:.75rem}
button:hover{color:#f60;border-color:#f60}
.page-footer{text-align:center;margin-top:1.5rem;font-size:.7rem;color:#630;font-family:ui-monospace,monospace}
.chat-warn{font-size:.8rem;color:#f96;border:1px solid #630;border-radius:.35rem;padding:.5rem .65rem}
.chat-meta{font-size:.75rem;color:#94f}
.chat-log{text-align:left;max-width:100%;height:14rem;overflow-y:auto;margin:1rem 0;
  padding:.65rem;border:1px solid #630;border-radius:.35rem;background:#0d0612;
  font-family:ui-monospace,Menlo,monospace;font-size:.72rem;line-height:1.45;color:#c90}
.chat-line{margin:.2rem 0;word-break:break-word}
.chat-line.sys{color:#94f}
.chat-line .nick{color:#93f}
.chat-form{max-width:100%}
.node-map{display:block;width:min(100%,20rem);height:auto;margin:1rem auto;
  border:1px solid #630;border-radius:50%;background:radial-gradient(circle,#120818 0%,#0d0612 70%)}
.map-ranges{text-align:center}
.map-list{list-style:none;margin:.75rem auto 0;padding:0;max-width:22rem;text-align:left;
  font-family:ui-monospace,Menlo,monospace;font-size:.7rem;color:#94f;max-height:8rem;overflow-y:auto}
.map-list li{margin:.2rem 0;padding:.2rem 0;border-bottom:1px solid rgba(99,0,0,.25)}
.donate{margin-top:1.5rem;padding:1rem;border:1px solid #630;border-radius:.5rem;background:#120818;
  text-align:left;max-width:22rem;margin-left:auto;margin-right:auto}
.donate h2{margin:0 0 .65rem;font-size:.85rem;color:#f60;text-transform:uppercase;letter-spacing:.06em}
.donate-row{margin:.55rem 0}
.donate-row .lbl{font-size:.7rem;color:#94f;text-transform:uppercase}
.donate-row code{display:block;margin:.2rem 0 .35rem;font-size:.68rem;color:#93f;word-break:break-all}
.donate-row button{width:auto;padding:.3rem .55rem;font-size:.65rem;margin:0}
.tribe-links{text-align:center;margin:1.25rem 0 .5rem;font-family:ui-monospace,monospace;font-size:.75rem}
.tribe-links a{color:#94f;margin:0 .35rem}
.tribe-links a:hover{color:#f60}
.mode-toggle{display:inline-block;margin-left:.5rem;font-size:.7rem}
/* Low-res 90s static HTML device mode */
html.lowres-mode body{font-family:"Times New Roman",Times,serif;background:#c0c0c0;color:#000;
  -webkit-font-smoothing:auto}
html.lowres-mode #matrixRainCanvas{display:none!important}
html.lowres-mode .explorer-content{max-width:40em;padding:12px 16px 24px;background:#c0c0c0;
  animation:none;image-rendering:pixelated}
html.lowres-mode .page-hero{border-bottom:2px solid #000;padding-bottom:8px;margin-bottom:12px}
html.lowres-mode .page-hero h1{font-size:22px;font-weight:700;color:#000080;letter-spacing:0}
html.lowres-mode .page-hero .sub{font-size:14px;color:#000}
html.lowres-mode .explorer-card{background:#fff;border:2px outset #dfdfdf;border-radius:0;
  box-shadow:none;padding:12px;text-align:left}
html.lowres-mode .explorer-card p,html.lowres-mode .explorer-card li{font-size:14px;color:#000;line-height:1.4}
html.lowres-mode .explorer-card b{color:#800000}
html.lowres-mode .explorer-card code{font-family:"Courier New",monospace;background:#ffffe0;
  color:#000;border:1px solid #808080;border-radius:0;padding:0 2px}
html.lowres-mode .nav-links a,html.lowres-mode .tribe-links a{color:#0000ee;border:1px solid #808080;
  border-radius:0;background:#dfdfdf;padding:2px 6px;font-family:Arial,Helvetica,sans-serif;font-size:12px}
html.lowres-mode .nav-links a:visited,html.lowres-mode .tribe-links a:visited{color:#551a8b}
html.lowres-mode .nav-links a:hover,html.lowres-mode .tribe-links a:hover{color:#ff0000;background:#ffff00}
html.lowres-mode label{font-size:12px;color:#000;text-transform:none;letter-spacing:0}
html.lowres-mode input,html.lowres-mode button{font-family:Arial,Helvetica,sans-serif;font-size:13px;
  border:2px outset #dfdfdf;border-radius:0;background:#dfdfdf;color:#000}
html.lowres-mode button:active{border-style:inset}
html.lowres-mode .chat-warn{border:2px inset #808080;background:#ffffe0;color:#000;border-radius:0}
html.lowres-mode .chat-meta,.html.lowres-mode .map-ranges{color:#333;font-size:12px}
html.lowres-mode .chat-log{border:2px inset #808080;background:#fff;color:#000;border-radius:0;
  font-family:"Courier New",monospace;font-size:12px}
html.lowres-mode .node-map{border:2px inset #808080;border-radius:0;background:#000}
html.lowres-mode .donate{border:2px groove #808080;background:#ffffe0;border-radius:0}
html.lowres-mode .donate h2{color:#800000;font-size:14px}
html.lowres-mode .page-footer{font-size:11px;color:#333;font-family:Arial,sans-serif}
html.lowres-mode hr.lowres-hr{border:0;border-top:1px solid #808080;margin:12px 0}
html.lowres-mode .lowres-badge{display:inline;font-size:11px;color:#800000;font-weight:bold}
html.lowres-mode .lowres-construction{font-size:12px;color:#800000}
html.lowres-mode .mode-bar{background:#000080;color:#fff;padding:4px 8px;font-size:12px;
  font-family:Arial,sans-serif;text-align:center;margin:-12px -16px 12px}
html.lowres-mode .mode-bar a{color:#ffff00;margin-left:8px}
</style>
<script>
(function(){var q=location.search||'';var low=q.indexOf('lowres=1')>=0||q.indexOf('mode=lowres')>=0;
if(low){document.documentElement.className='lowres-mode';try{localStorage.setItem('twUi','lowres');}catch(e){}}
else if(q.indexOf('lowres=0')>=0||q.indexOf('mode=full')>=0){document.documentElement.className='';
try{localStorage.setItem('twUi','full');}catch(e){}}
else{try{if(localStorage.getItem('twUi')==='lowres')document.documentElement.className='lowres-mode';}catch(e){}}})();
</script>
</head>
<body class="min-h-screen">
<div class="mode-bar" hidden>TRIBEWAREZ NODE — <a class="ui-mode-full" href="?lowres=0">Switch to full UI</a></div>
<canvas id="matrixRainCanvas" class="explorer-matrix-bg" aria-hidden="true"></canvas>
<div class="explorer-content">
<header class="page-hero">
<h1>)WEBPOST";

static const char WEB_UI_HERO_MID[] PROGMEM = R"HEROMID(</h1>
<p class="sub">Hotspot &amp; NAT uplink — AP <code style="color:#93f">)HEROMID";

static const char WEB_UI_MID[] PROGMEM = R"WEBMID(</code></p>
</header>
<main class="explorer-card">
)WEBMID";

static const char WEB_UI_TAIL[] PROGMEM = R"WEBTAIL(
</main>
<nav class="nav-links">
<a href="/">Status</a>
<a href="/chat">IRC</a>
<a href="/map">Node map</a>
<a href="/config">Uplink WiFi</a>
<a href="/settings">Node settings</a>
<a href="/scan">Scan JSON</a>
<span class="mode-toggle">| <a class="ui-mode-low" href="?lowres=1">Low-res 90s</a> · <a class="ui-mode-full" href="?lowres=0">Full UI</a></span>
</nav>
<hr class="lowres-hr" hidden>
<p class="lowres-construction" hidden>⚠ Best viewed at 800×600 — static HTML node interface</p>
<nav class="tribe-links" aria-label="TribeWarez">
<a href="https://mystic.tribewarez.com" target="_blank" rel="noopener noreferrer">mystic.tribewarez.com</a>
·
<a href="https://defi.tribewarez.com" target="_blank" rel="noopener noreferrer">defi.tribewarez.com</a>
·
<a href="https://docs.tribewarez.com" target="_blank" rel="noopener noreferrer">docs.tribewarez.com</a>
</nav>
<section class="donate" aria-label="Donations">
<h2>Support</h2>
<div class="donate-row"><span class="lbl">Solana</span>
<code id="don-sol">2jSxUrYmZB6iwPBttUDgfjqXjH4H79TdgWkgv9JaKHtq</code>
<button type="button" data-copy="don-sol">Copy</button></div>
<div class="donate-row"><span class="lbl">TRX</span>
<code id="don-trx">TJMmwJaYgxUmSenuP693VzMEYhoAKwa4Ls</code>
<button type="button" data-copy="don-trx">Copy</button></div>
<div class="donate-row"><span class="lbl">ETH</span>
<code id="don-eth">0x7DA29D0A60c27597e4b9FAE525A0aff6e0691Bc4</code>
<button type="button" data-copy="don-eth">Copy</button></div>
</section>
<p class="page-footer">Local node only — chat &amp; map reset on reboot</p>
</div>
<script>
document.querySelectorAll('[data-copy]').forEach(function(btn){
btn.addEventListener('click',function(){
var el=document.getElementById(btn.getAttribute('data-copy'));
if(!el)return;
var t=el.textContent;
if(navigator.clipboard){navigator.clipboard.writeText(t).then(function(){
btn.textContent='Copied';setTimeout(function(){btn.textContent='Copy';},1200);
});}else{var ta=document.createElement('textarea');ta.value=t;document.body.appendChild(ta);
ta.select();try{document.execCommand('copy');btn.textContent='Copied';}catch(e){}
document.body.removeChild(ta);setTimeout(function(){btn.textContent='Copy';},1200);}
});
});
</script>
<script>
(function(){
var c=document.getElementById('matrixRainCanvas');
if(!c)return;
var CHARS='ァアィイゥウェエォカキクグケゲコゴサザシジスズセゼソゾタダチヂッツヅテデトドナニヌネノハバパヒビピフブプヘベペホボポマミ0123456789ABCDEF<>[]{}|';
var ORANGE='#ff6600',PURPLE='#9933ff',DIM='rgba(153,51,255,0.15)';
var drops=[];
function resize(){
c.width=window.innerWidth;c.height=window.innerHeight;
var cols=Math.max(1,Math.floor(c.width/18));
while(drops.length<cols)drops.push(Math.random()*-50);
drops.length=cols;
}
function draw(){
var ctx=c.getContext('2d'),w=c.width,h=c.height;
ctx.fillStyle='rgba(13,6,18,0.06)';ctx.fillRect(0,0,w,h);
ctx.font='14px monospace';
for(var i=0;i<drops.length;i++){
var x=i*18;
drops[i]+=0.5+(i%3)*0.3;
if(drops[i]*14>h+20)drops[i]=0;
var ch=CHARS[Math.floor(Math.random()*CHARS.length)];
ctx.fillStyle=i%3===0?ORANGE:PURPLE;
ctx.fillText(ch,x,drops[i]*14);
ctx.fillStyle=DIM;
for(var k=1;k<12;k++){
var y=(drops[i]-k)*14;
if(y>0){ctx.globalAlpha=1-k/14;
ctx.fillText(CHARS[Math.floor(Math.random()*CHARS.length)],x,y);
ctx.globalAlpha=1;}}}
requestAnimationFrame(draw);
}
resize();draw();
window.addEventListener('resize',resize);
})();
</script>
<script>
(function(){
var p=location.pathname||'/';
var base=p.split('?')[0];
function modeHref(on){return base+(on?'?lowres=1':'?lowres=0');}
document.querySelectorAll('.ui-mode-low').forEach(function(a){a.href=modeHref(true);});
document.querySelectorAll('.ui-mode-full').forEach(function(a){a.href=modeHref(false);});
var low=document.documentElement.classList.contains('lowres-mode');
if(!low)return;
var bar=document.querySelector('.mode-bar');
var hr=document.querySelector('.lowres-hr');
var note=document.querySelector('.lowres-construction');
if(bar){bar.hidden=false;}
if(hr){hr.hidden=false;}
if(note){note.hidden=false;}
var h=document.querySelector('.page-hero h1');
if(h&&!h.querySelector('.lowres-badge')){
var s=document.createElement('span');
s.className='lowres-badge';s.textContent=' [640×480 mode]';
h.appendChild(s);
}
})();
</script>
</body>
</html>
)WEBTAIL";

static const char WEB_UI_CHAT_SCRIPT[] PROGMEM = R"CHATJS(
<script>
(function(){
var log=document.getElementById('chatLog');
var form=document.getElementById('chatForm');
var input=document.getElementById('chatIn');
var since=0;
function esc(s){
var d=document.createElement('div');d.textContent=s;return d.innerHTML;
}
function pad(n){return (n<10?'0':'')+n;}
function line(m){
var t=new Date(m.ts*1000);
var ts=pad(t.getHours())+':'+pad(t.getMinutes());
var el=document.createElement('div');
el.className='chat-line'+(m.sys?' sys':'');
if(m.sys){
el.innerHTML='<span class="ts">'+ts+'</span> *** '+esc(m.text);
}else{
el.innerHTML='<span class="ts">'+ts+'</span> &lt;'+esc(m.user)+'&gt; '+esc(m.text);
}
return el;
}
function poll(){
fetch('/chat/poll?since='+since).then(function(r){return r.json();}).then(function(j){
if(!j.messages)return;
for(var i=0;i<j.messages.length;i++){
var m=j.messages[i];
log.appendChild(line(m));
since=Math.max(since,m.id);
}
log.scrollTop=log.scrollHeight;
}).catch(function(){});
}
form.addEventListener('submit',function(e){
e.preventDefault();
var msg=input.value.trim();
if(!msg)return;
fetch('/chat/send',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
body:'msg='+encodeURIComponent(msg)}).then(function(){
input.value='';poll();
});
});
poll();
setInterval(poll,2000);
})();
</script>
)CHATJS";

static const char WEB_FAVICON_SVG[] PROGMEM = R"FAVICON(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32">
<rect width="32" height="32" rx="6" fill="#0d0612"/>
<circle cx="16" cy="16" r="10" fill="none" stroke="#630" stroke-width="1"/>
<circle cx="16" cy="16" r="3" fill="#f60"/>
<path d="M16 6v4M16 22v4M6 16h4M22 16h4" stroke="#9933ff" stroke-width="1.2"/>
<text x="16" y="28" text-anchor="middle" fill="#94f" font-size="5" font-family="monospace">TW</text>
</svg>
)FAVICON";

static const char WEB_UI_MAP_SCRIPT[] PROGMEM = R"MAPJS(
<script>
(function(){
var cv=document.getElementById('nodeMap');
var meta=document.getElementById('mapMeta');
var list=document.getElementById('mapList');
var btn=document.getElementById('mapRefresh');
if(!cv)return;
var defaultRanges=[{rssi:-45,label:'~2m'},{rssi:-60,label:'~10m'},{rssi:-75,label:'~30m'},{rssi:-88,label:'100m+'}];
function rssiRadius(rssi){
return Math.max(0.08,Math.min(0.92,((-rssi)-25)/75));
}
function rangeLabel(rssi,n){
if(n&&n.range)return n.range;
if(rssi>=-45)return '~2m';
if(rssi>=-60)return '~10m';
if(rssi>=-75)return '~30m';
return '100m+';
}
function draw(data){
var ctx=cv.getContext('2d');
var w=cv.width,h=cv.height,cx=w/2,cy=h/2,R=Math.min(cx,cy)-8;
var ranges=data.ranges||defaultRanges;
ctx.clearRect(0,0,w,h);
ctx.font='9px monospace';
for(var i=0;i<ranges.length;i++){
var rr=R*rssiRadius(ranges[i].rssi);
ctx.strokeStyle='rgba(153,51,255,0.18)';
ctx.setLineDash([3,4]);
ctx.beginPath();ctx.arc(cx,cy,rr,0,Math.PI*2);ctx.stroke();
ctx.setLineDash([]);
ctx.fillStyle='#94f';
ctx.fillText(ranges[i].label+' ('+ranges[i].rssi+' dBm)',cx+rr-18,cy-2);
}
ctx.fillStyle='#f60';ctx.beginPath();ctx.arc(cx,cy,5,0,Math.PI*2);ctx.fill();
ctx.font='10px monospace';ctx.fillStyle='#94f';
ctx.fillText('NODE',cx-14,cy-10);
var nodes=data.nodes||[];
if(list){list.innerHTML='';}
for(var j=0;j<nodes.length;j++){
var n=nodes[j];
var ang=(n.angle||0)*Math.PI/180;
var rad=R*rssiRadius(n.rssi||-80);
var x=cx+Math.cos(ang)*rad;
var y=cy+Math.sin(ang)*rad;
if(n.kind==='wifi')ctx.fillStyle=n.tribe?'#f60':'#94f';
else if(n.kind==='client')ctx.fillStyle='#93f';
else if(n.kind==='ble')ctx.fillStyle='#c6f';
else ctx.fillStyle='#c90';
ctx.beginPath();ctx.arc(x,y,4,0,Math.PI*2);ctx.fill();
if(list){
var li=document.createElement('li');
var nm=n.name||'?';
li.textContent=nm+' · '+rangeLabel(n.rssi,n)+' · '+n.rssi+' dBm';
list.appendChild(li);
}
}
meta.textContent=(data.radio||'')+' · '+nodes.length+' blips · rings = RSSI range est.';
}
function load(){
meta.textContent='Scanning…';
fetch('/map/data').then(function(r){return r.json();}).then(draw)
.catch(function(){meta.textContent='Scan failed';});
}
if(btn)btn.addEventListener('click',load);
load();
setInterval(load,15000);
})();
</script>
)MAPJS";

static inline String htmlPage(const String &body, const char *apSsid,
                              const char *pageTitle) {
  String out;
  out.reserve(body.length() + 4800);
  out += FPSTR(WEB_UI_HEAD_PRE);
  out += pageTitle;
  out += FPSTR(WEB_UI_HEAD_POST);
  out += pageTitle;
  out += FPSTR(WEB_UI_HERO_MID);
  out += apSsid;
  out += FPSTR(WEB_UI_MID);
  out += body;
  out += FPSTR(WEB_UI_TAIL);
  return out;
}
