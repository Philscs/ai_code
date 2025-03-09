// main.js - Main Thread Code
class BackgroundTaskManager {
    constructor(workerCount = navigator.hardwareConcurrency || 4) {
        this.workers = [];
        this.taskQueue = [];
        this.activeWorkers = new Map();
        this.taskResults = new Map();
        this.maxWorkers = workerCount;
        this.resourcePool = new ResourcePool();
        
        this.initializeWorkers();
    }

    initializeWorkers() {
        for (let i = 0; i < this.maxWorkers; i++) {
            const worker = new Worker('worker.js');
            worker.onmessage = (e) => this.handleWorkerMessage(worker, e);
            worker.onerror = (e) => this.handleWorkerError(worker, e);
            this.workers.push(worker);
        }
    }

    async addTask(taskConfig) {
        const task = {
            id: crypto.randomUUID(),
            priority: taskConfig.priority || 1,
            data: taskConfig.data,
            resourceRequirements: taskConfig.resourceRequirements || {},
            status: 'pending',
            progress: 0,
            retries: 0,
            maxRetries: taskConfig.maxRetries || 3,
            onProgress: taskConfig.onProgress || (() => {}),
            onComplete: taskConfig.onComplete || (() => {}),
            onError: taskConfig.onError || (() => {})
        };

        this.taskQueue.push(task);
        this.taskQueue.sort((a, b) => b.priority - a.priority);
        
        this.scheduleNextTask();
        return task.id;
    }

    async scheduleNextTask() {
        if (this.taskQueue.length === 0) return;

        const availableWorker = this.workers.find(w => !this.activeWorkers.has(w));
        if (!availableWorker) return;

        const task = this.taskQueue.find(task => 
            this.resourcePool.canAllocate(task.resourceRequirements)
        );

        if (!task) return;

        this.taskQueue = this.taskQueue.filter(t => t.id !== task.id);
        this.activeWorkers.set(availableWorker, task);
        this.resourcePool.allocate(task.resourceRequirements);

        availableWorker.postMessage({
            type: 'execute',
            taskId: task.id,
            data: task.data,
            resourceRequirements: task.resourceRequirements
        });
    }

    handleWorkerMessage(worker, event) {
        const { type, taskId, data, progress } = event.data;
        const task = this.activeWorkers.get(worker);

        switch (type) {
            case 'progress':
                task.progress = progress;
                task.onProgress(progress);
                break;

            case 'complete':
                this.taskResults.set(taskId, data);
                task.status = 'completed';
                task.onComplete(data);
                this.cleanupTask(worker, task);
                break;

            case 'error':
                this.handleTaskError(worker, task, data);
                break;
        }
    }

    handleWorkerError(worker, error) {
        const task = this.activeWorkers.get(worker);
        if (task) {
            this.handleTaskError(worker, task, error);
        }
    }

    handleTaskError(worker, task, error) {
        if (task.retries < task.maxRetries) {
            task.retries++;
            task.status = 'retrying';
            this.taskQueue.unshift(task);
        } else {
            task.status = 'failed';
            task.onError(error);
        }
        this.cleanupTask(worker, task);
    }

    cleanupTask(worker, task) {
        this.activeWorkers.delete(worker);
        this.resourcePool.release(task.resourceRequirements);
        this.scheduleNextTask();
    }

    getTaskStatus(taskId) {
        const queuedTask = this.taskQueue.find(t => t.id === taskId);
        if (queuedTask) return queuedTask;

        for (const [_, task] of this.activeWorkers) {
            if (task.id === taskId) return task;
        }

        return {
            id: taskId,
            status: 'unknown'
        };
    }

    terminateTask(taskId) {
        const queuedTaskIndex = this.taskQueue.findIndex(t => t.id === taskId);
        if (queuedTaskIndex !== -1) {
            this.taskQueue.splice(queuedTaskIndex, 1);
            return true;
        }

        for (const [worker, task] of this.activeWorkers) {
            if (task.id === taskId) {
                worker.terminate();
                const newWorker = new Worker('worker.js');
                newWorker.onmessage = (e) => this.handleWorkerMessage(newWorker, e);
                newWorker.onerror = (e) => this.handleWorkerError(newWorker, e);
                
                const index = this.workers.indexOf(worker);
                this.workers[index] = newWorker;
                
                this.cleanupTask(worker, task);
                return true;
            }
        }
        return false;
    }
}

class ResourcePool {
    constructor() {
        this.resources = new Map();
    }

    canAllocate(requirements) {
        for (const [resource, amount] of Object.entries(requirements)) {
            const available = this.resources.get(resource) || 0;
            if (available < amount) return false;
        }
        return true;
    }

    allocate(requirements) {
        for (const [resource, amount] of Object.entries(requirements)) {
            const current = this.resources.get(resource) || 0;
            this.resources.set(resource, current - amount);
        }
    }

    release(requirements) {
        for (const [resource, amount] of Object.entries(requirements)) {
            const current = this.resources.get(resource) || 0;
            this.resources.set(resource, current + amount);
        }
    }

    addResource(resource, amount) {
        const current = this.resources.get(resource) || 0;
        this.resources.set(resource, current + amount);
    }
}

// worker.js - Worker Thread Code
self.onmessage = async function(e) {
    const { type, taskId, data, resourceRequirements } = e.data;

    if (type === 'execute') {
        try {
            const result = await executeTask(data, progress => {
                self.postMessage({
                    type: 'progress',
                    taskId,
                    progress
                });
            });

            self.postMessage({
                type: 'complete',
                taskId,
                data: result
            });
        } catch (error) {
            self.postMessage({
                type: 'error',
                taskId,
                data: error.message
            });
        }
    }
};

// Example task execution function
async function executeTask(data, onProgress) {
    // Simulate work with progress updates
    const steps = 10;
    for (let i = 0; i < steps; i++) {
        await new Promise(resolve => setTimeout(resolve, 1000));
        onProgress((i + 1) / steps * 100);
    }
    return `Processed: ${data}`;
}

// Usage Example
const taskManager = new BackgroundTaskManager();

// Add resources to the pool
taskManager.resourcePool.addResource('memory', 1000);
taskManager.resourcePool.addResource('cpu', 4);

// Example task configuration
const taskConfig = {
    priority: 2,
    data: 'Sample task data',
    resourceRequirements: {
        memory: 250,
        cpu: 1
    },
    maxRetries: 3,
    onProgress: (progress) => console.log(`Task progress: ${progress}%`),
    onComplete: (result) => console.log('Task completed:', result),
    onError: (error) => console.error('Task failed:', error)
};

// Add task to the manager
const taskId = await taskManager.addTask(taskConfig);

// Check task status
console.log(taskManager.getTaskStatus(taskId));

// Optionally terminate task
// taskManager.terminateTask(taskId);