const char webpageCode[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 ADC6 Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; background: #f0f2f5; padding: 20px; color: #333; }
        .container { max-width: 900px; margin: auto; background: white; padding: 25px; border-radius: 15px; box-shadow: 0 10px 25px rgba(0,0,0,0.1); }
        .control-panel { display: flex; justify-content: center; gap: 15px; flex-wrap: wrap; margin-bottom: 20px; padding: 15px; background: #fafafa; border-radius: 10px; border: 1px solid #eee; }
        .btn { border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; font-weight: bold; transition: 0.3s; }
        .btn-clear { background: #ff5252; color: white; }
        .btn-csv { background: #4caf50; color: white; }
        .btn:hover { opacity: 0.8; }
        .data-table-wrapper { max-height: 300px; overflow-y: auto; margin-top: 20px; border: 1px solid #ddd; border-radius: 8px; }
        .data-table { width: 100%; border-collapse: collapse; }
        .data-table th { background-color: #9c27b0; color: white; position: sticky; top: 0; padding: 12px; }
        .data-table td { padding: 10px; border-bottom: 1px solid #eee; text-align: center; }
        #v-display { font-size: 4em; color: #9c27b0; font-weight: bold; margin: 10px 0; text-shadow: 2px 2px 4px rgba(0,0,0,0.1); }
        input { padding: 5px; border: 1px solid #ccc; border-radius: 4px; width: 70px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ADC 6 (GPIO 6) Monitor</h1>
        <div id="v-display">0.00V</div>
        
        <div class="control-panel">
            <div>
                <label>Max Samples: </label>
                <input type="number" id="maxPoints" value="50" min="10" max="1000">
            </div>
            <div>
                <label>Interval (ms): </label>
                <input type="number" id="logInterval" value="500" min="50" step="50">
            </div>
            <button class="btn btn-clear" onclick="clearData()">Clear Data</button>
            <button class="btn btn-csv" onclick="downloadCSV()">Download CSV</button>
        </div>

        <canvas id="adcChart" width="400" height="180"></canvas>

        <div class="data-table-wrapper">
            <table class="data-table">
                <thead>
                    <tr>
                        <th>Timestamp</th>
                        <th>Voltage (V)</th>
                    </tr>
                </thead>
                <tbody id="tableBody"></tbody>
            </table>
        </div>
    </div>

    <script>
        var socket = new WebSocket('ws://' + window.location.hostname + ':81/');
        var adcData = [];
        var labels = [];
        var lastLogTime = 0;

        const ctx = document.getElementById('adcChart').getContext('2d');
        const chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'ADC Voltage (V)',
                    data: adcData,
                    borderColor: '#9c27b0',
                    backgroundColor: 'rgba(156, 39, 176, 0.1)',
                    fill: true,
                    tension: 0.4,
                    pointRadius: 2
                }]
            },
            options: { 
                responsive: true, 
                scales: { y: { min: 0, max: 3.3, title: { display: true, text: 'Voltage (V)' } } },
                animation: { duration: 0 } 
            }
        });

        socket.onmessage = function(event) {
            var obj = JSON.parse(event.data);
            var v = parseFloat(obj.adc);
            document.getElementById('v-display').innerHTML = v.toFixed(2) + "V";

            var now = Date.now();
            var interval = parseInt(document.getElementById('logInterval').value);
            
            if (now - lastLogTime >= interval) {
                addData(v);
                lastLogTime = now;
            }
        };

        function addData(value) {
            var max = parseInt(document.getElementById('maxPoints').value);
            var timestamp = new Date().toLocaleTimeString();

            if (adcData.length >= max) {
                adcData.shift();
                labels.shift();
                var table = document.getElementById('tableBody');
                table.deleteRow(table.rows.length - 1);
            }

            adcData.push(value);
            labels.push(timestamp);
            chart.update();

            var table = document.getElementById('tableBody');
            var row = table.insertRow(0);
            row.insertCell(0).innerHTML = timestamp;
            row.insertCell(1).innerHTML = value.toFixed(3);
        }

        function clearData() {
            adcData.length = 0;
            labels.length = 0;
            document.getElementById('tableBody').innerHTML = "";
            chart.update();
        }

        function downloadCSV() {
            let csvContent = "data:text/csv;charset=utf-8,Timestamp,Voltage(V)\n";
            
            // ดึงข้อมูลจากอาเรย์ล่าสุด
            for(let i = 0; i < adcData.length; i++) {
                csvContent += labels[i] + "," + adcData[i] + "\n";
            }

            const encodedUri = encodeURI(csvContent);
            const link = document.createElement("a");
            const dateStr = new Date().toISOString().slice(0,10);
            
            link.setAttribute("href", encodedUri);
            link.setAttribute("download", "ADC_Log_" + dateStr + ".csv");
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }
    </script>
</body>
</html>
)=====";