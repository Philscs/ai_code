class WorkflowEngine {
    constructor() {
      this.activities = {};
      this.log = [];
    }
  
    // Register a new activity
    registerActivity(name, activityFn) {
      this.activities[name] = activityFn;
    }
  
    // Log activity
    logActivity(state, message) {
      this.log.push({ state, message, timestamp: new Date() });
    }
  
    // Execute a workflow
    async execute(stateMachine) {
      let currentState = stateMachine.initialState;
      const context = {};
  
      while (currentState) {
        const stateConfig = stateMachine.states[currentState];
        
        if (!stateConfig) {
          throw new Error(`State '${currentState}' not found`);
        }
  
        this.logActivity(currentState, `Entering state`);
  
        try {
          if (stateConfig.activities) {
            // Execute activities in parallel
            await Promise.all(
              stateConfig.activities.map(async (activityName) => {
                const activity = this.activities[activityName];
                if (!activity) {
                  throw new Error(`Activity '${activityName}' not registered`);
                }
                await activity(context);
              })
            );
          }
  
          // Evaluate transitions
          if (stateConfig.conditionalBranches) {
            for (const branch of stateConfig.conditionalBranches) {
              if (branch.condition(context)) {
                currentState = branch.nextState;
                this.logActivity(currentState, `Condition met, transitioning to next state`);
                break;
              }
            }
          } else {
            currentState = stateConfig.nextState;
          }
  
        } catch (error) {
          this.logActivity(currentState, `Error: ${error.message}`);
          if (stateConfig.onError) {
            currentState = stateConfig.onError;
            this.logActivity(currentState, `Transitioning to error state`);
          } else {
            throw error;
          }
        }
      }
  
      this.logActivity(currentState, `Workflow complete`);
    }
  }
  
  // Example usage
  const workflowEngine = new WorkflowEngine();
  
  // Register activities
  workflowEngine.registerActivity("fetchData", async (context) => {
    context.data = { key: "value" };
  });
  
  workflowEngine.registerActivity("processData", async (context) => {
    if (!context.data) throw new Error("No data to process");
    context.processed = { ...context.data, processed: true };
  });
  
  // Define state machine
  const stateMachine = {
    initialState: "fetching",
    states: {
      fetching: {
        activities: ["fetchData"],
        nextState: "processing",
      },
      processing: {
        activities: ["processData"],
        nextState: "done",
        onError: "errorHandling",
      },
      errorHandling: {
        activities: [],
        nextState: null, // End workflow
      },
      done: {
        activities: [],
        nextState: null, // End workflow
      },
    },
  };
  
  // Execute workflow
  workflowEngine.execute(stateMachine).then(() => {
    console.log("Workflow complete");
    console.log("Logs:", workflowEngine.log);
  }).catch((error) => {
    console.error("Workflow failed:", error);
  });
  