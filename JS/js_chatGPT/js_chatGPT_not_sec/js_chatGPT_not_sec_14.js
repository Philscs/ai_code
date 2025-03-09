const cronParser = require('cron-parser');

class TaskScheduler {
    constructor(maxConcurrentTasks = 5) {
        this.taskQueue = [];
        this.runningTasks = new Set();
        this.maxConcurrentTasks = maxConcurrentTasks;
    }

    addTask({ id, cron, priority = 1, fn, retries = 3, dependencies = [] }) {
        const interval = cronParser.parseExpression(cron);
        const task = {
            id,
            cron,
            priority,
            fn,
            retries,
            dependencies,
            nextRun: interval.next().toDate(),
            interval,
        };
        this.taskQueue.push(task);
        this.taskQueue.sort((a, b) => a.priority - b.priority);
    }

    async runTask(task) {
        if (task.dependencies.length > 0 && task.dependencies.some(dep => this.runningTasks.has(dep))) {
            return; // Wait for dependencies to complete
        }

        this.runningTasks.add(task.id);

        try {
            await task.fn();
            console.log(`Task ${task.id} completed successfully.`);
        } catch (err) {
            console.error(`Task ${task.id} failed:`, err);
            if (task.retries > 0) {
                console.log(`Retrying task ${task.id} (${task.retries} retries left)...`);
                task.retries--;
                this.taskQueue.push(task);
            }
        } finally {
            this.runningTasks.delete(task.id);
            task.nextRun = task.interval.next().toDate();
            this.taskQueue.push(task);
            this.taskQueue.sort((a, b) => a.priority - b.priority);
        }
    }

    async runScheduler() {
        while (true) {
            const now = new Date();

            const eligibleTasks = this.taskQueue.filter(
                (task) => task.nextRun <= now && !this.runningTasks.has(task.id)
            );

            while (
                eligibleTasks.length > 0 &&
                this.runningTasks.size < this.maxConcurrentTasks
            ) {
                const task = eligibleTasks.shift();
                this.taskQueue = this.taskQueue.filter((t) => t.id !== task.id);
                this.runTask(task);
            }

            await new Promise((resolve) => setTimeout(resolve, 1000)); // Check every second
        }
    }
}

// Example usage
(async () => {
    const scheduler = new TaskScheduler(2);

    scheduler.addTask({
        id: 'task1',
        cron: '* * * * * *', // Runs every second
        priority: 1,
        fn: async () => {
            console.log('Task 1 is running');
            await new Promise((resolve) => setTimeout(resolve, 2000)); // Simulate work
        },
    });

    scheduler.addTask({
        id: 'task2',
        cron: '*/5 * * * * *', // Runs every 5 seconds
        priority: 2,
        fn: async () => {
            console.log('Task 2 is running');
            await new Promise((resolve) => setTimeout(resolve, 1000)); // Simulate work
        },
        dependencies: ['task1'],
    });

    scheduler.addTask({
        id: 'task3',
        cron: '* * * * * *', // Runs every second
        priority: 3,
        fn: async () => {
            console.log('Task 3 is running');
            throw new Error('Simulated error');
        },
        retries: 2,
    });

    scheduler.runScheduler();
})();
