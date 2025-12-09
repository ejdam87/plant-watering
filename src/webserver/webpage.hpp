const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Plant Watering System</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
      background: white;
      border-radius: 15px;
      padding: 30px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.3);
    }
    h1 {
      color: #333;
      text-align: center;
      margin-bottom: 30px;
    }
    .sensor-card {
      background: #f8f9fa;
      border-radius: 10px;
      padding: 20px;
      margin: 15px 0;
      border-left: 4px solid #667eea;
    }
    .sensor-title {
      font-size: 18px;
      font-weight: bold;
      color: #555;
      margin-bottom: 10px;
    }
    .sensor-value {
      font-size: 32px;
      font-weight: bold;
      color: #667eea;
      margin: 10px 0;
    }
    .sensor-unit {
      font-size: 16px;
      color: #888;
    }
    .control-section {
      background: #f8f9fa;
      border-radius: 10px;
      padding: 20px;
      margin: 15px 0;
      border-left: 4px solid #28a745;
    }
    .control-title {
      font-size: 18px;
      font-weight: bold;
      color: #555;
      margin-bottom: 15px;
    }
    .button-group {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
    }
    button {
      padding: 12px 24px;
      font-size: 16px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      transition: all 0.3s;
      font-weight: bold;
    }
    .btn-on {
      background: #28a745;
      color: white;
    }
    .btn-on:hover {
      background: #218838;
    }
    .btn-off {
      background: #dc3545;
      color: white;
    }
    .btn-off:hover {
      background: #c82333;
    }
    .btn-active {
      box-shadow: 0 0 10px rgba(0,0,0,0.3);
      transform: scale(1.05);
    }
    .slider-container {
      margin: 15px 0;
    }
    .slider-label {
      display: flex;
      justify-content: space-between;
      margin-bottom: 5px;
    }
    input[type="range"] {
      width: 100%;
      height: 8px;
      border-radius: 5px;
      background: #ddd;
      outline: none;
    }
    .status-indicator {
      display: inline-block;
      width: 12px;
      height: 12px;
      border-radius: 50%;
      margin-right: 8px;
    }
    .status-on {
      background: #28a745;
      box-shadow: 0 0 10px #28a745;
    }
    .status-off {
      background: #dc3545;
    }
    .last-update {
      text-align: center;
      color: #888;
      font-size: 12px;
      margin-top: 20px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Plant Watering System</h1>
    
    <div class="sensor-card">
      <div class="sensor-title">Humidity Sensor</div>
      <div class="sensor-value" id="humidity">--</div>
      <div class="sensor-unit">Analog Reading (0-4095)</div>
    </div>
    
    <div class="sensor-card">
      <div class="sensor-title">Water Level Sensor</div>
      <div class="sensor-value" id="waterLevel">--</div>
      <div class="sensor-unit">Status</div>
      <div style="margin-top: 10px; color: #666;">
        Time since last detection: <span id="waterLevelTime">--</span> ms
      </div>
    </div>
    
    <div class="control-section">
      <div class="control-title">
        <span class="status-indicator" id="motorStatusIndicator"></span>
        Motor Control
      </div>
      <div class="button-group">
        <button class="btn-on" id="btnOn" onclick="controlMotor(true)">Turn ON</button>
        <button class="btn-off" id="btnOff" onclick="controlMotor(false)">Turn OFF</button>
      </div>
      <div class="slider-container">
        <div class="slider-label">
          <span>Motor Speed:</span>
          <span id="speedValue">255</span>
        </div>
        <input type="range" id="speedSlider" min="0" max="255" value="255" 
               oninput="updateSpeed(this.value)" onchange="setSpeed(this.value)">
      </div>
    </div>
    
    <div class="last-update" id="lastUpdate"></div>
  </div>

  <script>
    let motorState = false;
    
    // Update sensor readings every second
    setInterval(updateSensors, 1000);
    updateSensors(); // Initial update
    
    function updateSensors() {
      fetch('/api/sensors')
        .then(response => response.json())
        .then(data => {
          document.getElementById('humidity').textContent = data.humidity;
          document.getElementById('waterLevel').textContent = data.water_level ? 'DETECTED' : 'NOT DETECTED';
          document.getElementById('waterLevelTime').textContent = data.water_level_time;
          document.getElementById('lastUpdate').textContent = 'Last update: ' + new Date().toLocaleTimeString();
        })
        .catch(error => console.error('Error:', error));
    }
    
    function controlMotor(state) {
      const stateStr = state ? 'true' : 'false';
      console.log('Controlling motor, state:', stateStr);
      
      fetch('/api/pump?state=' + stateStr, {
        method: 'POST'
      })
        .then(response => {
          if (!response.ok) {
            throw new Error('Network response was not ok');
          }
          return response.json();
        })
        .then(data => {
          console.log('Motor control response:', data);
          motorState = data.state === true || data.state === 'true';
          updateMotorUI();
        })
        .catch(error => {
          console.error('Error controlling motor:', error);
          alert('Error controlling motor: ' + error.message);
        });
    }
    
    function updateSpeed(value) {
      document.getElementById('speedValue').textContent = value;
    }
    
    function setSpeed(value) {
      fetch('/api/pump/speed?speed=' + value, {
        method: 'POST'
      })
        .then(response => response.json())
        .then(data => {
          console.log('Speed updated:', data.speed);
        })
        .catch(error => console.error('Error:', error));
    }
    
    function updateMotorUI() {
      const indicator = document.getElementById('motorStatusIndicator');
      const btnOn = document.getElementById('btnOn');
      const btnOff = document.getElementById('btnOff');
      
      if (motorState) {
        indicator.className = 'status-indicator status-on';
        btnOn.classList.add('btn-active');
        btnOff.classList.remove('btn-active');
      } else {
        indicator.className = 'status-indicator status-off';
        btnOff.classList.add('btn-active');
        btnOn.classList.remove('btn-active');
      }
    }
    
    // Get initial motor state
    fetch('/api/pump')
      .then(response => response.json())
      .then(data => {
        motorState = data.state === true || data.state === 'true';
        document.getElementById('speedValue').textContent = data.speed;
        document.getElementById('speedSlider').value = data.speed;
        updateMotorUI();
      })
      .catch(error => console.error('Error:', error));
  </script>
</body>
</html>
)rawliteral";
