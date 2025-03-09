class WorkflowEngine {
  constructor() {
    this.states = {};
    this.transitions = {};
    this.activities = [];
    this.log = [];
  }

  addState(stateName) {
    this.states[stateName] = {};
  }

  addTransition(fromState, toState, condition) {
    if (!this.transitions[fromState]) {
      this.transitions[fromState] = [];
    }
    this.transitions[fromState].push({ toState, condition });
  }

  addActivity(activity) {
    this.activities.push(activity);
  }

  execute() {
    let currentState = Object.keys(this.states)[0];

    while (currentState) {
      this.log.push(`Entering state: ${currentState}`);

      const currentActivities = this.states[currentState].activities || [];
      const parallelActivities = [];

      for (const activity of currentActivities) {
        if (activity.parallel) {
          parallelActivities.push(activity);
        } else {
          this.executeActivity(activity);
        }
      }

      for (const activity of parallelActivities) {
        this.executeActivity(activity);
      }

      const transitions = this.transitions[currentState] || [];
      let nextState = null;

      for (const transition of transitions) {
        if (!transition.condition || transition.condition()) {
          nextState = transition.toState;
          break;
        }
      }

      if (!nextState) {
        this.log.push(`No valid transition found from state: ${currentState}`);
        break;
      }

      this.log.push(`Transitioning to state: ${nextState}`);
      currentState = nextState;
    }

    this.log.push('Workflow execution completed');
  }

  executeActivity(activity) {
    try {
      this.log.push(`Executing activity: ${activity.name}`);
      activity.execute();
      this.log.push(`Activity executed successfully: ${activity.name}`);
    } catch (error) {
      this.log.push(`Error executing activity: ${activity.name}`);
      this.log.push(`Error message: ${error.message}`);
    }
  }

  getActivityLog() {
    return this.log;
  }
}

// Example usage:

const workflow = new WorkflowEngine();

workflow.addState('State1');
workflow.addState('State2');
workflow.addState('State3');

workflow.addTransition('State1', 'State2', () => {
  // Condition for transition from State1 to State2
  return true;
});

workflow.addTransition('State2', 'State3', () => {
  // Condition for transition from State2 to State3
  return true;
});

workflow.addActivity({
  name: 'Activity1',
  execute: () => {
    // Code for executing Activity1
    console.log('Executing Activity1');
  },
});

workflow.addActivity({
  name: 'Activity2',
  execute: () => {
    // Code for executing Activity2
    console.log('Executing Activity2');
  },
});

workflow.addActivity({
  name: 'Activity3',
  execute: () => {
    // Code for executing Activity3
    console.log('Executing Activity3');
  },
});

workflow.execute();

const activityLog = workflow.getActivityLog();
console.log(activityLog);
