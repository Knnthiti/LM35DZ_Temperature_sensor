#ifndef HTML_PAGE_H
#define HTML_PAGE_H

#include <Arduino.h>

const char webpageCode[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Temperature & ADC Monitor</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; text-align: center; background: #f0f2f5; padding: 20px; color: #333; }
        .container { max-width: 900px; margin: auto; background: white; padding: 25px; border-radius: 15px; box-shadow: 0 10px 25px rgba(0,0,0,0.1); }
        .control-panel { display: flex; justify-content: center; gap: 15px; flex-wrap: wrap; margin-bottom: 20px; padding: 15px; background: #fafafa; border-radius: 10px; border: 1px solid #eee; }
        .btn { border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; font-weight: bold; transition: 0.3s; }
        .btn-clear { background: #ff5252; color: white; }
        .btn-txt { background: #2196F3; color: white; }
        .btn:hover { opacity: 0.8; }
        .data-table-wrapper { max-height: 300px; overflow-y: auto; margin-top: 20px; border: 1px solid #ddd; border-radius: 8px; }
        .data-table { width: 100%; border-collapse: collapse; }
        .data-table th { background-color: #2196F3; color: white; position: sticky; top: 0; padding: 12px; }
        .data-table td { padding: 10px; border-bottom: 1px solid #eee; text-align: center; }
        .display-box { display: flex; justify-content: center; gap: 40px; }
        .val-display { font-size: 3em; color: #2196F3; font-weight: bold; margin: 10px 0; text-shadow: 2px 2px 4px rgba(0,0,0,0.1); }
        .val-label { font-size: 0.3em; color: #666; display: block; }
        input { padding: 5px; border: 1px solid #ccc; border-radius: 4px; width: 70px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Sensor Monitor</h1>
        
        <div class="display-box">
            <div class="val-display" id="t-display">0.00 <span class="val-label">&deg;C</span></div>
            <div class="val-display" style="color: #9c27b0;" id="v-display">0.000 <span class="val-label">Volts</span></div>
        </div>
        
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
            <button class="btn btn-txt" onclick="downloadTXT()">Download TXT</button>
        </div>

        <canvas id="tempChart" width="400" height="180"></canvas>

        <div class="data-table-wrapper">
            <table class="data-table">
                <thead>
                    <tr>
                        <th>Date</th>
                        <th>Time</th>
                        <th>Voltage (V)</th>
                        <th>Temperature (&deg;C)</th>
                    </tr>
                </thead>
                <tbody id="tableBody"></tbody>
            </table>
        </div>
    </div>

    <script>
        var socket = new WebSocket('ws://' + window.location.hostname + ':81/');
        var dateData = [];
        var timeData = [];
        var vData = [];
        var tData = [];
        var lastLogTime = 0;

        const ctx = document.getElementById('tempChart').getContext('2d');
        const chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: timeData,
                datasets: [{
                    label: 'Temperature (°C)',
                    data: tData,
                    borderColor: '#2196F3',
                    backgroundColor: 'rgba(33, 150, 243, 0.1)',
                    fill: true,
                    tension: 0.4,
                    pointRadius: 2
                }]
            },
            options: { 
                responsive: true, 
                scales: { y: { title: { display: true, text: 'Temperature (°C)' } } },
                animation: { duration: 0 } 
            }
        });

        socket.onmessage = function(event) {
            var obj = JSON.parse(event.data);
            var v = parseFloat(obj.v);
            var t = parseFloat(obj.t);
            
            document.getElementById('v-display').innerHTML = v.toFixed(3) + " <span class='val-label'>Volts</span>";
            document.getElementById('t-display').innerHTML = t.toFixed(2) + " <span class='val-label'>&deg;C</span>";

            var now = Date.now();
            var interval = parseInt(document.getElementById('logInterval').value);
            
            if (now - lastLogTime >= interval) {
                addData(v, t);
                lastLogTime = now;
            }
        };

        function addData(v, t) {
            var max = parseInt(document.getElementById('maxPoints').value);
            
            var d = new Date();
            var dateStr = d.getDate().toString().padStart(2, '0') + "/" + 
                          (d.getMonth() + 1).toString().padStart(2, '0') + "/" + 
                          d.getFullYear();
            var timeStr = d.toLocaleTimeString('en-GB');

            if (tData.length >= max) {
                dateData.shift();
                timeData.shift();
                vData.shift();
                tData.shift();
                var table = document.getElementById('tableBody');
                table.deleteRow(table.rows.length - 1);
            }

            dateData.push(dateStr);
            timeData.push(timeStr);
            vData.push(v);
            tData.push(t);
            chart.update();

            var table = document.getElementById('tableBody');
            var row = table.insertRow(0);
            row.insertCell(0).innerHTML = dateStr;
            row.insertCell(1).innerHTML = timeStr;
            row.insertCell(2).innerHTML = v.toFixed(3);
            row.insertCell(3).innerHTML = t.toFixed(2);
        }

        function clearData() {
            dateData.length = 0;
            timeData.length = 0;
            vData.length = 0;
            tData.length = 0;
            document.getElementById('tableBody').innerHTML = "";
            chart.update();
        }

        function downloadTXT() {
            let txtContent = "data:text/plain;charset=utf-8,Date\tTime\tVoltage(V)\tTemperature(C)\n";
            
            for(let i = 0; i < tData.length; i++) {
                txtContent += dateData[i] + "\t" + timeData[i] + "\t" + vData[i].toFixed(3) + "\t" + tData[i].toFixed(2) + "\n";
            }

            const encodedUri = encodeURI(txtContent);
            const link = document.createElement("a");
            
            var d = new Date();
            const dateFileName = d.getFullYear() + "-" + 
                                 (d.getMonth() + 1).toString().padStart(2, '0') + "-" + 
                                 d.getDate().toString().padStart(2, '0');
            
            link.setAttribute("href", encodedUri);
            link.setAttribute("download", "Data_Log_" + dateFileName + ".txt");
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }
    </script>
</body>
</html>
)=====";

#endif