import React, { useState, useEffect } from 'react';
import { AlertCircle, CheckCircle2 } from 'lucide-react';
import { Alert, AlertDescription } from '@/components/ui/alert';

// Form Context to manage state and validation
const FormContext = React.createContext({});

// Custom hook for form handling
const useForm = (initialValues = {}, validationRules = {}) => {
  const [values, setValues] = useState(initialValues);
  const [errors, setErrors] = useState({});
  const [touched, setTouched] = useState({});
  const [csrfToken, setCsrfToken] = useState('');

  // Generate CSRF token on mount
  useEffect(() => {
    const generateToken = () => {
      const token = Math.random().toString(36).substring(2) + 
                   Math.random().toString(36).substring(2);
      setCsrfToken(token);
      // In a real app, you'd store this token in your backend session
    };
    generateToken();
  }, []);

  // Validation engine
  const validate = (fieldName, value) => {
    const rules = validationRules[fieldName] || [];
    const fieldErrors = [];

    rules.forEach(rule => {
      if (rule.required && !value) {
        fieldErrors.push(rule.message || 'This field is required');
      }
      if (rule.minLength && value.length < rule.minLength) {
        fieldErrors.push(`Minimum length is ${rule.minLength} characters`);
      }
      if (rule.pattern && !rule.pattern.test(value)) {
        fieldErrors.push(rule.message || 'Invalid format');
      }
      if (rule.custom && !rule.custom(value)) {
        fieldErrors.push(rule.message || 'Invalid value');
      }
    });

    return fieldErrors;
  };

  // Handle input changes with XSS prevention
  const handleChange = (e) => {
    const { name, value } = e.target;
    // XSS Prevention: Sanitize input
    const sanitizedValue = value.replace(/[<>]/g, '');
    
    setValues(prev => ({
      ...prev,
      [name]: sanitizedValue
    }));

    // Validate on change
    const fieldErrors = validate(name, sanitizedValue);
    setErrors(prev => ({
      ...prev,
      [name]: fieldErrors
    }));
  };

  // Handle blur events
  const handleBlur = (e) => {
    const { name } = e.target;
    setTouched(prev => ({
      ...prev,
      [name]: true
    }));
  };

  // Form submission handler
  const handleSubmit = async (callback) => {
    // Validate all fields
    const formErrors = {};
    Object.keys(values).forEach(fieldName => {
      const fieldErrors = validate(fieldName, values[fieldName]);
      if (fieldErrors.length > 0) {
        formErrors[fieldName] = fieldErrors;
      }
    });

    setErrors(formErrors);
    setTouched(Object.keys(values).reduce((acc, key) => ({
      ...acc,
      [key]: true
    }), {}));

    // If no errors, proceed with submission
    if (Object.keys(formErrors).length === 0) {
      // Add CSRF token to submission
      const formData = {
        ...values,
        _csrf: csrfToken
      };
      
      await callback(formData);
    }
  };

  return {
    values,
    errors,
    touched,
    handleChange,
    handleBlur,
    handleSubmit,
    csrfToken
  };
};

// Custom Input Component with Two-Way Binding
const FormInput = ({ 
  name, 
  label, 
  type = 'text', 
  value, 
  onChange, 
  onBlur, 
  error,
  touched 
}) => {
  return (
    <div className="mb-4">
      <label className="block text-sm font-medium text-gray-700 mb-1">
        {label}
      </label>
      <input
        type={type}
        name={name}
        value={value}
        onChange={onChange}
        onBlur={onBlur}
        className={`
          w-full px-3 py-2 border rounded-md
          ${touched && error ? 'border-red-500' : 'border-gray-300'}
          focus:outline-none focus:ring-2 focus:ring-blue-500
        `}
      />
      {touched && error && (
        <Alert variant="destructive" className="mt-2">
          <AlertCircle className="h-4 w-4" />
          <AlertDescription className="ml-2">
            {error}
          </AlertDescription>
        </Alert>
      )}
    </div>
  );
};

// Example Form Implementation
const ExampleForm = () => {
  const validationRules = {
    email: [
      { required: true, message: 'Email is required' },
      { 
        pattern: /^[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}$/i,
        message: 'Invalid email address'
      }
    ],
    password: [
      { required: true, message: 'Password is required' },
      { minLength: 8, message: 'Password must be at least 8 characters' },
      { 
        custom: (value) => /[A-Z]/.test(value) && /[0-9]/.test(value),
        message: 'Password must contain at least one uppercase letter and one number'
      }
    ]
  };

  const {
    values,
    errors,
    touched,
    handleChange,
    handleBlur,
    handleSubmit,
    csrfToken
  } = useForm(
    { email: '', password: '' },
    validationRules
  );

  const onSubmit = async (formData) => {
    console.log('Form submitted with:', formData);
    // Here you would typically send the data to your server
  };

  return (
    <div className="max-w-md mx-auto p-6 bg-white rounded-lg shadow-md">
      <h2 className="text-2xl font-bold mb-6">Sign In</h2>
      <form onSubmit={(e) => {
        e.preventDefault();
        handleSubmit(onSubmit);
      }}>
        <input type="hidden" name="_csrf" value={csrfToken} />
        
        <FormInput
          name="email"
          label="Email"
          type="email"
          value={values.email}
          onChange={handleChange}
          onBlur={handleBlur}
          error={touched.email && errors.email?.[0]}
          touched={touched.email}
        />

        <FormInput
          name="password"
          label="Password"
          type="password"
          value={values.password}
          onChange={handleChange}
          onBlur={handleBlur}
          error={touched.password && errors.password?.[0]}
          touched={touched.password}
        />

        <button
          type="submit"
          className="w-full bg-blue-500 text-white py-2 px-4 rounded-md
                     hover:bg-blue-600 focus:outline-none focus:ring-2
                     focus:ring-blue-500 focus:ring-offset-2"
        >
          Submit
        </button>
      </form>
    </div>
  );
};

export default ExampleForm;