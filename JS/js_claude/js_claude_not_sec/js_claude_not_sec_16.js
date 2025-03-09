import React, { useState, useEffect } from 'react';
import { Alert, AlertDescription } from '@/components/ui/alert';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';

// Validation rules
const rules = {
  required: (value) => (value ? '' : 'Dieses Feld ist erforderlich'),
  email: (value) => {
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return emailRegex.test(value) ? '' : 'Ungültige E-Mail-Adresse';
  },
  minLength: (length) => (value) =>
    value.length >= length ? '' : `Mindestens ${length} Zeichen erforderlich`,
  passwordStrength: (value) => {
    const hasUpperCase = /[A-Z]/.test(value);
    const hasLowerCase = /[a-z]/.test(value);
    const hasNumbers = /\d/.test(value);
    const hasSpecialChar = /[!@#$%^&*(),.?":{}|<>]/.test(value);
    
    if (value.length < 8) return 'Passwort muss mindestens 8 Zeichen lang sein';
    if (!hasUpperCase) return 'Passwort muss einen Großbuchstaben enthalten';
    if (!hasLowerCase) return 'Passwort muss einen Kleinbuchstaben enthalten';
    if (!hasNumbers) return 'Passwort muss eine Zahl enthalten';
    if (!hasSpecialChar) return 'Passwort muss ein Sonderzeichen enthalten';
    return '';
  },
};

// Async validation example (simulating API call)
const checkEmailAvailability = async (email) => {
  await new Promise(resolve => setTimeout(resolve, 1000));
  return email.includes('taken') ? 'Diese E-Mail ist bereits vergeben' : '';
};

const FormValidator = () => {
  const [formData, setFormData] = useState({
    email: '',
    password: '',
    confirmPassword: '',
    accountType: 'personal',
    companyName: '',
    vatNumber: '',
  });

  const [errors, setErrors] = useState({});
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [validationGroup, setValidationGroup] = useState('all');

  // Validation groups definition
  const validationGroups = {
    all: ['email', 'password', 'confirmPassword', 'accountType'],
    personal: ['email', 'password', 'confirmPassword'],
    business: ['email', 'password', 'confirmPassword', 'companyName', 'vatNumber'],
  };

  // Field dependencies
  const conditionalFields = {
    companyName: (data) => data.accountType === 'business',
    vatNumber: (data) => data.accountType === 'business',
  };

  // Cross-field validation
  const crossFieldValidation = {
    confirmPassword: (value, data) =>
      value === data.password ? '' : 'Passwörter stimmen nicht überein',
  };

  const validateField = async (fieldName, value, allData) => {
    let fieldErrors = [];

    // Skip validation for conditional fields that shouldn't be shown
    if (conditionalFields[fieldName] && !conditionalFields[fieldName](allData)) {
      return '';
    }

    // Apply basic validation rules
    switch (fieldName) {
      case 'email':
        fieldErrors.push(rules.required(value));
        fieldErrors.push(rules.email(value));
        if (!fieldErrors.find(error => error)) {
          const asyncError = await checkEmailAvailability(value);
          if (asyncError) fieldErrors.push(asyncError);
        }
        break;
      case 'password':
        fieldErrors.push(rules.required(value));
        fieldErrors.push(rules.passwordStrength(value));
        break;
      case 'confirmPassword':
        fieldErrors.push(rules.required(value));
        if (crossFieldValidation[fieldName]) {
          fieldErrors.push(crossFieldValidation[fieldName](value, allData));
        }
        break;
      case 'companyName':
        if (allData.accountType === 'business') {
          fieldErrors.push(rules.required(value));
          fieldErrors.push(rules.minLength(3)(value));
        }
        break;
      case 'vatNumber':
        if (allData.accountType === 'business') {
          fieldErrors.push(rules.required(value));
          // Add custom VAT number validation if needed
        }
        break;
    }

    return fieldErrors.find(error => error) || '';
  };

  const validateForm = async (group = validationGroup) => {
    const fieldsToValidate = validationGroups[group];
    const newErrors = {};
    
    for (const field of fieldsToValidate) {
      const error = await validateField(field, formData[field], formData);
      if (error) newErrors[field] = error;
    }
    
    setErrors(newErrors);
    return Object.keys(newErrors).length === 0;
  };

  const handleChange = async (e) => {
    const { name, value } = e.target;
    const newFormData = { ...formData, [name]: value };
    setFormData(newFormData);

    // Validate changed field
    const error = await validateField(name, value, newFormData);
    setErrors(prev => ({ ...prev, [name]: error }));

    // Validate dependent fields
    if (name === 'password' && newFormData.confirmPassword) {
      const confirmError = await validateField('confirmPassword', newFormData.confirmPassword, newFormData);
      setErrors(prev => ({ ...prev, confirmPassword: confirmError }));
    }
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    setIsSubmitting(true);

    try {
      const isValid = await validateForm();
      if (isValid) {
        console.log('Form submitted successfully:', formData);
        // Handle successful submission
      }
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <Card className="w-full max-w-lg mx-auto">
      <CardHeader>
        <CardTitle>Registrierung</CardTitle>
      </CardHeader>
      <CardContent>
        <form onSubmit={handleSubmit} className="space-y-4">
          <div className="space-y-2">
            <label className="block">
              Account-Typ
              <select
                name="accountType"
                value={formData.accountType}
                onChange={handleChange}
                className="w-full p-2 border rounded"
              >
                <option value="personal">Persönlich</option>
                <option value="business">Geschäftlich</option>
              </select>
            </label>
          </div>

          <div className="space-y-2">
            <label className="block">
              E-Mail
              <input
                type="email"
                name="email"
                value={formData.email}
                onChange={handleChange}
                className="w-full p-2 border rounded"
              />
            </label>
            {errors.email && (
              <Alert variant="destructive">
                <AlertDescription>{errors.email}</AlertDescription>
              </Alert>
            )}
          </div>

          <div className="space-y-2">
            <label className="block">
              Passwort
              <input
                type="password"
                name="password"
                value={formData.password}
                onChange={handleChange}
                className="w-full p-2 border rounded"
              />
            </label>
            {errors.password && (
              <Alert variant="destructive">
                <AlertDescription>{errors.password}</AlertDescription>
              </Alert>
            )}
          </div>

          <div className="space-y-2">
            <label className="block">
              Passwort bestätigen
              <input
                type="password"
                name="confirmPassword"
                value={formData.confirmPassword}
                onChange={handleChange}
                className="w-full p-2 border rounded"
              />
            </label>
            {errors.confirmPassword && (
              <Alert variant="destructive">
                <AlertDescription>{errors.confirmPassword}</AlertDescription>
              </Alert>
            )}
          </div>

          {formData.accountType === 'business' && (
            <>
              <div className="space-y-2">
                <label className="block">
                  Firmenname
                  <input
                    type="text"
                    name="companyName"
                    value={formData.companyName}
                    onChange={handleChange}
                    className="w-full p-2 border rounded"
                  />
                </label>
                {errors.companyName && (
                  <Alert variant="destructive">
                    <AlertDescription>{errors.companyName}</AlertDescription>
                  </Alert>
                )}
              </div>

              <div className="space-y-2">
                <label className="block">
                  USt-IdNr.
                  <input
                    type="text"
                    name="vatNumber"
                    value={formData.vatNumber}
                    onChange={handleChange}
                    className="w-full p-2 border rounded"
                  />
                </label>
                {errors.vatNumber && (
                  <Alert variant="destructive">
                    <AlertDescription>{errors.vatNumber}</AlertDescription>
                  </Alert>
                )}
              </div>
            </>
          )}

          <button
            type="submit"
            disabled={isSubmitting}
            className="w-full p-2 text-white bg-blue-600 rounded hover:bg-blue-700 disabled:bg-blue-300"
          >
            {isSubmitting ? 'Wird gesendet...' : 'Registrieren'}
          </button>
        </form>
      </CardContent>
    </Card>
  );
};

export default FormValidator;