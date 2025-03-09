class TaskManager {
    constructor() {
      this.tasks = [];
      this.worker = null;
    }
  
    // Prioritätsordnung:
    // HIGH (1), MEDIUM (2), LOW (3)
    static get priorityMap() {
      return {
        HIGH: 1,
        MEDIUM: 2,
        LOW: 3,
      };
    }
  
    // Fügt einen neuen Task hinzu
    addTask(task) {
      this.tasks.push(task);
      this.sortTasks();
    }
  
    // Sortiert die Tasks basierend auf ihrer Priorität
    sortTasks() {
      this.tasks.sort((a, b) => TaskManager.priorityMap[a.priority] - 
  TaskManager.priorityMap[b.priority]);
    }
  
    // Läuft den Background Worker und verwaltet die Aufgaben
    runWorker() {
      if (!this.worker) {
        this.worker = new Worker('worker.js');
        this.worker.onmessage = (event) => this.handleProgress(event.data);
        this.worker.onerror = (event) => this.handleError(event.error);
      }
    }
  
    // Verwaltet die Fortschritte der Aufgaben
    handleProgress(progress) {
      const task = progress.task;
      if (task) {
        task.progress = progress.value;
      }
    }
  
    // Verwaltet Fehlern bei den Aufgaben
    handleError(error) {
      const task = error.task;
      if (task) {
        task.error = error.message;
      }
    }
  
    // Führt eine Aufgabe aus und gibt ihr einen Wert zwischen 0 und 100 zurück
    executeTask(task) {
      return new Promise((resolve, reject) => {
        this.worker.postMessage({ type: 'EXECUTE_TASK', data: { task } });
        const intervalId = setInterval(() => {
          if (task.progress >= 100) {
            clearInterval(intervalId);
            resolve(task.value);
          }
        }, 1000); // pro Sekunde
      });
    }
  
    // Führt eine Aufgabe aus und gibt ihr einen Wert zwischen 0 und 1 zurück
    async executeTaskAsync(task) {
      try {
        const value = await this.executeTask(task);
        return value / task.maxValue;
      } catch (error) {
        throw error;
      }
    }
  
    // Schliebt den Background Worker
    shutdown() {
      if (this.worker) {
        this.worker.terminate();
        this.worker = null;
      }
    }
  }
  
  export default TaskManager;
  
  // worker.js
  class WorkerTask {
    constructor(task) {
      this.task = task;
      this.progress = 0;
      this.error = null;
    }
  
    async execute() {
      if (this.task.priority === 'HIGH') {
        // Hier füge deine hohen Aufgaben ein, die schneller ausgeführt werden sollten
        return new Promise((resolve) => {
          setTimeout(() => {
            resolve(this.task.value);
          }, this.task.duration);
        });
      } else if (this.task.priority === 'MEDIUM') {
        // Hier füge deine mittleren Aufgaben ein, die durchschnittlich ausgeführt werden sollten
        return new Promise((resolve) => {
          setTimeout(() => {
            resolve(this.task.value);
          }, this.task.duration);
        });
      } else if (this.task.priority === 'LOW') {
        // Hier füge deine niedrigen Aufgaben ein, die langsamer ausgeführt werden sollten
        return new Promise((resolve) => {
          setTimeout(() => {
            resolve(this.task.value);
          }, this.task.duration);
        });
      }
    }
  
    async complete() {
      this.progress = 100;
    }
  
    getProgress() {
      return this.progress;
    }
  
    getError() {
      return this.error;
    }
  }
  
  // main.js
  class HighPriorityTask extends WorkerTask {
    constructor(name) {
      super({ priority: 'HIGH', name, value: 100, maxValue: 1 });
    }
  }
  
  class MediumPriorityTask extends WorkerTask {
    constructor(name) {
      super({ priority: 'MEDIUM', name, value: 50, maxValue: 2 });
    }
  }
  
  class LowPriorityTask extends WorkerTask {
    constructor(name) {
      super({ priority: 'LOW', name, value: 20, maxValue: 3 });
    }
  }
  
  const taskManager = new TaskManager();
  
  const highPriorityTask = new HighPriorityTask('Hohe Aufgabe');
  const mediumPriorityTask = new MediumPriorityTask('Mittlere Aufgabe');
  const lowPriorityTask = new LowPriorityTask('Niedrige Aufgabe');
  
  taskManager.addTask(highPriorityTask);
  taskManager.addTask(mediumPriorityTask);
  taskManager.addTask(lowPriorityTask);
  
  taskManager.runWorker();
  
  // Ausführen der Aufgaben
  async function executeTasks() {
    const highValue = await taskManager.executeTaskAsync(highPriorityTask);
    console.log(`Hohe Aufgabe: ${highValue * 100}% complete`);
  
    const mediumValue = await taskManager.executeTaskAsync(mediumPriorityTask);
    console.log(`Mittlere Aufgabe: ${mediumValue * 100}% complete`);
  
    const lowValue = await taskManager.executeTaskAsync(lowPriorityTask);
    console.log(`Niedrige Aufgabe: ${lowValue * 100}% complete`);
  }
  
  executeTasks();
  
  // Schließen des Background Workers
  taskManager.shutdown();
  