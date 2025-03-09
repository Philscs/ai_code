class FeatureFlagManager {
    constructor() {
        this.flags = {};
        this.userTargeting = {};
        this.analyticsIntegration = null;
    }

    addFlag(flagName, defaultValue) {
        this.flags[flagName] = defaultValue;
    }

    updateFlag(flagName, newValue) {
        if (this.flags.hasOwnProperty(flagName)) {
            this.flags[flagName] = newValue;
        } else {
            console.error(`Flag '${flagName}' does not exist.`);
        }
    }

    setUserTargeting(userId, targeting) {
        this.userTargeting[userId] = targeting;
    }

    getFlagValue(flagName, userId) {
        if (this.flags.hasOwnProperty(flagName)) {
            const flagValue = this.flags[flagName];
            if (this.userTargeting.hasOwnProperty(userId)) {
                const targeting = this.userTargeting[userId];
                if (targeting.hasOwnProperty(flagName)) {
                    return targeting[flagName];
                }
            }
            return flagValue;
        } else {
            console.error(`Flag '${flagName}' does not exist.`);
        }
    }

    runABTest(flagName, variantA, variantB, userId) {
        const flagValue = this.getFlagValue(flagName, userId);
        if (flagValue === variantA) {
            return variantA;
        } else if (flagValue === variantB) {
            return variantB;
        } else {
            console.error(`Invalid variants for flag '${flagName}'.`);
        }
    }

    setRolloutPercentage(flagName, percentage) {
        if (this.flags.hasOwnProperty(flagName)) {
            this.flags[flagName].rolloutPercentage = percentage;
        } else {
            console.error(`Flag '${flagName}' does not exist.`);
        }
    }

    setAnalyticsIntegration(analyticsIntegration) {
        this.analyticsIntegration = analyticsIntegration;
    }

    trackEvent(eventName, properties) {
        if (this.analyticsIntegration) {
            this.analyticsIntegration.track(eventName, properties);
        }
    }
}
