
import { reactive, toRefs } from 'vue';

// Define your form data
const formData = reactive({
  email: '',
  password: '',
  confirmPassword: '',
  age: '',
  termsAccepted: false,
});

// Define your custom validation rules
const customRules = {
  isEmailValid: (value) => {
    // Perform async validation logic here
    return new Promise((resolve, reject) => {
      // Simulating async validation
      setTimeout(() => {
        if (value.includes('@')) {
          resolve();
        } else {
          reject('Invalid email');
        }
      }, 1000);
    });
  },
  isPasswordValid: (value) => {
    // Perform custom password validation logic here
    return value.length >= 8;
  },
  isAgeValid: (value) => {
    // Perform custom age validation logic here
    return value >= 18;
  },
};

// Define your validation groups
const validationGroups = {
  group1: ['email', 'password', 'confirmPassword'],
  group2: ['age', 'termsAccepted'],
};

// Define your form validator
const formValidator = reactive({
  errors: {},
  async validate(group) {
    this.errors = {};

    const fieldsToValidate = validationGroups[group];

    for (const field of fieldsToValidate) {
      const value = formData[field];

      // Perform cross-field validation
      if (field === 'confirmPassword' && value !== formData.password) {
        this.errors[field] = 'Passwords do not match';
        continue;
      }

      // Perform custom validation rules
      if (customRules[field]) {
        if (typeof customRules[field] === 'function') {
          const isValid = await customRules[field](value);
          if (!isValid) {
            this.errors[field] = `Invalid ${field}`;
          }
        }
      }
    }

    return Object.keys(this.errors).length === 0;
  },
});

export default {
  setup() {
    return {
      ...toRefs(formData),
      ...toRefs(formValidator),
    };
  },
};
