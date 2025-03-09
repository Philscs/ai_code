class TaskScheduler {
    constructor() {
      this.tasks = [];
      this.concurrencyLimit = 5;
      this.retryStrategies = [
        { retry: 3, delay: 1000 },
        { retry: 2, delay: 5000 },
        { retry: 1, delay: 20000 }
      ];
      this.dependencies = {};
    }
  
    addTask(task) {
      this.tasks.push(task);
    }
  
    schedule() {
      const readyTasks = [];
      for (const task of this.tasks) {
        if (!task.dependsOn || Object.keys(this.dependencies).every(dependency => 
  !this.tasks.includes(dependency))) {
          readyTasks.push(task);
        }
      }
  
      return new Promise((resolve, reject) => {
        const runningTasks = [];
        let finishedTasks = 0;
  
        function runNextTask() {
          if (readyTasks.length === 0 && runningTasks.length === this.concurrencyLimit) {
            resolve();
            return;
          }
  
          const task = readyTasks.shift();
  
          if (!task) {
            return;
          }
  
          runningTasks.push(task);
  
          task.run().then(() => {
            runningTasks.splice(runningTasks.indexOf(task), 1);
            finishedTasks++;
  
            for (const dependentTask of Object.values(this.dependencies)) {
              if (dependentTask.dependsOn.includes(task.id)) {
                this.dependencies[dependentTask.id] = dependentTask.dependsOn.filter(dependency => 
  dependency !== task.id);
                runNextTask();
              }
            }
  
            runNextTask();
          }).catch((error) => {
            runningTasks.splice(runningTasks.indexOf(task), 1);
  
            if (task.retryStrategy && task.retryStrategy.retry > 0) {
              const retryDelay = this.retryStrategies[task.retryStrategy.retry - 1].delay;
              setTimeout(() => {
                task.run().then(() => {
                  runningTasks.splice(runningTasks.indexOf(task), 1);
                  finishedTasks++;
                  runNextTask();
                }).catch((error) => {
                  console.error(`Task ${task.id} failed with error: ${error}`);
                  throw error;
                });
              }, retryDelay);
            } else {
              this.dependencies[task.id] = task.dependsOn;
              console.error(`Task ${task.id} failed`);
              reject(error);
            }
          });
        }
  
        runNextTask();
      });
    }
  }
  
  class Task {
    constructor(id, name, dependsOn = []) {
      this.id = id;
      this.name = name;
      this.dependsOn = dependsOn;
  
      if (!this.dependsOn.includes('*')) {
        for (const dependency of this.dependsOn) {
          if (!Object.values(this.dependencies).includes(dependency)) {
            throw new Error(`Task ${id} depends on unknown task: ${dependency}`);
          }
        }
      }
  
      this.retryStrategy = null;
    }
  
    run() {
      return new Promise((resolve, reject) => {
        // Hier kommt die eigentliche Ausführung des Tasks
        console.log(`Task ${this.id} started`);
        resolve();
      });
    }
  }
  
  const scheduler = new TaskScheduler();
  
  const task1 = new Task('task1', 'Task 1');
  const task2 = new Task('task2', 'Task 2', ['task1']);
  const task3 = new Task('task3', 'Task 3', ['task2']);
  
  scheduler.addTask(task1);
  scheduler.addTask(task2);
  scheduler.addTask(task3);
  
  scheduler.schedule().then(() => {
    console.log('All tasks finished');
  }).catch((error) => {
    console.error(error);
  });