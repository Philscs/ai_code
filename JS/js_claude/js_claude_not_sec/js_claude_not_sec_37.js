class FeatureFlagManager {
    constructor() {
        this.features = new Map();
        this.analytics = new Map();
        this.updateInterval = 60000; // 1 minute default
        this.startPolling();
    }

    // Feature definition and basic operations
    addFeature(featureKey, config) {
        this.features.set(featureKey, {
            enabled: config.enabled ?? false,
            rolloutPercentage: config.rolloutPercentage ?? 0,
            userTargeting: config.userTargeting ?? {},
            abTest: config.abTest ?? null,
            analytics: {
                impressions: 0,
                enabledCount: 0,
                disabledCount: 0
            }
        });
    }

    removeFeature(featureKey) {
        this.features.delete(featureKey);
    }

    // Dynamic Updates
    async startPolling() {
        setInterval(async () => {
            await this.fetchUpdates();
        }, this.updateInterval);
    }

    async fetchUpdates() {
        try {
            // Simulate API call to fetch updates
            const updates = await this.fetchFeaturesFromAPI();
            updates.forEach((config, key) => {
                if (this.features.has(key)) {
                    const currentConfig = this.features.get(key);
                    this.features.set(key, { ...currentConfig, ...config });
                }
            });
        } catch (error) {
            console.error('Error fetching feature updates:', error);
        }
    }

    // User Targeting
    isEnabled(featureKey, user = null) {
        if (!this.features.has(featureKey)) {
            return false;
        }

        const feature = this.features.get(featureKey);
        
        // Track analytics
        this.trackImpression(featureKey);

        // Check if feature is globally enabled
        if (!feature.enabled) {
            this.trackResult(featureKey, false);
            return false;
        }

        // User targeting
        if (user && feature.userTargeting) {
            const targetingResult = this.evaluateUserTargeting(feature.userTargeting, user);
            if (targetingResult !== null) {
                this.trackResult(featureKey, targetingResult);
                return targetingResult;
            }
        }

        // A/B testing
        if (feature.abTest && user) {
            const abTestResult = this.evaluateABTest(feature.abTest, user);
            if (abTestResult !== null) {
                this.trackResult(featureKey, abTestResult);
                return abTestResult;
            }
        }

        // Rollout percentage
        const rolloutResult = this.evaluateRollout(feature.rolloutPercentage, user);
        this.trackResult(featureKey, rolloutResult);
        return rolloutResult;
    }

    // User Targeting Implementation
    evaluateUserTargeting(targeting, user) {
        if (!user) return null;

        for (const [rule, value] of Object.entries(targeting)) {
            switch (rule) {
                case 'country':
                    if (user.country !== value) return false;
                    break;
                case 'userType':
                    if (user.type !== value) return false;
                    break;
                case 'version':
                    if (!this.compareVersions(user.version, value)) return false;
                    break;
            }
        }

        return true;
    }

    // A/B Testing Implementation
    evaluateABTest(abTest, user) {
        if (!user || !abTest.active) return null;

        const hash = this.generateHash(`${user.id}:${abTest.id}`);
        const bucket = hash % 100;

        if (bucket < abTest.controlPercentage) {
            return false; // Control group
        } else if (bucket < (abTest.controlPercentage + abTest.treatmentPercentage)) {
            return true; // Treatment group
        }

        return null; // Not part of test
    }

    // Rollout Management
    evaluateRollout(percentage, user) {
        if (!user) {
            return Math.random() * 100 < percentage;
        }

        const hash = this.generateHash(user.id);
        return (hash % 100) < percentage;
    }

    // Analytics Integration
    trackImpression(featureKey) {
        const feature = this.features.get(featureKey);
        if (feature) {
            feature.analytics.impressions++;
        }
    }

    trackResult(featureKey, result) {
        const feature = this.features.get(featureKey);
        if (feature) {
            if (result) {
                feature.analytics.enabledCount++;
            } else {
                feature.analytics.disabledCount++;
            }
        }
    }

    getAnalytics(featureKey) {
        const feature = this.features.get(featureKey);
        if (!feature) return null;

        const { impressions, enabledCount, disabledCount } = feature.analytics;
        return {
            impressions,
            enabledCount,
            disabledCount,
            enableRate: (enabledCount / impressions * 100).toFixed(2) + '%'
        };
    }

    // Utility Functions
    generateHash(str) {
        let hash = 0;
        for (let i = 0; i < str.length; i++) {
            const char = str.charCodeAt(i);
            hash = ((hash << 5) - hash) + char;
            hash = hash & hash;
        }
        return Math.abs(hash);
    }

    compareVersions(version1, version2) {
        const v1 = version1.split('.').map(Number);
        const v2 = version2.split('.').map(Number);
        
        for (let i = 0; i < Math.max(v1.length, v2.length); i++) {
            const num1 = v1[i] || 0;
            const num2 = v2[i] || 0;
            if (num1 < num2) return false;
            if (num1 > num2) return true;
        }
        return true;
    }

    // Mock API call - Replace with actual API implementation
    async fetchFeaturesFromAPI() {
        return new Map(); // Implement actual API call
    }
}

// Usage Example
const featureManager = new FeatureFlagManager();

// Add a feature with various configurations
featureManager.addFeature('newUI', {
    enabled: true,
    rolloutPercentage: 50,
    userTargeting: {
        country: 'DE',
        userType: 'premium',
        version: '2.0.0'
    },
    abTest: {
        id: 'ui-test-001',
        active: true,
        controlPercentage: 50,
        treatmentPercentage: 50
    }
});

// Check if feature is enabled for a user
const user = {
    id: '123',
    country: 'DE',
    type: 'premium',
    version: '2.1.0'
};

const isFeatureEnabled = featureManager.isEnabled('newUI', user);
console.log('Feature enabled for user:', isFeatureEnabled);

// Get analytics for the feature
const analytics = featureManager.getAnalytics('newUI');
console.log('Feature analytics:', analytics);