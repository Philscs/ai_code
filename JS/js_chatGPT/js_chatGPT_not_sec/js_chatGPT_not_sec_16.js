class ReactiveFormValidator {
    constructor(schema) {
        this.schema = schema;
        this.values = {};
        this.errors = {};
        this.init();
    }

    init() {
        Object.keys(this.schema).forEach((field) => {
            this.values[field] = '';
            this.errors[field] = [];
        });
    }

    async validateField(field, value) {
        const fieldRules = this.schema[field]?.rules || [];
        const errors = [];

        for (const rule of fieldRules) {
            const isValid = await rule.validate(value, this.values);
            if (!isValid) errors.push(rule.message);
        }

        this.errors[field] = errors;
        return errors.length === 0;
    }

    async validate() {
        let isValid = true;
        for (const field of Object.keys(this.schema)) {
            const fieldIsValid = await this.validateField(field, this.values[field]);
            if (!fieldIsValid) isValid = false;
        }
        return isValid;
    }

    updateValue(field, value) {
        this.values[field] = value;
        if (this.schema[field]?.dependsOn) {
            this.updateConditionalFields(field);
        }
        this.validateField(field, value);
    }

    updateConditionalFields(field) {
        const dependents = Object.keys(this.schema).filter((key) => this.schema[key]?.dependsOn === field);
        for (const dependent of dependents) {
            const condition = this.schema[dependent]?.condition;
            if (condition && condition(this.values[field])) {
                this.schema[dependent].enabled = true;
            } else {
                this.schema[dependent].enabled = false;
                this.values[dependent] = '';
                this.errors[dependent] = [];
            }
        }
    }

    getValidationGroup(groupName) {
        const group = Object.keys(this.schema).filter((field) => this.schema[field]?.group === groupName);
        return group.map((field) => ({
            field,
            value: this.values[field],
            errors: this.errors[field],
        }));
    }
}

// Example Usage
const formSchema = {
    email: {
        rules: [
            { validate: (value) => !!value, message: 'Email is required.' },
            { validate: (value) => /^[^@]+@[^@]+\.[^@]+$/.test(value), message: 'Invalid email address.' },
        ],
    },
    password: {
        rules: [
            { validate: (value) => value.length >= 6, message: 'Password must be at least 6 characters long.' },
        ],
    },
    confirmPassword: {
        rules: [
            { 
                validate: (value, values) => value === values.password, 
                message: 'Passwords do not match.', 
            },
        ],
    },
    age: {
        rules: [
            { validate: (value) => !isNaN(value) && value >= 18, message: 'You must be at least 18 years old.' },
        ],
        dependsOn: 'email',
        condition: (emailValue) => emailValue.includes('@example.com'),
    },
};

const formValidator = new ReactiveFormValidator(formSchema);

// Updating values
formValidator.updateValue('email', 'user@example.com');
formValidator.updateValue('password', 'secure123');
formValidator.updateValue('confirmPassword', 'secure123');

// Validate entire form
formValidator.validate().then((isValid) => {
    if (isValid) {
        console.log('Form is valid:', formValidator.values);
    } else {
        console.log('Form has errors:', formValidator.errors);
    }
});
