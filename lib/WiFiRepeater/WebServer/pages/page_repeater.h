#ifndef PAGE_REPEATER_H
#define PAGE_REPEATER_H

// Modern dark-themed HTML page for WiFi Repeater configuration
// Features: Live Traffic Gauge, Connected Clients Table, WiFi Scanner, NVS Config forms

static const char PAGE_REPEATER_HEAD[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>WiFi Repeater Dashboard</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:#0a0e27;color:#e0e0e0;min-height:100vh;padding:1rem}
.container{max-width:540px;margin:0 auto}
h1{text-align:center;color:#00d9ff;font-size:1.6rem;margin-bottom:1.2rem;text-shadow:0 0 20px rgba(0,217,255,0.3)}
.card{background:rgba(22,33,62,0.85);border:1px solid rgba(0,217,255,0.18);border-radius:12px;padding:1.2rem;margin-bottom:1rem;backdrop-filter:blur(10px);box-shadow:0 8px 24px rgba(0,0,0,0.3)}
.card h2{color:#00d9ff;font-size:1.05rem;margin-bottom:0.8rem;display:flex;align-items:center;gap:0.5rem}
table{width:100%;border-collapse:collapse}
td,th{padding:0.5rem 0.3rem;font-size:0.85rem;text-align:left}
th{color:#00d9ff;border-bottom:1px solid rgba(0,217,255,0.2);font-weight:600}
td:first-child{color:#aaa}
.online{color:#4caf50}
.offline{color:#f44336}
.traffic-grid{display:grid;grid-template-columns:1fr 1fr;gap:0.8rem;margin-bottom:0.5rem}
.stat-box{background:rgba(0,217,255,0.06);border:1px solid rgba(0,217,255,0.2);border-radius:10px;padding:0.8rem;text-align:center}
.stat-box.tx{background:rgba(118,75,162,0.1);border-color:rgba(118,75,162,0.3)}
.stat-label{font-size:0.75rem;color:#aaa;margin-bottom:0.2rem}
.stat-val{font-size:1.2rem;font-weight:700;color:#00d9ff}
.stat-box.tx .stat-val{color:#b794f4}
.stat-sub{font-size:0.75rem;color:#888;margin-top:0.2rem}
input[type=text],input[type=password]{width:100%;padding:0.65rem;margin:0.3rem 0 0.6rem;background:rgba(255,255,255,0.08);border:1px solid rgba(0,217,255,0.25);border-radius:8px;color:#e0e0e0;font-size:0.9rem;outline:none;transition:border 0.3s}
input[type=text]:focus,input[type=password]:focus{border-color:#00d9ff}
label{color:#aaa;font-size:0.85rem}
.btn{display:block;width:100%;padding:0.7rem;border:none;border-radius:8px;font-size:0.95rem;font-weight:600;cursor:pointer;transition:all 0.3s;margin-top:0.5rem}
.btn-primary{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff}
.btn-primary:hover{transform:translateY(-1px);box-shadow:0 4px 15px rgba(102,126,234,0.4)}
.btn-scan{background:linear-gradient(135deg,#00d9ff 0%,#0077b6 100%);color:#fff}
.btn-scan:hover{transform:translateY(-1px);box-shadow:0 4px 15px rgba(0,217,255,0.4)}
.ap-list{max-height:220px;overflow-y:auto;margin-top:0.5rem}
.ap-item{display:flex;justify-content:space-between;align-items:center;padding:0.5rem 0.7rem;margin:0.3rem 0;background:rgba(255,255,255,0.05);border-radius:8px;cursor:pointer;transition:background 0.2s}
.ap-item:hover{background:rgba(0,217,255,0.15)}
.ap-item .ssid{font-weight:500;max-width:65%;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.ap-item .rssi{color:#888;font-size:0.8rem}
.signal-bar{display:inline-block;width:4px;height:8px;background:#555;margin:0 1px;border-radius:1px;vertical-align:bottom}
.signal-bar.active{background:#4caf50}
.msg{padding:0.6rem;border-radius:8px;margin:0.5rem 0;font-size:0.85rem;text-align:center}
.msg-info{background:rgba(0,217,255,0.1);color:#00d9ff;border:1px solid rgba(0,217,255,0.3)}
.msg-err{background:rgba(244,67,54,0.1);color:#f44336;border:1px solid rgba(244,67,54,0.3)}
.spinner{display:inline-block;width:16px;height:16px;border:2px solid rgba(0,217,255,0.3);border-top:2px solid #00d9ff;border-radius:50%;animation:spin 0.8s linear infinite;margin-right:0.5rem;vertical-align:middle}
@keyframes spin{to{transform:rotate(360deg)}}
.footer{text-align:center;color:#555;font-size:0.75rem;margin-top:1rem}
</style>
</head>
<body>
<div class='container'>
<h1>📡 WiFi Repeater</h1>

<!-- Live Traffic Card -->
<div class='card'>
<h2>⚡ Live Traffic & Speed</h2>
<div class='traffic-grid'>
  <div class='stat-box'>
    <div class='stat-label'>⬇ Download Speed</div>
    <div class='stat-val' id='rxSpeed'>0 B/s</div>
    <div class='stat-sub' id='rxTotal'>Total: 0 B</div>
  </div>
  <div class='stat-box tx'>
    <div class='stat-label'>⬆ Upload Speed</div>
    <div class='stat-val' id='txSpeed'>0 B/s</div>
    <div class='stat-sub' id='txTotal'>Total: 0 B</div>
  </div>
</div>
</div>

<!-- Connected Clients Card -->
<div class='card'>
<h2>👥 Connected Clients (<span id='clientCount'>0</span>)</h2>
<div id='clientTableContainer'>
  <table id='clientTable'>
    <thead><tr><th>IP Address</th><th>MAC Address</th><th>Signal</th></tr></thead>
    <tbody id='clientBody'><tr><td colspan='3' style='text-align:center;color:#666'>No clients connected</td></tr></tbody>
  </table>
</div>
</div>
)rawliteral";

static const char PAGE_REPEATER_TAIL[] = R"rawliteral(
<div class='footer'>ESP32 WiFi Repeater Dashboard</div>
</div>

<script>
function fetchStats(){
  fetch('/api/stats').then(r=>r.json()).then(data=>{
    document.getElementById('rxSpeed').innerText = data.rx_speed_str;
    document.getElementById('txSpeed').innerText = data.tx_speed_str;
    document.getElementById('rxTotal').innerText = 'Total: ' + data.rx_bytes_str;
    document.getElementById('txTotal').innerText = 'Total: ' + data.tx_bytes_str;
    document.getElementById('clientCount').innerText = data.clients_count;

    var body = document.getElementById('clientBody');
    if(!data.clients || !data.clients.length){
      body.innerHTML = '<tr><td colspan="3" style="text-align:center;color:#666">No clients connected</td></tr>';
      return;
    }
    var html = '';
    for(var i=0; i<data.clients.length; i++){
      var c = data.clients[i];
      html += '<tr><td><strong>' + c.ip + '</strong></td><td>' + c.mac + '</td><td>' + c.rssi + ' dBm</td></tr>';
    }
    body.innerHTML = html;
  }).catch(e=>{});
}
setInterval(fetchStats, 2000);
fetchStats();

function scanWifi(){
  var btn=document.getElementById('scanBtn');
  var list=document.getElementById('apList');
  btn.disabled=true;
  btn.innerHTML='<span class="spinner"></span>Scanning...';
  list.innerHTML='<div class="msg msg-info">Scanning nearby networks...</div>';
  fetch('/scan').then(r=>r.json()).then(data=>{
    btn.disabled=false;
    btn.innerHTML='🔍 Scan WiFi';
    if(!data.length){list.innerHTML='<div class="msg msg-info">No networks found</div>';return;}
    var html='';
    for(var i=0;i<data.length;i++){
      var s=data[i];
      var bars='';
      var lvl=s.rssi>-50?4:s.rssi>-60?3:s.rssi>-70?2:1;
      for(var b=1;b<=4;b++){
        bars+='<span class="signal-bar'+(b<=lvl?' active':'')+'" style="height:'+(4+b*3)+'px"></span>';
      }
      html+='<div class="ap-item" onclick="selectAp(\''+s.ssid.replace(/'/g,"\\'")+'\')">';
      html+='<span class="ssid">'+(s.ssid||'[Hidden]')+'</span>';
      html+='<span class="rssi">'+bars+' '+s.rssi+'dBm</span></div>';
    }
    list.innerHTML=html;
  }).catch(e=>{
    btn.disabled=false;
    btn.innerHTML='🔍 Scan WiFi';
    list.innerHTML='<div class="msg msg-err">Scan failed: '+e+'</div>';
  });
}
function selectAp(ssid){
  document.getElementById('sta_ssid').value=ssid;
  document.getElementById('sta_pass').focus();
}
function saveConfig(){
  var btn=document.getElementById('saveBtn');
  btn.disabled=true;
  btn.innerHTML='<span class="spinner"></span>Applying...';
  var data='sta_ssid='+encodeURIComponent(document.getElementById('sta_ssid').value);
  data+='&sta_pass='+encodeURIComponent(document.getElementById('sta_pass').value);
  data+='&ap_ssid='+encodeURIComponent(document.getElementById('ap_ssid').value);
  data+='&ap_pass='+encodeURIComponent(document.getElementById('ap_pass').value);
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:data})
  .then(r=>r.json()).then(d=>{
    if(d.ok){
      btn.innerHTML='✅ Config Applied!';
      setTimeout(function(){
        btn.disabled=false;
        btn.innerHTML='💾 Save & Apply';
      },3000);
    }else{
      btn.disabled=false;
      btn.innerHTML='💾 Save & Apply';
      alert('Error: '+d.msg);
    }
  }).catch(e=>{
    btn.disabled=false;
    btn.innerHTML='💾 Save & Apply';
    alert('Save failed: '+e);
  });
}
</script>
</body></html>
)rawliteral";

#endif // PAGE_REPEATER_H
