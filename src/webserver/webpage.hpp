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
      .sensor-card, .control-section {
        background: #f8f9fa;
        border-radius: 10px;
        padding: 20px;
        margin: 15px 0;
        border-left: 4px solid #667eea;
      }
      .control-section {
        border-left-color: #28a745;
      }
      .sensor-title, .control-title {
        font-size: 18px;
        font-weight: bold;
        color: #555;
        margin-bottom: 10px;
      }
      .sensor-value {
        font-size: 32px;
        font-weight: bold;
        color: #667eea;
      }
      button {
        padding: 12px 24px;
        font-size: 16px;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        font-weight: bold;
        background: #28a745;
        color: white;
        transition: 0.3s;
      }
      button:hover {
        background: #218838;
      }
      button:disabled {
        background: #aaa;
        cursor: not-allowed;
      }
      .input-group {
        margin: 15px 0;
      }
      input[type="number"], input[type="range"] {
        width: 100%;
        padding: 8px;
        font-size: 16px;
      }
      .hint {
        color: #666;
        font-size: 13px;
        margin-top: 6px;
      }
      .status-indicator {
        display: inline-block;
        width: 12px;
        height: 12px;
        border-radius: 50%;
        margin-right: 8px;
        background: #dc3545;
      }
      .status-on {
        background: #28a745;
        box-shadow: 0 0 8px #28a745;
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
      </div>

      <div class="sensor-card">
        <div class="sensor-title">Water Level Sensor</div>
        <div class="sensor-value" id="waterLevel">--</div>
      </div>

      <div class="control-section">
        <div class="control-title">
          <span class="status-indicator" id="cycleIndicator"></span>
          Pump Cycle Control
        </div>

        <div class="input-group">
          <label>
            Auto-water when humidity below:
            <span id="thresholdValue">20</span>%
          </label>
          <input type="range"
                id="thresholdDial"
                min="0" max="100" value="20"
                oninput="updateThresholdPreview(this.value)">
          <div class="hint">
            When humidity drops below this value, the pump can run automatically.
          </div>
        </div>

        <button id="setThresholdBtn" onclick="applyThreshold()">
          Set Humidity Threshold
        </button>

        <div class="input-group">
          <label>Duration (seconds)</label>
          <input type="number" id="durationInput" min="1" value="5">
        </div>

        <button id="startCycleBtn" onclick="startPumpCycle()">
          Start Pump Cycle
        </button>

        <div class="input-group">
          <label>Motor Speed: <span id="speedValue">255</span></label>
          <input type="range"
                id="speedSlider"
                min="0" max="255" value="255"
                oninput="updateSpeed(this.value)">
        </div>

        <button id="setSpeedBtn" onclick="applySpeed()">
          Set Speed
        </button>
      </div>

      <div class="last-update" id="lastUpdate"></div>
    </div>

    <script>
      let pumpRunning = false;
      let expectedFinishTimer = null;
      const FINISH_MARGIN_MS = 1000;
      let lastKnownThreshold = 20;

      setInterval(updateSensors, 1000);
      setThresholdUI(lastKnownThreshold);
      loadThreshold();
      updateSensors();

      function loadThreshold() {
        fetch('/api/pump/threshold')
          .then(r => r.json())
          .then(d => {
            if (typeof d.threshold === 'number') {
              lastKnownThreshold = d.threshold;
              setThresholdUI(d.threshold);
            }
          })
          .catch(() => {
            // Keep defaults if settings fetch fails
          });
      }

      function updateSensors() {
        fetch('/api/sensors')
          .then(r => r.json())
          .then(d => {
            document.getElementById('humidity').textContent = parseFloat(d.humidity).toFixed(1) + '%';
            document.getElementById('waterLevel').textContent =
              d.water_level ? 'DETECTED' : 'NOT DETECTED';

            document.getElementById('lastUpdate').textContent =
              'Last update: ' + new Date().toLocaleTimeString();
          });
      }

      function startPumpCycle() {
        const duration = parseInt(
          document.getElementById('durationInput').value
        );

        if (!duration || duration <= 0) {
          alert('Enter a valid duration');
          return;
        }

        fetch('/api/pump?duration=' + duration, { method: 'POST' })
          .then(r => {
            if (!r.ok) throw new Error('Pump start failed');
            return r.json();
          })
          .then(() => {
            setUIRunning(true);

            if (expectedFinishTimer)
              clearTimeout(expectedFinishTimer);

            expectedFinishTimer = setTimeout(
              queryPumpState,
              duration * 1000 + FINISH_MARGIN_MS
            );
          })
          .catch(err => {
            alert(err.message);
            setUIRunning(false);
          });
      }

      function queryPumpState() {
        fetch('/api/pump')
          .then(r => r.json())
          .then(data => {
            setUIRunning(data.running === true);

            document.getElementById('speedSlider').value = data.speed;
            document.getElementById('speedValue').textContent = data.speed;

            if (!data.running && expectedFinishTimer) {
              clearTimeout(expectedFinishTimer);
              expectedFinishTimer = null;
            }
          })
          .catch(() => setUIRunning(false));
      }

      function setUIRunning(running) {
        pumpRunning = running;

        const indicator = document.getElementById('cycleIndicator');
        const startBtn = document.getElementById('startCycleBtn');
        const setSpeedBtn = document.getElementById('setSpeedBtn');

        indicator.className =
          'status-indicator' + (running ? ' status-on' : '');

        startBtn.disabled = running;
        setSpeedBtn.disabled = running;
      }

      function updateSpeed(value) {
        document.getElementById('speedValue').textContent = value;
      }

      function updateThresholdPreview(value) {
        document.getElementById('thresholdValue').textContent = value;
      }

      function setThresholdUI(value) {
        const v = Math.max(0, Math.min(100, parseInt(value, 10) || 0));
        document.getElementById('thresholdDial').value = v;
        document.getElementById('thresholdValue').textContent = v;
      }

      function applyThreshold() {
        const threshold = document.getElementById('thresholdDial').value;

        fetch('/api/pump/threshold?threshold=' + threshold, { method: 'POST' })
          .then(r => {
            if (!r.ok) throw new Error('Threshold update failed');
            return r.json();
          })
          .then(d => {
            if (typeof d.threshold === 'number') {
              lastKnownThreshold = d.threshold;
              setThresholdUI(d.threshold);
            }
          })
          .catch(() => alert('Threshold update failed'));
      }

      function applySpeed() {
        const speed =
          document.getElementById('speedSlider').value;

        fetch('/api/pump/speed?speed=' + speed, { method: 'POST' })
          .catch(err => alert('Speed update failed'));
      }

      // Initial authoritative sync
      queryPumpState();
    </script>
  </body>
</html>
)rawliteral";
