class BackgroundTaskManager {
    constructor() {
      this.tasks = [];
      this.workers = [];
      this.workerCount = navigator.hardwareConcurrency || 4;
      this.initWorkers();
    }
  
    initWorkers() {
      for (let i = 0; i < this.workerCount; i++) {
        const worker = new Worker('worker.js');
        worker.onmessage = (e) => this.handleWorkerMessage(e, worker);
        this.workers.push({ worker, busy: false });
      }
    }
  
    addTask(taskFunction, priority = 0, onProgress = null) {
      const taskId = Math.random().toString(36).substr(2, 9);
      this.tasks.push({ taskId, taskFunction, priority, onProgress, status: 'pending' });
      this.tasks.sort((a, b) => b.priority - a.priority); // Higher priority first
      this.processTasks();
      return taskId;
    }
  
    processTasks() {
      for (const task of this.tasks) {
        if (task.status === 'pending') {
          const idleWorker = this.workers.find(w => !w.busy);
          if (idleWorker) {
            idleWorker.busy = true;
            task.status = 'in-progress';
            idleWorker.worker.postMessage({ taskId: task.taskId, taskFunction: task.taskFunction.toString() });
          }
        }
      }
    }
  
    handleWorkerMessage(event, worker) {
      const { taskId, status, progress, error } = event.data;
      const task = this.tasks.find(t => t.taskId === taskId);
  
      if (task) {
        if (status === 'progress') {
          task.onProgress && task.onProgress(progress);
        } else if (status === 'completed') {
          task.status = 'completed';
          worker.busy = false;
          this.processTasks();
        } else if (status === 'error') {
          task.status = 'error';
          console.error(`Task ${taskId} failed:`, error);
          worker.busy = false;
          this.processTasks();
        }
      }
    }
  
    getTaskStatus(taskId) {
      const task = this.tasks.find(t => t.taskId === taskId);
      return task ? task.status : 'not-found';
    }
  }
  
  // Example worker.js file content:
  // onmessage = function(e) {
  //   const { taskId, taskFunction } = e.data;
  //   try {
  //     const func = new Function(`return ${taskFunction}`)();
  //     const result = func();
  //     postMessage({ taskId, status: 'completed', result });
  //   } catch (error) {
  //     postMessage({ taskId, status: 'error', error: error.message });
  //   }
  // };
  
  // Usage example:
  const manager = new BackgroundTaskManager();
  const taskId = manager.addTask(() => {
    let sum = 0;
    for (let i = 0; i < 1e7; i++) sum += i;
    return sum;
  }, 1, (progress) => console.log('Progress:', progress));
  
  console.log(`Task ${taskId} added.`);
  