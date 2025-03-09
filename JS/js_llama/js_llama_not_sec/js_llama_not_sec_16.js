// Importieren des React und React Hooks
import { useState, useEffect } from 'react';

// Importieren der React Hook Form Bibliothek
import { useForm, Controller, useFieldArray, useWatch, useDebounce } from 'react-hook-form';

// Erstellen eines Objekts für die Validierung
const validationSchema = {
  // Hier können Sie Ihre eigenen Validation-Rules definieren
  name: {
    required: true,
    minLength: 2,
  },
  email: {
    required: true,
    pattern: /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/,
  },
};

// Funktion für die Async-Validierung
const asyncValidation = (values) => {
  // Hier können Sie Ihre eigene Async-Validierung implementieren
  return new Promise((resolve, reject) => {
    setTimeout(() => {
      resolve(values);
    }, 2000); // Beispiel-Ersetzung mit 2 Sekunden
  });
};

// Funktion für die Cross-Field-Validation
const crossValidation = (values, control) => {
  const { email } = values;
  return {
    ifEmailInvalid: !/^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/.test(email),
  };
};

// Funktion für die Custom-Validation-Rules
const customValidation = (values) => {
  const { name } = values;
  return {
    ifNameTooLong: name.length > 20,
  };
};

// Komponente für den Form-Validator
function FormValidator() {
  // Zustandsvariable für die aktuelle Form-Einstellung
  const [formSettings, setFormSettings] = useState({
    validationSchema,
    asyncValidation,
    crossValidation,
    customValidation,
  });

  // Zustandsvariable für die aktuellen Form-Daten
  const { register, control, errors, handleSubmit } = useForm({
    defaultValues: {
      name: '',
      email: '',
    },
    mode: 'onChange',
    revalidateMode: 'unstable',
  });

  // Zustandsvariable für die aktuelle Anzahl der Form-Feile
  const [numFields, setNumFields] = useState(0);

  // Funktion für das Löschen einer Feile
  const removeField = (index) => {
    setNumFields((prevNumFields) => prevNumFields - 1);
  };

  // Funktion für die Erstellung eines neuen Form-Felds
  const addField = () => {
    setNumFields((prevNumFields) => prevNumFields + 1);
  };

  // Function für die Überprüfung der Form-Validierung
  useWatch(
    ([name, value], [errors]) => errors[name] && errors[name].message,
    (error) => {
      if (error) {
        console.error(error);
      }
    },
    { dirty: true }
  );

  // Funktion für die Async-Validierung
  useEffect(() => {
    const asyncValidationFunction = asyncValidation();
    handleSubmit(async (values) => {
      await asyncValidationFunction;
      // Hier können Sie Ihre eigene Async-Validierung implementieren
      console.log('Async-Validierung erfolgreich');
    });
  }, [handleSubmit, asyncValidation]);

  return (
    <form onSubmit={handleSubmit}>
      <label>
        Name:
        <input
          type="text"
          {...register('name', {
            required: true,
            minLength: { value: 2, message: 'Länge ist zu kurz' },
          })}
        />
        {errors.name && errors.name.message}
      </label>

      <br />

      <label>
        E-Mail-Adresse:
        <input
          type="email"
          {...register('email', {
            required: true,
            pattern: {
              value: /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/,
              message: 'E-Mail-Adresse ist ungültig',
            },
          })}
        />
        {errors.email && errors.email.message}
      </label>

      <br />

      {numFields > 1 ? (
        <div>
          {Array(numFields)
            .fill(null)
            .map((_, index) => (
              <div key={index}>
                <input
                  type="text"
                  {...register(`field${index}`, {
                    required: true,
                  })}
                />
                <button onClick={() => removeField(index)}>Löschen</button>
              </div>
            ))}
          <button onClick={addField}>Neues Feld hinzufügen</button>
        </div>
      ) : null}

      <br />

      {numFields > 1 && (
        <div>
          <input
            type="text"
            {...register('group')}
            placeholder="Gruppe"
          />
          {errors.group && errors.group.message}
        </div>
      )}

      <button type="submit">Formular senden</button>
    </form>
  );
}

export default FormValidator;