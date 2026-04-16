#ifndef INDEX_H
#define INDEX_H

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Aquality Monitor</title>
<script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
<style>
body { margin:0; font-family:'Segoe UI', sans-serif; background:#0b0b0b; color:#fff; }
.container { max-width:420px; margin:auto; padding:20px; }
.header { display:flex; align-items:center; margin-bottom:20px; }
.header h1 { font-size:24px; margin:0; }
.card { background:#1e1e1e; border-radius:10px; padding:16px; margin-bottom:12px; display:flex; flex-direction:column; align-items:center; }
.card-content { display:grid; grid-template-columns:50px auto; gap:10px; width:100%; align-items:center; }
.card-info { display:flex; flex-direction:column; }
.details-link { text-decoration:none; font-size:14px; margin-top:10px; cursor:pointer; }
.emoji { font-size:32px; text-align:center; }
.gauge-container { width:300px; height:150px; margin:10px auto 0; display:none; position:relative; }
.status-dot {
  width: 14px;
  height: 14px;
  border-radius: 50%;
  margin-left: 10px;
  background: red; /* mặc định đỏ */
}

</style>
</head>
<body>
<div class="container">
<div class="header">
  <span class="emoji">⏲️</span>
  <h1>Aquality Monitor</h1>
  <div id="uartStatus" class="status-dot"></div>
</div>

<!-- pH Card -->
<div class="card">
  <div class="card-content"><span class="emoji">🧪</span>
    <div class="card-info"><strong>pH</strong><span>%PH%</span></div>
  </div>
  <span class="details-link" id="phDetails" onclick="toggleGauge('phGauge','ph')">Details</span>
  <div id="phGauge" class="gauge-container"></div>
</div>

<!-- TDS Card -->
<div class="card">
  <div class="card-content"><span class="emoji">🧂</span>
    <div class="card-info"><strong>TDS</strong><span>%TDS% ppm</span></div>
  </div>
  <span class="details-link" id="tdsDetails" onclick="toggleGauge('tdsGauge','tds')">Details</span>
  <div id="tdsGauge" class="gauge-container"></div>
</div>

<!-- Temp Card -->
<div class="card">
  <div class="card-content"><span class="emoji">🌡️</span>
    <div class="card-info"><strong>Nhiệt độ</strong><span>%TEMP%°C</span></div>
  </div>
  <span class="details-link" id="tempDetails" onclick="toggleGauge('tempGauge','temp')">Details</span>
  <div id="tempGauge" class="gauge-container"></div>
</div>

<!-- Turb Card -->
<div class="card">
  <div class="card-content"><span class="emoji">🌫️</span>
    <div class="card-info"><strong>Độ đục</strong><span>%TURB% NTU</span></div>
  </div>
  <span class="details-link" id="turbDetails" onclick="toggleGauge('turbGauge','turb')">Details</span>
  <div id="turbGauge" class="gauge-container"></div>
</div>

<!-- DO Card -->
<div class="card">
  <div class="card-content"><span class="emoji">∘˙○˚</span>
    <div class="card-info"><strong>Nồng độ Oxy</strong><span>%DO% mg/L</span></div>
  </div>
  <span class="details-link" id="doDetails" onclick="toggleGauge('doGauge','do')">Details</span>
  <div id="doGauge" class="gauge-container"></div>
</div>

</div>

<script type="text/javascript">
google.charts.load('current', {'packages':['gauge']});
let dataValues={ph:0,tds:0,temp:0,turb:0,do:0};
let gauges={};

function getOptions(key){
  switch(key){
    case 'ph': return {width:300,height:150,redFrom:8.5,redTo:14,yellowFrom:0,yellowTo:6.5,greenFrom:6.5,greenTo:8.5,minorTicks:1,min:0,max:14,majorTicks: ['0','2','4','6','8','10','12','14']};
    case 'tds': return {width:300,height:150,redFrom:1500,redTo:2000,yellowFrom:0,yellowTo:500,greenFrom:500,greenTo:1500,minorTicks:100,min:0,max:2000};
    case 'temp': return {width:300,height:150,redFrom:32,redTo:60,yellowFrom:0,yellowTo:28,greenFrom:28,greenTo:32,minorTicks:1,min:0,max:60};
    case 'turb': return {width:300,height:150,redFrom:80,redTo:150,yellowFrom:0,yellowTo:25,greenFrom:25,greenTo:80,minorTicks:5,min:0,max:150};
    case 'do': return {width:300,height:150,redFrom:15,redTo:20,yellowFrom:0,yellowTo:5,greenFrom:5,greenTo:15,minorTicks:1,min:0,max:20};
  }
}

function getStatus(key,value){
  let opt=getOptions(key);
  if(value>=opt.redFrom && value<=opt.redTo) return {text:'Cao quá', color:'red'};
  if(value>=opt.yellowFrom && value<=opt.yellowTo) return {text:'Thấp quá', color:'yellow'};
  if(value>=opt.greenFrom && value<=opt.greenTo) return {text:'Ổn định', color:'lime'};
  return {text:'', color:'#fff'};
}

function toggleGauge(containerId,key){
  let container=document.getElementById(containerId);
  if(container.style.display==="none"){
    container.style.display="block";
    if(!gauges[containerId]){
      google.charts.setOnLoadCallback(()=>{
        let data=google.visualization.arrayToDataTable([['Label','Value'],[key.toUpperCase(),dataValues[key]]]);
        let options=getOptions(key);
        let chart=new google.visualization.Gauge(container);
        chart.draw(data,options);
        gauges[containerId]={chart:chart,data:data,options:options};
      });
    }
  }else{container.style.display="none";}
}

function updateData(){
  fetch('/data')
    .then(res=>res.json())
    .then(d=>{
      dataValues=d;
      document.querySelectorAll('.card-info span')[0].innerText=d.ph.toFixed(2);
      document.querySelectorAll('.card-info span')[1].innerText=d.tds.toFixed(0)+' ppm';
      document.querySelectorAll('.card-info span')[2].innerText=d.temp.toFixed(1)+'°C';
      document.querySelectorAll('.card-info span')[3].innerText=d.turb.toFixed(2)+' NTU';
      document.querySelectorAll('.card-info span')[4].innerText=d.do.toFixed(2)+' mg/L';
      // Update UART status dot
let dot = document.getElementById('uartStatus');
if (d.uart === 1) {
  dot.style.background = 'lime';   // Xanh khi có UART
} else {
  dot.style.background = 'red';    // Đỏ khi mất UART
}

      
      for(let id in gauges){
        let key=id.replace('Gauge','');
        let val=dataValues[key];
        gauges[id].data.setValue(0,1,val);
        gauges[id].chart.draw(gauges[id].data,gauges[id].options);
      }

      // Update Details chữ & màu LUÔN, kể cả chưa bấm
      ['ph','tds','temp','turb','do'].forEach(key=>{
        let val=dataValues[key];
        let detailsId=key+'Details';
        let el=document.getElementById(detailsId);
        if(el){
          let status=getStatus(key,val);
          el.innerText=status.text;
          el.style.color=status.color;
        }
      });
    })
    .catch(err=>console.error(err));
}

setInterval(updateData,1000);
</script>
</body>
</html>
)rawliteral";

#endif
