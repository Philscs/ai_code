// analytics_processor.js
const express = require('express');
const app = express();
const server = require('http').createServer(app);
const io = require('socket.io')(server);

// Datenbank für die Analysen (in diesem Fall ein einfacher Object-Array)
let analytics = [];

// Funktion zum Hinzufügen von neuen Analysedaten
function addData(data) {
  analytics.push(data);
}

// Funktion zur Zeitfensteraggregation
function aggregateByTimeWindow(timeWindow, metric) {
  let aggregatedData = {};
  for (let i = 0; i < analytics.length; i++) {
    const timestamp = analytics[i].timestamp;
    if (!aggregatedData[metric]) {
      aggregatedData[metric] = { sum: 0, count: 0 };
    }
    aggregatedData[metric].sum += analytics[i][metric];
    aggregatedData[metric].count++;
  }
  for (const metric in aggregatedData) {
    const sum = aggregatedData[metric].sum;
    aggregatedData[metric].average = sum / aggregatedData[metric].count;
  }
  return aggregatedData;
}

// Funktion zur Berechnung von Metriken
function calculateMetrics(data, metrics) {
  let calculatedMetrics = {};
  for (const metric in metrics) {
    const metricValue = data[metric];
    if (metrics[metric] === 'sum') {
      calculatedMetrics[metric] = sumOfValues(data, metric);
    } else if (metrics[metric] === 'average') {
      calculatedMetrics[metric] = averageOfValues(data, metric);
    }
  }
  return calculatedMetrics;
}

// Funktion zur Erzeugung von Benachrichtigungen
function generateAlerts(calculatedMetrics) {
  const alerts = [];
  for (const metric in calculatedMetrics) {
    if (calculatedMetrics[metric].value > threshold) {
      alerts.push(metric);
    }
  }
  return alerts;
}

// Funktion zur Summe der Werte einer Liste
function sumOfValues(data, metric) {
  let sum = 0;
  for (const item of data) {
    if (item[metric] !== null && item[metric] !== undefined) {
      sum += item[metric];
    }
  }
  return sum;
}

// Funktion zur Durchschnittswert eines Wertes einer Liste
function averageOfValues(data, metric) {
  let sum = 0;
  for (const item of data) {
    if (item[metric] !== null && item[metric] !== undefined) {
      sum += item[metric];
    }
  }
  return sum / data.length;
}

// Funktion zur Data-Visualization
function visualizeData(calculatedMetrics) {
  const svg = d3.select('#visualization')
    .append('svg width="800" height="600"')
    .append('g transform="translate(400, 300)"');
  
  for (const metric in calculatedMetrics) {
    const metricValue = calculatedMetrics[metric];
    svg.append('rect')
      .attr('x', metricValue.value)
      .attr('y', -metricValue.value);
  }
}

// Funktion zur Erzeugung von Benachrichtigungen
function displayAlerts(alerts) {
  const alertElement = document.getElementById('alerts');
  alerts.forEach((alert) => {
    alertElement.innerHTML += `<p>Benachrichtigung: ${alert}</p>`;
  });
}

// Händeler für die Socket-IO-Konversations
io.on('connection', (socket) => {
  console.log('Neue Verbindung hergestellt');

  // Funktion zum Empfangen von neuen Analysedaten
  socket.on('new-data', (data) => {
    addData(data);
    socket.emit('updated-analytics', analytics);
  });

  // Funktion zum Lösen der Zeitfensteraggregation
  socket.on('aggregate-by-time-window', (timeWindow, metric) => {
    const aggregatedData = aggregateByTimeWindow(timeWindow, metric);
    socket.emit('aggregated-data', aggregatedData);
  });

  // Funktion zum Lösen der Metrikenberechnung
  socket.on('calculate-metrics', (data, metrics) => {
    const calculatedMetrics = calculateMetrics(data, metrics);
    socket.emit('calculated-metrics', calculatedMetrics);
  });

  // Funktion zum Lösen der Benachrichtigungserzeugung
  socket.on('generate-alerts', (calculatedMetrics) => {
    const alerts = generateAlerts(calculatedMetrics);
    displayAlerts(alerts);
  });
});

server.listen(3000, () => {
  console.log('Server läuft auf Port 3000');
});
