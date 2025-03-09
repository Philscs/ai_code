const paymentOptions = {
    // Die Anzahl der Zahlungsmethoden
    supportedMethods: ['card'],
    // Die Ziel-Währung
    currency: 'EUR',
    // Die Mindest- und Höchstbetrag
    minAmount: 10,
    maxAmount: 1000,
  };
  const paymentRequest = {
    method: 'card',
    amount: {
      currency: 'EUR',
      value: 50,
    },
    billingAddress: {
      line1: 'Stree',
      addressState: 'Wien',
      postalCode: '1090',
      city: 'Wien',
    },
  };
  function validatePaymentRequest(paymentRequest) {
    // Überprüfen Sie den Name des Benutzers
    if (!paymentRequest.billingAddress.line1) {
      throw new Error('Name erforderlich');
    }
  
    // Überprüfen Sie die Adresse
    if (!paymentRequest.billingAddress.addressState || !paymentRequest.billingAddress.postalCode) {
      throw new Error('Adresse erforderlich');
    }
  }
  function detectFraud(paymentRequest) {
    // Überprüfen Sie, ob der Benutzer einen gültigen Kreditkarten-Inhaber ist
    if (!paymentRequest.method.supportedMethods.includes('card')) {
      throw new Error('Betrug erkannt');
    }
  
    // Überprüfen Sie, ob der Betrag ungewöhnlich groß ist
    if (paymentRequest.amount.value > 1000) {
      throw new Error('Betrug erkannt');
    }
  }
  function handleErrors(error) {
    // Wenn ein Fehler auftritt, anzeigen Sie einen fehlerhaften Zustand
    if (error.message === 'Name erforderlich') {
      alert('Name ist erforderlich');
    } else if (error.message === 'Adresse erforderlich') {
      alert('Adresse ist erforderlich');
    }
  }
  async function processPayment(paymentOptions, paymentRequest) {
    try {
      // Erstellen Sie eine Payment-Request
      const options = await navigator.paymentRequest({
        ...paymentOptions,
        paymentMethodId: paymentRequest.method.id,
        amount: {
          currency: paymentOptions.currency,
          value: paymentRequest.amount.value,
        },
      });
  
      // Validieren Sie die Zahlungsanforderungen
      validatePaymentRequest(options);
  
      // Erkennen von Betrug
      detectFraud(options);
  
      // Verarbeiten der Antwort
      const response = await options.resolve();
      console.log(response);
    } catch (error) {
      handleErrors(error);
    }
  }
  const paymentOptions = {
    supportedMethods: ['card'],
    currency: 'EUR',
    minAmount: 10,
    maxAmount: 1000,
  };
  
  const paymentRequest = {
    method: 'card',
    amount: {
      currency: 'EUR',
      value: 50,
    },
    billingAddress: {
      line1: 'Stree',
      addressState: 'Wien',
      postalCode: '1090',
      city: 'Wien',
    },
  };
  
  processPayment(paymentOptions, paymentRequest);