class ReactiveForm {
    constructor(config) {
        this.formData = config.initialData || {};
        this.validators = config.validators || {};
        this.errors = {};
        this.csrfToken = this.generateCsrfToken();
        this.initBindings(config.controls || {});
    }

    // Generate a CSRF token
    generateCsrfToken() {
        return btoa(String.fromCharCode(...crypto.getRandomValues(new Uint8Array(16))));
    }

    // Bind controls and initialize two-way data binding
    initBindings(controls) {
        for (const [key, element] of Object.entries(controls)) {
            if (!element) continue;

            // Initialize with existing data
            if (this.formData[key] !== undefined) {
                element.value = this.formData[key];
            }

            // Listen for changes
            element.addEventListener('input', (event) => {
                this.formData[key] = event.target.value;
                this.validateField(key);
            });
        }
    }

    // Validation logic
    validateField(fieldName) {
        const rules = this.validators[fieldName];
        const value = this.formData[fieldName];

        if (!rules) return;

        const fieldErrors = rules
            .map((rule) => rule(value))
            .filter((error) => error !== null);

        this.errors[fieldName] = fieldErrors;
    }

    validateAll() {
        for (const field in this.validators) {
            this.validateField(field);
        }
    }

    isValid() {
        this.validateAll();
        return Object.values(this.errors).every((errorList) => errorList.length === 0);
    }

    // Get sanitized data
    getSanitizedData() {
        const sanitizedData = {};
        for (const [key, value] of Object.entries(this.formData)) {
            sanitizedData[key] = this.escapeHtml(value);
        }
        return sanitizedData;
    }

    // Escape HTML to prevent XSS
    escapeHtml(str) {
        const div = document.createElement('div');
        div.textContent = str;
        return div.innerHTML;
    }

    // Submit logic with CSRF token
    async submit(url) {
        if (!this.isValid()) {
            console.error('Form is invalid:', this.errors);
            return;
        }

        const sanitizedData = this.getSanitizedData();

        try {
            const response = await fetch(url, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'CSRF-Token': this.csrfToken,
                },
                body: JSON.stringify(sanitizedData),
            });

            if (!response.ok) {
                throw new Error(`Error: ${response.statusText}`);
            }

            console.log('Form submitted successfully:', await response.json());
        } catch (error) {
            console.error('Submission failed:', error);
        }
    }
}

// Example Usage
const form = new ReactiveForm({
    initialData: {
        name: '',
        email: ''
    },
    controls: {
        name: document.querySelector('#name'),
        email: document.querySelector('#email')
    },
    validators: {
        name: [
            (value) => (value ? null : 'Name is required.')
        ],
        email: [
            (value) => (/.+@.+\..+/.test(value) ? null : 'Invalid email address.')
        ]
    }
});

document.querySelector('#submit').addEventListener('click', () => {
    form.submit('/submit-endpoint');
});
