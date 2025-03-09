class ProgressiveEnhancement {
    constructor() {
        this.features = new Map();
        this.fallbacks = new Map();
        this.securityRules = new Set();
        this.performanceMetrics = new Map();
        this.a11yChecks = new Set();

        this.initialize();
    }

    initialize() {
        this.initializeFeatureDetection();
        this.initializeFallbacks();
        this.initializeSecurity();
        this.initializePerformance();
        this.initializeAccessibility();
    }

    initializeFeatureDetection() {
        this.features.set('modules', 'import' in window);
        this.features.set('asyncAwait', (async () => {})() instanceof Promise);
        this.features.set('webComponents', 'customElements' in window);
        this.features.set('localStorage', (() => {
            try {
                localStorage.setItem('test', 'test');
                localStorage.removeItem('test');
                return true;
            } catch (e) {
                return false;
            }
        })());
        this.features.set('serviceWorker', 'serviceWorker' in navigator);
        this.features.set('webGL', (() => {
            try {
                const canvas = document.createElement('canvas');
                return !!(canvas.getContext('webgl') || canvas.getContext('experimental-webgl'));
            } catch (e) {
                return false;
            }
        })());
    }

    initializeFallbacks() {
        this.fallbacks.set('localStorage', {
            data: new Map(),
            setItem: (key, value) => this.fallbacks.get('localStorage').data.set(key, value),
            getItem: (key) => this.fallbacks.get('localStorage').data.get(key),
            removeItem: (key) => this.fallbacks.get('localStorage').data.delete(key),
            clear: () => this.fallbacks.get('localStorage').data.clear()
        });

        this.fallbacks.set('webGL', {
            create2DFallback: (canvas) => canvas.getContext('2d')
        });

        this.fallbacks.set('modules', {
            loadScript: (url) => new Promise((resolve, reject) => {
                const script = document.createElement('script');
                script.src = url;
                script.onload = resolve;
                script.onerror = reject;
                document.head.appendChild(script);
            })
        });
    }

    initializeSecurity() {
        this.securityRules.add({
            type: 'CSP',
            implement: () => {
                const meta = document.createElement('meta');
                meta.httpEquiv = 'Content-Security-Policy';
                meta.content = "default-src 'self'; img-src https://*; child-src 'none';";
                document.head.appendChild(meta);
            }
        });

        this.securityRules.add({
            type: 'XSS',
            sanitize: (input) => {
                const div = document.createElement('div');
                div.textContent = input;
                return div.innerHTML;
            }
        });

        this.securityRules.add({
            type: 'CSRF',
            addToken: (form) => {
                const token = this.generateCSRFToken();
                const input = document.createElement('input');
                input.type = 'hidden';
                input.name = 'csrf_token';
                input.value = token;
                form.appendChild(input);
            }
        });
    }

    initializePerformance() {
        this.performanceMetrics.set('resourceLoading', {
            lazyLoad: (elements) => {
                const observer = new IntersectionObserver((entries) => {
                    entries.forEach(entry => {
                        if (entry.isIntersecting) {
                            if (entry.target.dataset.src) {
                                entry.target.src = entry.target.dataset.src;
                                observer.unobserve(entry.target);
                            }
                        }
                    });
                });

                elements.forEach(element => observer.observe(element));
            }
        });

        this.performanceMetrics.set('codeSplitting', {
            loadChunk: async (chunkName) => {
                try {
                    const module = await import(`/chunks/${chunkName}.js`);
                    return module;
                } catch (error) {
                    console.error(`Failed to load chunk: ${chunkName}`, error);
                    return null;
                }
            }
        });

        this.performanceMetrics.set('monitoring', {
            measure: (label, callback) => {
                const start = performance.now();
                callback();
                const end = performance.now();
                return {
                    label,
                    duration: end - start
                };
            }
        });
    }

    initializeAccessibility() {
        this.a11yChecks.add({
            type: 'ARIA',
            enhance: (element) => {
                if (element.getAttribute('role') === 'button' && !element.getAttribute('tabindex')) {
                    element.setAttribute('tabindex', '0');
                }
                if (element.getAttribute('aria-expanded')) {
                    element.addEventListener('click', () => {
                        const isExpanded = element.getAttribute('aria-expanded') === 'true';
                        element.setAttribute('aria-expanded', (!isExpanded).toString());
                    });
                }
            }
        });

        this.a11yChecks.add({
            type: 'keyboard',
            implement: (element) => {
                element.addEventListener('keydown', (e) => {
                    if (e.key === 'Enter' || e.key === ' ') {
                        e.preventDefault();
                        element.click();
                    }
                });
            }
        });

        this.a11yChecks.add({
            type: 'focus',
            manage: () => {
                const focusableElements = document.querySelectorAll(
                    'button, [href], input, select, textarea, [tabindex]:not([tabindex="-1"])'
                );
                return {
                    trap: (container) => {
                        const firstFocusable = container.querySelector(
                            'button, [href], input, select, textarea, [tabindex]:not([tabindex="-1"])'
                        );
                        const lastFocusable = container.querySelectorAll(
                            'button, [href], input, select, textarea, [tabindex]:not([tabindex="-1"])'
                        ).slice(-1)[0];

                        container.addEventListener('keydown', (e) => {
                            if (e.key === 'Tab') {
                                if (e.shiftKey && document.activeElement === firstFocusable) {
                                    e.preventDefault();
                                    lastFocusable.focus();
                                } else if (!e.shiftKey && document.activeElement === lastFocusable) {
                                    e.preventDefault();
                                    firstFocusable.focus();
                                }
                            }
                        });
                    }
                };
            }
        });
    }

    generateCSRFToken() {
        return Array.from(crypto.getRandomValues(new Uint8Array(32)))
            .map(b => b.toString(16).padStart(2, '0'))
            .join('');
    }

    isFeatureSupported(featureName) {
        return this.features.get(featureName) || false;
    }

    getFallback(featureName) {
        return this.fallbacks.get(featureName);
    }

    applySecurity(element, type) {
        const rule = Array.from(this.securityRules)
            .find(rule => rule.type === type);
        if (rule) {
            return rule.implement(element);
        }
    }

    optimizePerformance(type, params) {
        const optimization = this.performanceMetrics.get(type);
        if (optimization) {
            return optimization(params);
        }
    }

    enhanceAccessibility(element, type) {
        const check = Array.from(this.a11yChecks)
            .find(check => check.type === type);
        if (check) {
            check.enhance(element);
        }
    }
}

const pe = new ProgressiveEnhancement();

if (pe.isFeatureSupported('webGL')) {
    // Use WebGL
} else {
    // Use 2D Canvas fallback
    const fallback = pe.getFallback('webGL');
    fallback.create2DFallback(canvas);
}

document.querySelectorAll('form').forEach(form => {
    pe.applySecurity(form, 'CSRF');
});

pe.optimizePerformance('resourceLoading', document.querySelectorAll('img[data-src]'));

document.querySelectorAll('[role="button"]').forEach(button => {
    pe.enhanceAccessibility(button, 'ARIA');
});
