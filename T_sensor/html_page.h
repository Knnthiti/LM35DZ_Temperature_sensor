const char webpageCode[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32-S3 Web Scope</title>
  <meta charset="UTF-8">
  <style>
    body { background: #111; color: #0f0; font-family: 'Courier New', monospace; text-align: center; }
    canvas { background: #000; border: 2px solid #333; width: 90%; max-width: 800px; }
    .data { font-size: 2em; margin: 20px; }
  </style>
</head>
<body>
  <h1>ESP32-S3 Real-time Scope</h1>
  <div class="data">Voltage: <span id="v_text">0.00</span> V</div>
  <canvas id="scopeCanvas" width="800" height="400"></canvas>

  <script>
    var canvas = document.getElementById('scopeCanvas');
    var ctx = canvas.getContext('2d');
    var dataPoints = new Array(100).fill(400);
    var ws = new WebSocket('ws://' + window.location.hostname + ':81/');

    ws.onmessage = function(evt) {
      var obj = JSON.parse(evt.data);
      document.getElementById('v_text').innerHTML = obj.val.toFixed(2);
      
      // คำนวณจุดกราฟ (Scale เข้าหาความสูง Canvas)
      var y = 400 - (obj.raw / 4095 * 350);
      dataPoints.push(y);
      if(dataPoints.length > 100) dataPoints.shift();
      draw();
    };

    function draw() {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      // วาด Grid
      ctx.strokeStyle = '#222';
      for(var i=0; i<8; i++) {
        ctx.beginPath(); ctx.moveTo(0, i*50); ctx.lineTo(800, i*50); ctx.stroke();
      }
      // วาดเส้นกราฟ
      ctx.strokeStyle = '#0f0';
      ctx.lineWidth = 3;
      ctx.beginPath();
      for(var i=0; i<dataPoints.length; i++) {
        var x = (800 / 100) * i;
        if(i==0) ctx.moveTo(x, dataPoints[i]);
        else ctx.lineTo(x, dataPoints[i]);
      }
      ctx.stroke();
    }
  </script>
</body>
</html>
)=====";