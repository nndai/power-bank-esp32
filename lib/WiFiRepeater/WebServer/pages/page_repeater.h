#ifndef PAGE_REPEATER_H
#define PAGE_REPEATER_H

// Modern dark-themed HTML page for WiFi Repeater configuration
// Single page with: Status, WiFi Scanner, Config forms, Save & Restart

static const char PAGE_REPEATER_HEAD[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>WiFi Repeater</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',system-ui,-apple-system,sans-serif;background:#0a0e27;color:#e0e0e0;min-height:100vh;padding:1rem}
.container{max-width:520px;margin:0 auto}
h1{text-align:center;color:#00d9ff;font-size:1.5rem;margin-bottom:1.2rem;text-shadow:0 0 20px rgba(0,217,255,0.3)}
.card{background:rgba(22,33,62,0.8);border:1px solid rgba(0,217,255,0.15);border-radius:12px;padding:1.2rem;margin-bottom:1rem;backdrop-filter:blur(10px)}
.card h2{color:#00d9ff;font-size:1rem;margin-bottom:0.8rem;display:flex;align-items:center;gap:0.5rem}
table{width:100%;border-collapse:collapse}
td{padding:0.4rem 0;font-size:0.9rem}
td:first-child{color:#888;width:40%}
td:last-child{font-weight:500}
.online{color:#4caf50}
.offline{color:#f44336}
input[type=text],input[type=password]{width:100%;padding:0.6rem;margin:0.3rem 0 0.6rem;background:rgba(255,255,255,0.08);border:1px solid rgba(0,217,255,0.25);border-radius:8px;color:#e0e0e0;font-size:0.9rem;outline:none;transition:border 0.3s}
input[type=text]:focus,input[type=password]:focus{border-color:#00d9ff}
label{color:#aaa;font-size:0.85rem}
.btn{display:block;width:100%;padding:0.7rem;border:none;border-radius:8px;font-size:0.95rem;font-weight:600;cursor:pointer;transition:all 0.3s;margin-top:0.5rem}
.btn-primary{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff}
.btn-primary:hover{transform:translateY(-1px);box-shadow:0 4px 15px rgba(102,126,234,0.4)}
.btn-scan{background:linear-gradient(135deg,#00d9ff 0%,#0077b6 100%);color:#fff}
.btn-scan:hover{transform:translateY(-1px);box-shadow:0 4px 15px rgba(0,217,255,0.4)}
.btn-danger{background:linear-gradient(135deg,#f093fb 0%,#f5576c 100%);color:#fff}
.btn-danger:hover{transform:translateY(-1px);box-shadow:0 4px 15px rgba(245,87,108,0.4)}
.ap-list{max-height:250px;overflow-y:auto;margin-top:0.5rem}
.ap-item{display:flex;justify-content:space-between;align-items:center;padding:0.5rem 0.7rem;margin:0.3rem 0;background:rgba(255,255,255,0.05);border-radius:8px;cursor:pointer;transition:background 0.2s}
.ap-item:hover{background:rgba(0,217,255,0.15)}
.ap-item .ssid{font-weight:500;max-width:65%;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.ap-item .rssi{color:#888;font-size:0.8rem}
.signal-bar{display:inline-block;width:4px;height:8px;background:#555;margin:0 1px;border-radius:1px;vertical-align:bottom}
.signal-bar.active{background:#4caf50}
.msg{padding:0.6rem;border-radius:8px;margin:0.5rem 0;font-size:0.85rem;text-align:center}
.msg-info{background:rgba(0,217,255,0.1);color:#00d9ff;border:1px solid rgba(0,217,255,0.3)}
.msg-ok{background:rgba(76,175,80,0.1);color:#4caf50;border:1px solid rgba(76,175,80,0.3)}
.msg-err{background:rgba(244,67,54,0.1);color:#f44336;border:1px solid rgba(244,67,54,0.3)}
.spinner{display:inline-block;width:16px;height:16px;border:2px solid rgba(0,217,255,0.3);border-top:2px solid #00d9ff;border-radius:50%;animation:spin 0.8s linear infinite;margin-right:0.5rem;vertical-align:middle}
@keyframes spin{to{transform:rotate(360deg)}}
.footer{text-align:center;color:#555;font-size:0.75rem;margin-top:1rem}
</style>
</head>
<body>
<div class='container'>
<h1>📡 WiFi Repeater</h1>
)rawliteral";

static const char PAGE_REPEATER_TAIL[] = R"rawliteral(
<div class='footer'>ESP32 WiFi Repeater</div>
</div>

<script>
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
  btn.innerHTML='<span class="spinner"></span>Saving...';
  var data='sta_ssid='+encodeURIComponent(document.getElementById('sta_ssid').value);
  data+='&sta_pass='+encodeURIComponent(document.getElementById('sta_pass').value);
  data+='&ap_ssid='+encodeURIComponent(document.getElementById('ap_ssid').value);
  data+='&ap_pass='+encodeURIComponent(document.getElementById('ap_pass').value);
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:data})
  .then(r=>r.json()).then(d=>{
    if(d.ok){
      btn.innerHTML='✅ Saved! Restarting...';
      setTimeout(function(){location.reload();},5000);
    }else{
      btn.disabled=false;
      btn.innerHTML='💾 Save & Restart';
      alert('Error: '+d.msg);
    }
  }).catch(e=>{
    btn.disabled=false;
    btn.innerHTML='💾 Save & Restart';
    alert('Save failed: '+e);
  });
}
</script>
</body></html>
)rawliteral";

#endif // PAGE_REPEATER_H
