import React, { useState, useEffect, useCallback } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { Alert, AlertDescription } from '@/components/ui/alert';
import _ from 'lodash';

// Simulierte Datenstrom-Generator
const generateDataPoint = () => ({
  timestamp: new Date(),
  value: Math.random() * 100,
  type: Math.random() > 0.5 ? 'A' : 'B'
});

const RealtimeAnalytics = () => {
  const [rawData, setRawData] = useState([]);
  const [processedData, setProcessedData] = useState([]);
  const [alerts, setAlerts] = useState([]);
  const [metrics, setMetrics] = useState({
    average: 0,
    max: 0,
    min: 0,
    count: 0
  });

  // Stream Processing
  const processStream = useCallback((newDataPoint) => {
    setRawData(prevData => {
      const newData = [...prevData, newDataPoint];
      // Behalte nur die letzten 50 Datenpunkte
      return newData.slice(-50);
    });
  }, []);

  // Time-Window Aggregation (5-Sekunden-Fenster)
  const aggregateTimeWindow = useCallback((data) => {
    const now = new Date();
    const windowSize = 5000; // 5 Sekunden in Millisekunden
    const windowStart = new Date(now.getTime() - windowSize);

    const windowData = data.filter(point => 
      point.timestamp >= windowStart && point.timestamp <= now
    );

    return _.groupBy(windowData, 'type');
  }, []);

  // Metric Calculation
  const calculateMetrics = useCallback((data) => {
    if (data.length === 0) return;

    const values = data.map(d => d.value);
    return {
      average: _.mean(values).toFixed(2),
      max: _.max(values).toFixed(2),
      min: _.min(values).toFixed(2),
      count: values.length
    };
  }, []);

  // Alert Generation
  const checkAlerts = useCallback((metrics) => {
    if (metrics.average > 70) {
      const alert = {
        id: Date.now(),
        message: `Hoher Durchschnittswert erkannt: ${metrics.average}`,
        severity: 'warning'
      };
      setAlerts(prev => [...prev, alert].slice(-5)); // Behalte die letzten 5 Alerts
    }
  }, []);

  // Hauptverarbeitungslogik
  useEffect(() => {
    const processData = () => {
      const newDataPoint = generateDataPoint();
      processStream(newDataPoint);
      
      const aggregatedData = aggregateTimeWindow(rawData);
      const newMetrics = calculateMetrics(rawData);
      
      setProcessedData(Object.entries(aggregatedData).map(([type, data]) => ({
        type,
        count: data.length,
        avgValue: _.mean(data.map(d => d.value)).toFixed(2)
      })));
      
      setMetrics(newMetrics);
      checkAlerts(newMetrics);
    };

    const interval = setInterval(processData, 1000);
    return () => clearInterval(interval);
  }, [rawData, processStream, aggregateTimeWindow, calculateMetrics, checkAlerts]);

  return (
    <div className="p-4 space-y-6">
      <h2 className="text-2xl font-bold">Real-Time Analytics Dashboard</h2>
      
      {/* Metriken */}
      <div className="grid grid-cols-4 gap-4">
        <div className="p-4 bg-white rounded shadow">
          <h3 className="font-semibold">Durchschnitt</h3>
          <p className="text-2xl">{metrics.average}</p>
        </div>
        <div className="p-4 bg-white rounded shadow">
          <h3 className="font-semibold">Maximum</h3>
          <p className="text-2xl">{metrics.max}</p>
        </div>
        <div className="p-4 bg-white rounded shadow">
          <h3 className="font-semibold">Minimum</h3>
          <p className="text-2xl">{metrics.min}</p>
        </div>
        <div className="p-4 bg-white rounded shadow">
          <h3 className="font-semibold">Anzahl</h3>
          <p className="text-2xl">{metrics.count}</p>
        </div>
      </div>

      {/* Visualisierung */}
      <div className="h-64 bg-white rounded shadow p-4">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={rawData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis 
              dataKey="timestamp" 
              domain={['auto', 'auto']}
              tickFormatter={(time) => new Date(time).toLocaleTimeString()}
            />
            <YAxis />
            <Tooltip 
              labelFormatter={(label) => new Date(label).toLocaleTimeString()}
            />
            <Legend />
            <Line 
              type="monotone" 
              dataKey="value" 
              stroke="#8884d8" 
              dot={false}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>

      {/* Aggregierte Daten */}
      <div className="grid grid-cols-2 gap-4">
        {processedData.map(data => (
          <div key={data.type} className="p-4 bg-white rounded shadow">
            <h3 className="font-semibold">Typ {data.type}</h3>
            <p>Anzahl: {data.count}</p>
            <p>Durchschnittswert: {data.avgValue}</p>
          </div>
        ))}
      </div>

      {/* Alerts */}
      <div className="space-y-2">
        {alerts.map(alert => (
          <Alert key={alert.id} variant="warning">
            <AlertDescription>{alert.message}</AlertDescription>
          </Alert>
        ))}
      </div>
    </div>
  );
};

export default RealtimeAnalytics;