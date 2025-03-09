class FeatureFlagManager {
    constructor() {
      this.flags = {}; // Stores feature flags and their configurations
      this.userOverrides = {}; // Per-user overrides for flags
      this.analytics = null; // Optional analytics integration
    }
  
    // Set analytics provider for tracking
    setAnalyticsProvider(provider) {
      this.analytics = provider;
    }
  
    // Add or update a feature flag
    setFlag(flagName, config) {
      this.flags[flagName] = {
        enabled: config.enabled || false,
        rollout: config.rollout || 100, // Percentage rollout
        targets: config.targets || [], // Specific user targeting
        variations: config.variations || [], // Variations for A/B testing
        defaultVariation: config.defaultVariation || null, // Default variation
      };
    }
  
    // Remove a feature flag
    removeFlag(flagName) {
      delete this.flags[flagName];
    }
  
    // Set override for a specific user
    setUserOverride(userId, flagName, value) {
      if (!this.userOverrides[userId]) {
        this.userOverrides[userId] = {};
      }
      this.userOverrides[userId][flagName] = value;
    }
  
    // Clear user override
    clearUserOverride(userId, flagName) {
      if (this.userOverrides[userId]) {
        delete this.userOverrides[userId][flagName];
      }
    }
  
    // Get the flag value for a user
    getFlagValue(userId, flagName) {
      const flag = this.flags[flagName];
  
      if (!flag) {
        return false; // Default to false if flag doesn't exist
      }
  
      // Check user overrides
      if (this.userOverrides[userId] && this.userOverrides[userId][flagName] !== undefined) {
        return this.userOverrides[userId][flagName];
      }
  
      // Check specific user targeting
      if (flag.targets.includes(userId)) {
        return true;
      }
  
      // Check rollout percentage
      const hash = this.hashUser(userId + flagName);
      if (hash % 100 < flag.rollout) {
        return true;
      }
  
      // Handle A/B testing
      if (flag.variations.length > 0) {
        const variation = this.getVariation(userId, flag);
        this.trackAnalytics(userId, flagName, variation);
        return variation;
      }
  
      return flag.enabled;
    }
  
    // Get a variation for A/B testing
    getVariation(userId, flag) {
      if (!flag.variations.length) {
        return flag.defaultVariation;
      }
  
      const hash = this.hashUser(userId + flag.name);
      return flag.variations[hash % flag.variations.length];
    }
  
    // Track analytics if a provider is set
    trackAnalytics(userId, flagName, variation) {
      if (this.analytics) {
        this.analytics.track('FeatureFlagEvaluated', {
          userId,
          flagName,
          variation,
        });
      }
    }
  
    // Simple hash function for consistent rollout calculations
    hashUser(userId) {
      let hash = 0;
      for (let i = 0; i < userId.length; i++) {
        hash = (hash << 5) - hash + userId.charCodeAt(i);
        hash |= 0; // Convert to 32-bit integer
      }
      return Math.abs(hash);
    }
  }
  
  // Usage example:
  const featureManager = new FeatureFlagManager();
  featureManager.setFlag('newFeature', {
    enabled: false,
    rollout: 50,
    targets: ['user123', 'user456'],
    variations: ['A', 'B'],
    defaultVariation: 'A',
  });
  
  featureManager.setUserOverride('user789', 'newFeature', true);
  
  const isEnabled = featureManager.getFlagValue('user123', 'newFeature');
  console.log(isEnabled); // Outputs: true or variation based on the configuration