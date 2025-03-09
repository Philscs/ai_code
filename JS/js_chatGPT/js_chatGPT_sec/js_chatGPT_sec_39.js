// Progressive Enhancement System

// Feature Detection
function detectFeatures() {
    const features = {
        fetch: typeof window.fetch === 'function',
        localStorage: typeof window.localStorage !== 'undefined',
        serviceWorker: 'serviceWorker' in navigator,
    };
    return features;
}

// Fallback Strategy
function applyFallbacks(features) {
    if (!features.fetch) {
        console.warn('Fetch API not supported. Using XMLHttpRequest as a fallback.');
        // Example fallback: XMLHttpRequest
        window.fetch = function (url, options) {
            return new Promise((resolve, reject) => {
                const xhr = new XMLHttpRequest();
                xhr.open(options?.method || 'GET', url);

                xhr.onload = () => resolve({
                    status: xhr.status,
                    json: () => Promise.resolve(JSON.parse(xhr.responseText))
                });
                xhr.onerror = () => reject(new Error('Network request failed'));
                xhr.send(options?.body);
            });
        };
    }

    if (!features.localStorage) {
        console.warn('localStorage not supported. Using an in-memory store as a fallback.');
        // Example fallback: In-memory storage
        const memoryStore = {};
        window.localStorage = {
            setItem: (key, value) => { memoryStore[key] = value; },
            getItem: (key) => memoryStore[key] || null,
            removeItem: (key) => { delete memoryStore[key]; },
        };
    }
}

// Security Controls
function applySecurityHeaders() {
    // Example: Content Security Policy (CSP)
    const metaCSP = document.createElement('meta');
    metaCSP.httpEquiv = 'Content-Security-Policy';
    metaCSP.content = "default-src 'self'; script-src 'self'; style-src 'self';";
    document.head.appendChild(metaCSP);

    // Additional headers could be applied on the server-side for better control.
}

// Performance Optimization
function optimizePerformance() {
    // Lazy loading images
    const images = document.querySelectorAll('img[data-src]');
    const loadImage = (img) => {
        img.src = img.dataset.src;
        img.removeAttribute('data-src');
    };

    if ('IntersectionObserver' in window) {
        const observer = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    loadImage(entry.target);
                    observer.unobserve(entry.target);
                }
            });
        });

        images.forEach(img => observer.observe(img));
    } else {
        // Fallback for browsers without IntersectionObserver
        images.forEach(loadImage);
    }

    // Minimize DOM reflows by batching style changes
    document.body.style.visibility = 'hidden';
    document.body.style.backgroundColor = '#f0f0f0';
    document.body.style.visibility = 'visible';
}

// Accessibility
function enhanceAccessibility() {
    // Ensure focus management
    document.addEventListener('keydown', (event) => {
        if (event.key === 'Tab') {
            document.body.classList.add('user-is-tabbing');
        }
    });

    document.addEventListener('mousedown', () => {
        document.body.classList.remove('user-is-tabbing');
    });

    // Add ARIA roles dynamically if needed
    const mainContent = document.querySelector('#main');
    if (mainContent && !mainContent.hasAttribute('role')) {
        mainContent.setAttribute('role', 'main');
    }

    // Example of keyboard navigation
    const navLinks = document.querySelectorAll('nav a');
    navLinks.forEach(link => {
        link.addEventListener('keydown', (event) => {
            if (event.key === 'Enter') {
                link.click();
            }
        });
    });
}

// Initialize Progressive Enhancement System
(function initialize() {
    const features = detectFeatures();
    applyFallbacks(features);
    applySecurityHeaders();
    optimizePerformance();
    enhanceAccessibility();

    console.log('Progressive Enhancement System initialized.');
})();
