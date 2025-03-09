class Task {
    constructor(id, cronPattern, handler, options = {}) {
      this.id = id;
      this.cronPattern = cronPattern;
      this.handler = handler;
      this.priority = options.priority || 1;
      this.dependencies = options.dependencies || [];
      this.retryStrategy = options.retryStrategy || {
        maxAttempts: 3,
        backoffMs: 1000,
        backoffMultiplier: 2
      };
      this.attempts = 0;
      this.status = 'pending';
      this.lastRun = null;
      this.nextRun = this.calculateNextRun();
    }
  
    calculateNextRun() {
      // Vereinfachte Cron-Pattern-Interpretation (Minuten * * * *)
      const [minutes] = this.cronPattern.split(' ');
      const now = new Date();
      const nextRun = new Date(now);
      
      if (minutes === '*') {
        nextRun.setMinutes(now.getMinutes() + 1);
      } else {
        const nextMinute = parseInt(minutes);
        if (nextMinute <= now.getMinutes()) {
          nextRun.setHours(now.getHours() + 1);
        }
        nextRun.setMinutes(nextMinute);
      }
      
      return nextRun;
    }
  }
  
  class TaskScheduler {
    constructor(maxConcurrent = 3) {
      this.tasks = new Map();
      this.maxConcurrent = maxConcurrent;
      this.running = new Set();
      this.initialized = false;
    }
  
    addTask(id, cronPattern, handler, options = {}) {
      const task = new Task(id, cronPattern, handler, options);
      this.tasks.set(id, task);
      return task;
    }
  
    async start() {
      if (this.initialized) return;
      this.initialized = true;
      this.scheduleLoop();
    }
  
    async scheduleLoop() {
      while (true) {
        await this.processNextTasks();
        await new Promise(resolve => setTimeout(resolve, 1000));
      }
    }
  
    async processNextTasks() {
      const now = new Date();
      const eligibleTasks = Array.from(this.tasks.values())
        .filter(task => 
          task.status !== 'running' &&
          task.nextRun <= now &&
          this.areDependenciesMet(task)
        )
        .sort((a, b) => b.priority - a.priority);
  
      for (const task of eligibleTasks) {
        if (this.running.size >= this.maxConcurrent) break;
        this.executeTask(task);
      }
    }
  
    areDependenciesMet(task) {
      return task.dependencies.every(depId => {
        const depTask = this.tasks.get(depId);
        return depTask && depTask.status === 'completed';
      });
    }
  
    async executeTask(task) {
      if (this.running.has(task.id)) return;
  
      this.running.add(task.id);
      task.status = 'running';
      task.attempts++;
  
      try {
        await task.handler();
        task.status = 'completed';
        task.lastRun = new Date();
        task.attempts = 0;
      } catch (error) {
        console.error(`Task ${task.id} failed:`, error);
        
        if (task.attempts < task.retryStrategy.maxAttempts) {
          const backoffTime = 
            task.retryStrategy.backoffMs * 
            Math.pow(task.retryStrategy.backoffMultiplier, task.attempts - 1);
          
          task.status = 'pending';
          task.nextRun = new Date(Date.now() + backoffTime);
        } else {
          task.status = 'failed';
        }
      } finally {
        this.running.delete(task.id);
        if (task.status !== 'failed') {
          task.nextRun = task.calculateNextRun();
        }
      }
    }
  
    stopTask(id) {
      const task = this.tasks.get(id);
      if (task) {
        task.status = 'stopped';
        this.running.delete(id);
      }
    }
  
    resumeTask(id) {
      const task = this.tasks.get(id);
      if (task && task.status === 'stopped') {
        task.status = 'pending';
        task.nextRun = task.calculateNextRun();
      }
    }
  }
  
  // Beispiel zur Verwendung:
  const scheduler = new TaskScheduler(2); // Max 2 concurrent tasks
  
  // Task mit hoher Priorität
  scheduler.addTask(
    'highPriorityTask',
    '*/5 * * * *', // Alle 5 Minuten
    async () => {
      console.log('Executing high priority task');
      await new Promise(resolve => setTimeout(resolve, 2000));
    },
    { priority: 3 }
  );
  
  // Task mit Abhängigkeit
  scheduler.addTask(
    'dependentTask',
    '*/10 * * * *', // Alle 10 Minuten
    async () => {
      console.log('Executing dependent task');
    },
    { 
      priority: 2,
      dependencies: ['highPriorityTask'],
      retryStrategy: {
        maxAttempts: 5,
        backoffMs: 2000,
        backoffMultiplier: 1.5
      }
    }
  );
  
  scheduler.start();