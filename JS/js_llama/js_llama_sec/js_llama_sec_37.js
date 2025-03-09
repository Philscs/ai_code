class PerformanceMonitor {
    constructor() {
      this.metrics = {
        pageLoadTime: null,
        requestCount: 0,
        responseSize: 0,
        dnsLookupCount: 0,
        httpConnectTimeouts: []
      };
      this.rus = {
        userId: null,
        sessionData: {}
      };
      this.sc = {
        vulnerabilities: []
      };
      this.pc = {
        userConsent: true,
        privacySettings: {}
      };
      this.ru = {
        memoryUsage: null,
        cpuUsage: null,
        diskUsage: null
      };
    }
  
    collectMetrics(event) {
      if (event.type === 'pageLoad') {
        this.metrics.pageLoadTime = event.duration;
      } else if (event.type === 'request' && event.status === 200) {
        this.metrics.requestCount++;
        this.metrics.responseSize += event.responseSize;
      } else if (event.type === 'dnsLookup') {
        this.metrics.dnsLookupCount++;
      }
    }
  
    collectRus(event) {
      if (event.userId !== this.rus.userId) {
        this.rus.userId = event.userId;
        this.rus.sessionData = {};
      }
      this.rus.sessionData[event.type] = event.value;
    }
  
    collectSc(event) {
      if (event.type === 'vulnerability') {
        this.sc.vulnerabilities.push(event.name);
      }
    }
  
    collectPc(event) {
      if (event.type === 'consent') {
        this.pc.userConsent = event.value;
      } else if (event.type === 'privacySetting') {
        this.pc.privacySettings[event.key] = event.value;
      }
    }
  
    collectRu(event) {
      if (event.type === 'memory') {
        this.ru.memoryUsage = event.value;
      } else if (event.type === 'cpu') {
        this.ru.cpuUsage = event.value;
      } else if (event.type === 'disk') {
        this.ru.diskUsage = event.value;
      }
    }
  
    saveMetrics() {
      console.log(this.metrics);
    }
  
    saveRus() {
      console.log(this.rus);
    }
  
    saveSc() {
      console.log(this.sc);
    }
  
    savePc() {
      console.log(this.pc);
    }
  
    saveRu() {
      console.log(this.ru);
    }
  
    monitorPerformance() {
      this.collectMetrics({ type: 'pageLoad', duration: 1000 });
      this.collectRus({ userId: 123, type: 'sessionStart' });
      this.collectSc({ type: 'vulnerability', name: 'SQL Injection' });
      this.collectPc({ type: 'consent', value: true });
      this.collectRu({ type: 'memory', value: 1000000 });
  
      this.saveMetrics();
      this.saveRus();
      this.saveSc();
      this.savePc();
      this.saveRu();
  
      console.log('Performance Monitoring abgeschlossen!');
    }
  }
  
  const monitor = new PerformanceMonitor();
  monitor.monitorPerformance();