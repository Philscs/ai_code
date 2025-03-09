// Erforderliche Bibliotheken installieren: npm install ws node-fetch express
const WebSocket = require('ws');
const fetch = require('node-fetch');
const express = require('express');

// Initialisiere den Express-Server für das Frontend
const app = express();
app.use(express.static('public'));
const server = app.listen(3000, () => console.log('Frontend läuft auf http://localhost:3000'));

// WebSocket-Server für Echtzeit-Daten
const wss = new WebSocket.Server({ port: 8080 });

// Simulierter Datenstrom
function generateData() {
    return {
        timestamp: Date.now(),
        value: Math.random() * 100
    };
}

setInterval(() => {
    const data = generateData();
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(data));
        }
    });
}, 1000);

// Stream-Processing und Aggregation
let dataBuffer = [];
const timeWindow = 5000; // 5 Sekunden
const aggregationInterval = 1000; // 1 Sekunde

function calculateMetrics(data) {
    const values = data.map(d => d.value);
    return {
        average: values.reduce((sum, val) => sum + val, 0) / values.length,
        max: Math.max(...values),
        min: Math.min(...values)
    };
}

setInterval(() => {
    const now = Date.now();
    const windowData = dataBuffer.filter(d => d.timestamp > now - timeWindow);
    const metrics = calculateMetrics(windowData);
    console.log('Aggregierte Metriken:', metrics);

    // Alert generieren, falls Schwellenwert überschritten
    if (metrics.max > 90) {
        console.log('ALERT: Wert überschreitet 90!');
    }

    dataBuffer = windowData; // Veraltete Daten entfernen
}, aggregationInterval);

// WebSocket für Datenempfang
wss.on('connection', ws => {
    ws.on('message', message => {
        const data = JSON.parse(message);
        dataBuffer.push(data);
    });
});

// Frontend: public/index.html
// In public/index.html platzieren:
/*
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Real-Time Analytics</title>
    <script src="https://d3js.org/d3.v7.min.js"></script>
</head>
<body>
    <h1>Echtzeit-Analytics</h1>
    <div id="chart"></div>
    <script>
        const ws = new WebSocket('ws://localhost:8080');
        const data = [];
        const chartWidth = 800;
        const chartHeight = 400;

        const svg = d3.select('#chart')
            .append('svg')
            .attr('width', chartWidth)
            .attr('height', chartHeight);

        ws.onmessage = event => {
            const point = JSON.parse(event.data);
            data.push(point);

            // Maximal 50 Datenpunkte anzeigen
            if (data.length > 50) data.shift();

            const xScale = d3.scaleTime()
                .domain([Date.now() - 5000, Date.now()])
                .range([0, chartWidth]);

            const yScale = d3.scaleLinear()
                .domain([0, 100])
                .range([chartHeight, 0]);

            svg.selectAll('circle')
                .data(data)
                .join('circle')
                .attr('cx', d => xScale(d.timestamp))
                .attr('cy', d => yScale(d.value))
                .attr('r', 5);
        };
    </script>
</body>
</html>
*/
