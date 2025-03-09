class TaskScheduler {
  constructor() {
    this.tasks = [];
    this.runningTasks = 0;
    this.maxConcurrentTasks = 5;
  }

  addTask(task) {
    this.tasks.push(task);
  }

  start() {
    this.scheduleTasks();
  }

  scheduleTasks() {
    for (const task of this.tasks) {
      if (this.runningTasks >= this.maxConcurrentTasks) {
        break;
      }

      if (this.canRunTask(task)) {
        this.runTask(task);
      }
    }
  }

  canRunTask(task) {
    // Check if task meets any dependencies
    for (const dependency of task.dependencies) {
      if (!dependency.isCompleted) {
        return false;
      }
    }

    // Check if task priority is higher than running tasks
    const higherPriorityTasks = this.tasks.filter(t => t.priority > task.priority && t.isRunning);
    return higherPriorityTasks.length === 0;
  }

  runTask(task) {
    task.isRunning = true;
    this.runningTasks++;

    // Execute task logic
    task.execute()
      .then(() => {
        task.isCompleted = true;
        task.isRunning = false;
        this.runningTasks--;

        // Retry task if necessary
        if (task.retryStrategy.shouldRetry()) {
          task.retryStrategy.retry(() => {
            this.runTask(task);
          });
        }

        // Schedule next tasks
        this.scheduleTasks();
      });
  }
}

class Task {
  constructor(name, cronExpression, priority, dependencies, retryStrategy, execute) {
    this.name = name;
    this.cronExpression = cronExpression;
    this.priority = priority;
    this.dependencies = dependencies;
    this.retryStrategy = retryStrategy;
    this.execute = execute;
    this.isRunning = false;
    this.isCompleted = false;
  }
}

class RetryStrategy {
  constructor(maxRetries, delay) {
    this.maxRetries = maxRetries;
    this.delay = delay;
    this.retryCount = 0;
  }

  shouldRetry() {
    return this.retryCount < this.maxRetries;
  }

  retry(callback) {
    setTimeout(() => {
      this.retryCount++;
      callback();
    }, this.delay);
  }
}

// Usage example
const taskScheduler = new TaskScheduler();

const task1 = new Task(
  "Task 1",
  "*/5 * * * *", // Run every 5 minutes
  1,
  [],
  new RetryStrategy(3, 1000), // Retry 3 times with a 1-second delay
  () => {
    console.log("Executing Task 1");
    // Task 1 logic here
  }
);

const task2 = new Task(
  "Task 2",
  "0 0 * * *", // Run every day at midnight
  2,
  [task1], // Task 2 depends on Task 1
  new RetryStrategy(0, 0), // No retries
  () => {
    console.log("Executing Task 2");
    // Task 2 logic here
  }
);

taskScheduler.addTask(task1);
taskScheduler.addTask(task2);

taskScheduler.start();
