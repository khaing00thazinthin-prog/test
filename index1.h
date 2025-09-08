const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Water Quality Monitoring</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
:root{
  --bg:#f0f4f8; --card:#ffffffbb; --ink:#1f2937; --muted:#6b7280; --ok:#16a34a; --warn:#f59e0b; --bad:#dc2626; --brand:#2c3e50;
}
*{box-sizing:border-box}
body{margin:0;font-family:Segoe UI,system-ui,-apple-system,Roboto,Helvetica,Arial,sans-serif;background:var(--bg);color:var(--ink)}
header{background:var(--brand);color:#fff;padding:16px}
header .row{display:flex;gap:12px;align-items:center;justify-content:space-between;flex-wrap:wrap}
header h1{font-size:20px;margin:0}
header .meta{font-size:13px;opacity:.9}
.container{padding:16px;max-width:1200px;margin:0 auto}
.toolbar{display:flex;gap:12px;align-items:center;flex-wrap:wrap;margin-bottom:16px}
.toolbar select,.toolbar button{padding:8px 10px;border-radius:10px;border:1px solid #e5e7eb;background:#fff}
.grid{display:grid;gap:16px}
@media(min-width:700px){.grid{grid-template-columns:repeat(4,1fr)}}
.card{background:var(--card);border-radius:14px;box-shadow:0 6px 16px rgba(0,0,0,.06);padding:14px;transition: all 0.3s ease;}
.card h3{margin:.25rem 0 .5rem;font-size:16px;color:var(--muted);font-weight:600}
.value{font-size:28px;font-weight:700}
.range{font-size:12px;color:var(--muted)}
.status{margin-top:6px;font-weight:700}
.status.normal{color:var(--ok)}
.status.warning{color:var(--warn)}
.status.alert{color:var(--bad)}
.chart-wrap{height:280px}
canvas{background:#fff;border-radius:14px;box-shadow:0 6px 16px rgba(0,0,0,.06);padding:8px}
table{width:100%;border-collapse:collapse;background:#fff;border-radius:14px;overflow:hidden;box-shadow:0 6px 16px rgba(0,0,0,.06)}
thead th{background:var(--brand);color:#fff;text-align:left;padding:10px;font-weight:600}
tbody td{border-top:1px solid #e5e7eb;padding:8px}
.settings{background:#fff;border-radius:14px;box-shadow:0 6px 16px rgba(0,0,0,.06);padding:14px}
.settings h3{margin:0 0 8px}
.settings .cols{display:grid;gap:12px}
@media(min-width:700px){.settings .cols{grid-template-columns:repeat(2,1fr)}}
.field{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
.field label{min-width:130px;font-size:14px;color:var(--muted)}
.field input,.field select{flex:1;min-width:80px;padding:8px;border:1px solid #e5e7eb;border-radius:10px;background:#fff}
.btn-row{display:flex;gap:10px;justify-content:flex-end;margin-top:10px}
.btn{padding:10px 14px;border-radius:12px;border:1px solid #e5e7eb;background:#fff;cursor:pointer}
.btn.primary{background:#111827;color:#fff;border-color:#111827}
.badge{padding:4px 8px;border-radius:999px;font-size:12px}
.badge.auto{background:#e0f2fe;color:#075985}
.badge.manual{background:#fef3c7;color:#92400e}
/* Login/Register */
#authContainer {max-width:400px;margin:80px auto;background:#fff;padding:20px;border-radius:12px;box-shadow:0 6px 16px rgba(0,0,0,0.1);}
#authContainer h2{text-align:center;margin-bottom:20px;color:#2c3e50;}
.tabs {display:flex;margin-bottom:16px;cursor:pointer;}
.tab {flex:1;padding:10px;text-align:center;background:#e5e7eb;border-radius:10px 10px 0 0;color:#1f2937;font-weight:600;}
.tab.active {background:#2c3e50;color:#fff;}
#authContainer form {display:flex;flex-direction:column;gap:12px;}
#authContainer input {padding:10px;border-radius:8px;border:1px solid #cbd5e1;}
#authContainer button {padding:10px;border:none;border-radius:8px;background:#111827;color:#fff;font-weight:600;cursor:pointer;}
.switch {text-align:center;margin-top:10px;font-size:13px;color:#6b7280;cursor:pointer;}

/* Login / Registration Form */
#authContainer {
  max-width: 400px;
  margin: 80px auto;
  background: linear-gradient(135deg, #e0f7fa, #ffffff); /* Subtle gradient background */
  padding: 20px;
  border-radius: 12px;
  box-shadow: 0 6px 20px rgba(0, 0, 0, 0.15);
}

/* Dashboard Background */
body {
  margin: 0;
  font-family: 'Segoe UI', system-ui, -apple-system, Roboto, Helvetica, Arial, sans-serif;
  background: linear-gradient(180deg, #f0f4f8, #d9e2ec); /* Soft gradient */
  color: var(--ink);
}

/* Header background */
header {
  background: linear-gradient(90deg, #256d85, #2c7a99);
  color: #fff;
  padding: 16px;
}

/* Cards */
.card {
  transition: all 0.3s ease;
  box-shadow: 0 6px 20px rgba(0, 0, 0, 0.06);
}

/* Settings panel */
.settings {
  background: linear-gradient(135deg, #ffffff, #f1f5f9);
  border-radius: 14px;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.06);
  padding: 14px;
}
/* Table */
table {
  width: 100%;
  border-collapse: collapse;
  border-radius: 14px;
  overflow: hidden;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.06);
  background: linear-gradient(135deg, #ffffff, #e6f0fa); /* subtle gradient */
}

thead th {
  background: linear-gradient(90deg, #256d85, #2c7a99); /* header gradient */
  color: #fff;
  text-align: left;
  padding: 10px;
  font-weight: 600;
}

tbody td {
  border-top: 1px solid #d1e3f0;
  padding: 8px;
}

/* Chart canvas */
canvas {
  background: linear-gradient(135deg, #ffffff, #e6f0fa); /* subtle gradient */
  border-radius: 14px;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.06);
  padding: 8px;
}

/* Settings panel */

.settings {
  background: linear-gradient(135deg, #f9fafb, #e6f0fa); /* soft gradient */
  border-radius: 14px;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.06);
  padding: 14px;  }

</style>
</head>
<body>

<!-- Login / Registration -->
<div class="container" id="authContainer">
  <h2>Water Quality Monitoring</h2>
  <div class="tabs">
    <div class="tab active" id="loginTab">Login</div>
    <div class="tab" id="registerTab">Register</div>
  </div>
  <form id="loginForm">
    <input type="text" id="loginUsername" placeholder="Username" required>
    <input type="password" id="loginPassword" placeholder="Password" required>
    <select id="loginRole">
      <option value="user">User</option>
      <option value="admin">Admin</option>
    </select>
    <button type="submit">Login</button>
  </form>
  <form id="registerForm" style="display:none;">
    <input type="text" id="regUsername" placeholder="Username" required>
    <input type="password" id="regPassword" placeholder="Password" required>
    <select id="regRole">
      <option value="user">User</option>
      <option value="admin">Admin</option>
    </select>
    <button type="submit">Register</button>
    <div class="switch" id="backToLogin">Already have an account? Login</div>
  </form>
</div>

<!-- Dashboard -->
<div id="dashboard" style="display:none;">
<header>
  <div class="row">
    <h1>Water Quality Monitoring</h1>
    <div class="meta">
      <span>Usage Area:</span>
      <select id="usageArea">
        <option value="drinking">Drinking</option>
        <option value="agriculture">Agriculture</option>
        <option value="industrial">Industrial</option>
        <option value="irrigation">Irrigation</option>
      </select>
      <span style="margin-left:10px">| <span id="datetime"></span></span>
    </div>
  </div>
</header>

<div class="container">
  <div class="toolbar" id="toolbar">
    <label for="modeSelect">Mode:</label>
    <select id="modeSelect">
      <option value="auto">Auto (Safe Ranges)</option>
      <option value="manual">Manual (Custom Thresholds)</option>
    </select>
    <span id="modeBadge" class="badge auto">Auto</span>
  </div>

  <div class="grid">
    <div class="card" id="card-ph">
      <h3>pH</h3>
      <div class="value" id="phValue">--</div>
      <div class="range" id="phRange">Safe: 6.5–8.5</div>
      <div class="status" id="phStatus">--</div>
    </div>
    <div class="card" id="card-tds">
      <h3>TDS (ppm)</h3>
      <div class="value" id="tdsValue">--</div>
      <div class="range" id="tdsRange">Safe: 0–500</div>
      <div class="status" id="tdsStatus">--</div>
    </div>
    <div class="card" id="card-turbidity">
      <h3>Turbidity (NTU)</h3>
      <div class="value" id="turbidityValue">--</div>
      <div class="range" id="turbidityRange">Safe: 0–5</div>
      <div class="status" id="turbidityStatus">--</div>
    </div>
    <div class="card" id="card-temp">
      <h3>Temperature (°C)</h3>
      <div class="value" id="tempValue">--</div>
      <div class="range" id="tempRange">Safe: 15–35</div>
      <div class="status" id="tempStatus">--</div>
    </div>
  </div>

  <!-- Chart & Table only for Admin -->
  <div class="chart-wrap">
    <canvas id="trendChart"></canvas>
  </div>

  <div style="margin-top:16px">
    <table>
      <thead>
        <tr>
          <th>Timestamp</th>
          <th>pH</th>
          <th>TDS</th>
          <th>Turbidity</th>
          <th>Temp (°C)</th>
          <th>Status</th>
        </tr>
      </thead>
      <tbody id="historyTable"></tbody>
    </table>
  </div>

  <div class="settings" id="settingsDiv">
    <h3>Threshold & Mode Settings</h3>
    <div class="cols">
      <div class="field"><label>pH</label>
        <input id="phThresholdMin" type="number" step="0.1" placeholder="Min"> 
        <span>–</span>
        <input id="phThresholdMax" type="number" step="0.1" placeholder="Max">
      </div>
      <div class="field"><label>TDS (ppm)</label>
        <input id="tdsThresholdMin" type="number" placeholder="Min">
        <span>–</span>
        <input id="tdsThresholdMax" type="number" placeholder="Max">
      </div>
      <div class="field"><label>Turbidity (NTU)</label>
        <input id="turbidityThresholdMin" type="number" placeholder="Min">
        <span>–</span>
        <input id="turbidityThresholdMax" type="number" placeholder="Max">
      </div>
      <div class="field"><label>Temperature (°C)</label>
        <input id="tempThresholdMin" type="number" placeholder="Min">
        <span>–</span>
        <input id="tempThresholdMax" type="number" placeholder="Max">
      </div>
    </div>
    <div class="btn-row">
      <button class="btn" id="resetBtn">Reset</button>
      <button class="btn primary" id="saveBtn">Save Settings</button>
    </div>
    <small style="display:block;margin-top:8px;color:var(--muted)">
      Auto mode locks inputs to safe ranges.
      Manual mode lets you edit and save custom thresholds.
      Values persist in localStorage.
    </small>
  </div>
</div>
</div>

<script>
/* ------------------- Auth ------------------- */
const loginTab = document.getElementById('loginTab');
const registerTab = document.getElementById('registerTab');
const loginForm = document.getElementById('loginForm');
const registerForm = document.getElementById('registerForm');
const backToLogin = document.getElementById('backToLogin');

loginTab.onclick = ()=>{ loginForm.style.display='flex'; registerForm.style.display='none'; loginTab.classList.add('active'); registerTab.classList.remove('active'); }
registerTab.onclick = ()=>{ loginForm.style.display='none'; registerForm.style.display='flex'; registerTab.classList.add('active'); loginTab.classList.remove('active'); }
backToLogin.onclick = ()=>{ loginTab.click(); }

let users = JSON.parse(localStorage.getItem('users')||'{}');
let currentRole = 'user';

registerForm.onsubmit = (e)=>{
  e.preventDefault();
  const u=document.getElementById('regUsername').value;
  const p=document.getElementById('regPassword').value;
  const r=document.getElementById('regRole').value;
  if(users[u]) { alert('Username exists'); return; }
  users[u]={password:p, role:r};
  localStorage.setItem('users', JSON.stringify(users));
  alert('Registered successfully!');
  loginTab.click();
};

loginForm.onsubmit = (e)=>{
  e.preventDefault();
  const u=document.getElementById('loginUsername').value;
  const p=document.getElementById('loginPassword').value;
  const r=document.getElementById('loginRole').value;
  if(!users[u] || users[u].password!==p || users[u].role!==r){ alert('Invalid credentials'); return; }
  currentRole = r;
  document.getElementById('authContainer').style.display='none';
  document.getElementById('dashboard').style.display='block';
  applyRoleVisibility();
  applyModeUI();
};

/* ------------------- Role-based visibility ------------------- */
const settingsDiv = document.getElementById('settingsDiv');
const toolbar = document.getElementById('toolbar');

function applyRoleVisibility(){
  if(currentRole === 'user'){
    settingsDiv.style.display = 'block';
    toolbar.style.display = 'flex';
    document.getElementById('trendChart').parentElement.style.display='none';
    document.getElementById('historyTable').parentElement.parentElement.style.display='none';
  }else{
    settingsDiv.style.display = 'block';
    toolbar.style.display = 'flex';
    document.getElementById('trendChart').parentElement.style.display='block';
    document.getElementById('historyTable').parentElement.parentElement.style.display='block';
  }
}

/* ------------------- Dashboard logic ------------------- */
function updateDateTime(){ document.getElementById('datetime').textContent=new Date().toLocaleString(); }
setInterval(updateDateTime,60000); updateDateTime();

const autoThresholdsByArea = {
  drinking: { ph:[6.5,8.5], tds:[0,500], turbidity:[0,5], temp:[15,35] },
  agriculture: { ph:[6.0,9.0], tds:[0,1000], turbidity:[0,10], temp:[10,40] },
  industrial: { ph:[6.0,9.5], tds:[0,1500], turbidity:[0,15], temp:[5,45] },
  irrigation : {ph:[6.5, 8.5], tds:[300, 500], turbidity:[0, 25], temp:[20, 32]}
};

const modeSelect = document.getElementById('modeSelect');
const usageArea = document.getElementById('usageArea');
const modeBadge = document.getElementById('modeBadge');

function loadManual(){ const saved = localStorage.getItem('manualThresholds'); if(saved){ try { return JSON.parse(saved); } catch(e){ return {}; } } return {}; }
function saveManual(obj){ localStorage.setItem('manualThresholds', JSON.stringify(obj)); }
let manualThresholds = loadManual();

function currentAuto(){ return autoThresholdsByArea[usageArea.value]; }
function getActiveThresholds(){ return modeSelect.value==='auto'?currentAuto():manualThresholds[usageArea.value]||currentAuto(); }
function setInputsEnabled(enabled){ document.querySelectorAll('.settings input').forEach(inp=>{inp.disabled = !enabled;}); }
function fillInputsFromThresholds(th){
  document.getElementById('phThresholdMin').value=th.ph[0]; document.getElementById('phThresholdMax').value=th.ph[1];
  document.getElementById('tdsThresholdMin').value=th.tds[0]; document.getElementById('tdsThresholdMax').value=th.tds[1];
  document.getElementById('turbidityThresholdMin').value=th.turbidity[0]; document.getElementById('turbidityThresholdMax').value=th.turbidity[1];
  document.getElementById('tempThresholdMin').value=th.temp[0]; document.getElementById('tempThresholdMax').value=th.temp[1];
}
function updateRangeLabels(th){
  document.getElementById('phRange').textContent=`Safe: ${th.ph[0]}–${th.ph[1]}`;
  document.getElementById('tdsRange').textContent=`Safe: ${th.tds[0]}–${th.tds[1]}`;
  document.getElementById('turbidityRange').textContent=`Safe: ${th.turbidity[0]}–${th.turbidity[1]}`;
  document.getElementById('tempRange').textContent=`Safe: ${th.temp[0]}–${th.temp[1]}`;
}
function applyModeUI(){
  const isAuto = modeSelect.value==='auto';
  modeBadge.textContent=isAuto?'Auto':'Manual';
  modeBadge.className='badge '+(isAuto?'auto':'manual');
  setInputsEnabled(true); // allow user to edit thresholds
  const th=getActiveThresholds();
  fillInputsFromThresholds(th);
  updateRangeLabels(th);
}

modeSelect.addEventListener('change',()=>{ applyModeUI(); });
usageArea.addEventListener('change',()=>{ applyModeUI(); });

document.getElementById('saveBtn').addEventListener('click',()=>{
  manualThresholds[usageArea.value]={
    ph:[parseFloat(document.getElementById('phThresholdMin').value), parseFloat(document.getElementById('phThresholdMax').value)],
    tds:[parseFloat(document.getElementById('tdsThresholdMin').value), parseFloat(document.getElementById('tdsThresholdMax').value)],
    turbidity:[parseFloat(document.getElementById('turbidityThresholdMin').value), parseFloat(document.getElementById('turbidityThresholdMax').value)],
    temp:[parseFloat(document.getElementById('tempThresholdMin').value), parseFloat(document.getElementById('tempThresholdMax').value)]
  };
  saveManual(manualThresholds);
  alert('Saved!');
  applyModeUI();
});
document.getElementById('resetBtn').addEventListener('click',()=>{
  if(currentRole !== 'admin'){ alert('Only Admin can reset'); return; }
  localStorage.removeItem('manualThresholds'); manualThresholds={}; applyModeUI();
});

/* ------------------- Chart.js ------------------- */
const ctx = document.getElementById('trendChart').getContext('2d');
const trendChart = new Chart(ctx, {type:'line',data:{labels:[],datasets:[
  {label:'pH',borderColor:'blue',data:[]},
  {label:'TDS',borderColor:'green',data:[]},
  {label:'Turbidity',borderColor:'orange',data:[]},
  {label:'Temp',borderColor:'red',data:[]}
]},options:{responsive:true,interaction:{mode:'index',intersect:false},scales:{y:{beginAtZero:true}}}});

/* ------------------- Simulate sensor data ------------------- */
const historyTable=document.getElementById('historyTable');
const cards = {
  ph: document.getElementById('card-ph'),
  tds: document.getElementById('card-tds'),
  turbidity: document.getElementById('card-turbidity'),
  temp: document.getElementById('card-temp')
};

function randomValue(min,max){ return (Math.random()*(max-min)+min).toFixed(2); }
function getStatus(val,[min,max]){
  if(val<min || val>max) return 'alert';
  if(val<min+((max-min)*0.2) || val>max-((max-min)*0.2)) return 'warning';
  return 'normal';
}
function combinedStatus(statuses){
  if(statuses.includes('alert')) return 'alert';
  if(statuses.includes('warning')) return 'warning';
  return 'normal';
}

function simulateData(){
  const th=getActiveThresholds();
  const data={
    timestamp:new Date().toLocaleTimeString(),
    ph:parseFloat(randomValue(th.ph[0]-1,th.ph[1]+1)),
    tds:parseFloat(randomValue(th.tds[0]-50,th.tds[1]+50)),
    turbidity:parseFloat(randomValue(th.turbidity[0]-2,th.turbidity[1]+2)),
    temp:parseFloat(randomValue(th.temp[0]-5,th.temp[1]+5))
  };
  const status = {
    ph:getStatus(data.ph,th.ph),
    tds:getStatus(data.tds,th.tds),
    turbidity:getStatus(data.turbidity,th.turbidity),
    temp:getStatus(data.temp,th.temp)
  };
  const overall = combinedStatus(Object.values(status));

  document.getElementById('phValue').textContent=data.ph;
  document.getElementById('tdsValue').textContent=data.tds;
  document.getElementById('turbidityValue').textContent=data.turbidity;
  document.getElementById('tempValue').textContent=data.temp;

  document.getElementById('phStatus').textContent=status.ph; document.getElementById('phStatus').className='status '+status.ph;
  document.getElementById('tdsStatus').textContent=status.tds; document.getElementById('tdsStatus').className='status '+status.tds;
  document.getElementById('turbidityStatus').textContent=status.turbidity; document.getElementById('turbidityStatus').className='status '+status.turbidity;
  document.getElementById('tempStatus').textContent=status.temp; document.getElementById('tempStatus').className='status '+status.temp;

  // Card background based on overall status
  Object.keys(cards).forEach(key=>{
    let bg;
    if(status[key]==='alert') bg='rgba(220,38,38,0.2)';
    else if(status[key]==='warning') bg='rgba(245,158,11,0.2)';
    else bg='rgba(22,163,74,0.15)';
    cards[key].style.background=bg;
  });

  // Update chart & table only if Admin
  if(currentRole==='admin'){
    trendChart.data.labels.push(data.timestamp);
    trendChart.data.datasets[0].data.push(data.ph);
    trendChart.data.datasets[1].data.push(data.tds);
    trendChart.data.datasets[2].data.push(data.turbidity);
    trendChart.data.datasets[3].data.push(data.temp);
    if(trendChart.data.labels.length>20){ trendChart.data.labels.shift(); trendChart.data.datasets.forEach(ds=>ds.data.shift()); }
    trendChart.update();

    const row=document.createElement('tr');
    row.innerHTML=`<td>${data.timestamp}</td><td>${data.ph}</td><td>${data.tds}</td><td>${data.turbidity}</td><td>${data.temp}</td><td>${overall}</td>`;
    historyTable.prepend(row);
  }
}

setInterval(simulateData,3000);
</script>
</body>
</html>
)=====";
