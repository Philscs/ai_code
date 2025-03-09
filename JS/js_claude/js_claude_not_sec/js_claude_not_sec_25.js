// State Machine base class
class State {
    constructor(name) {
        this.name = name;
    }

    async enter(context) {
        await WorkflowLogger.log(`Entering state: ${this.name}`);
    }

    async exit(context) {
        await WorkflowLogger.log(`Exiting state: ${this.name}`);
    }
}

// Workflow Logger
class WorkflowLogger {
    static async log(message, level = 'info') {
        const timestamp = new Date().toISOString();
        console.log(`[${timestamp}] [${level}] ${message}`);
    }

    static async error(message) {
        await this.log(message, 'error');
    }
}

// Workflow Context to maintain state and data
class WorkflowContext {
    constructor(initialData = {}) {
        this.data = initialData;
        this.history = [];
    }

    updateData(newData) {
        this.data = { ...this.data, ...newData };
    }

    addToHistory(state) {
        this.history.push({
            state: state.name,
            timestamp: new Date().toISOString()
        });
    }
}

// Workflow Engine
class WorkflowEngine {
    constructor() {
        this.states = new Map();
        this.transitions = new Map();
        this.currentState = null;
        this.context = null;
    }

    addState(state) {
        this.states.set(state.name, state);
        return this;
    }

    addTransition(fromState, toState, condition = () => true) {
        if (!this.transitions.has(fromState)) {
            this.transitions.set(fromState, []);
        }
        this.transitions.get(fromState).push({ toState, condition });
        return this;
    }

    async start(initialState, initialData = {}) {
        try {
            this.context = new WorkflowContext(initialData);
            this.currentState = this.states.get(initialState);
            
            if (!this.currentState) {
                throw new Error(`Invalid initial state: ${initialState}`);
            }

            await WorkflowLogger.log(`Starting workflow with state: ${initialState}`);
            await this.currentState.enter(this.context);
            this.context.addToHistory(this.currentState);
            
            return this.currentState;
        } catch (error) {
            await WorkflowLogger.error(`Error starting workflow: ${error.message}`);
            throw error;
        }
    }

    async transition() {
        try {
            const possibleTransitions = this.transitions.get(this.currentState.name) || [];
            const validTransitions = [];

            // Evaluate all possible transitions
            for (const transition of possibleTransitions) {
                if (await transition.condition(this.context)) {
                    validTransitions.push(transition);
                }
            }

            if (validTransitions.length === 0) {
                await WorkflowLogger.log(`No valid transitions from state: ${this.currentState.name}`);
                return null;
            }

            // Execute the first valid transition
            const nextTransition = validTransitions[0];
            const nextState = this.states.get(nextTransition.toState);

            if (!nextState) {
                throw new Error(`Invalid transition state: ${nextTransition.toState}`);
            }

            await this.currentState.exit(this.context);
            this.currentState = nextState;
            await this.currentState.enter(this.context);
            this.context.addToHistory(this.currentState);

            return this.currentState;
        } catch (error) {
            await WorkflowLogger.error(`Error during transition: ${error.message}`);
            throw error;
        }
    }

    // Parallel execution of multiple workflows
    static async executeParallel(workflows) {
        try {
            await WorkflowLogger.log('Starting parallel workflow execution');
            const results = await Promise.all(workflows.map(wf => wf.execute()));
            await WorkflowLogger.log('Completed parallel workflow execution');
            return results;
        } catch (error) {
            await WorkflowLogger.error(`Error in parallel execution: ${error.message}`);
            throw error;
        }
    }
}

// Example usage:
// Define custom states
class InitialState extends State {
    async enter(context) {
        await super.enter(context);
        // Custom logic for initial state
        context.updateData({ initialized: true });
    }
}

class ProcessingState extends State {
    async enter(context) {
        await super.enter(context);
        // Custom processing logic
        context.updateData({ processing: true });
    }
}

class CompletedState extends State {
    async enter(context) {
        await super.enter(context);
        // Custom completion logic
        context.updateData({ completed: true });
    }
}

// Example workflow creation
async function createSampleWorkflow() {
    const workflow = new WorkflowEngine();
    
    // Add states
    workflow
        .addState(new InitialState('initial'))
        .addState(new ProcessingState('processing'))
        .addState(new CompletedState('completed'));
    
    // Add transitions with conditions
    workflow
        .addTransition('initial', 'processing', context => context.data.initialized)
        .addTransition('processing', 'completed', context => context.data.processing);
    
    return workflow;
}

// Example usage
async function runWorkflowExample() {
    try {
        // Create and start a single workflow
        const workflow = await createSampleWorkflow();
        await workflow.start('initial', { someData: 'value' });
        
        // Execute transitions
        let currentState = await workflow.transition();
        while (currentState && currentState.name !== 'completed') {
            currentState = await workflow.transition();
        }
        
        // Execute parallel workflows
        const workflow1 = await createSampleWorkflow();
        const workflow2 = await createSampleWorkflow();
        await WorkflowEngine.executeParallel([workflow1, workflow2]);
        
    } catch (error) {
        await WorkflowLogger.error(`Workflow execution failed: ${error.message}`);
    }
}

// Run the example
runWorkflowExample();