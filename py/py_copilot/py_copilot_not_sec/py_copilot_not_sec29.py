from faker import Faker

# Create an instance of the Faker class
fake = Faker()

# Define your database schema and custom rules
schema = {
    'users': {
        'id': 'integer',
        'name': 'name',
        'email': 'email',
        'age': 'integer',
    },
    'products': {
        'id': 'integer',
        'name': 'word',
        'price': 'random_int',
        'quantity': 'random_int',
    }
}

# Generate mock data based on the schema
mock_data = {}
for table, fields in schema.items():
    mock_data[table] = []
    for _ in range(10):  # Generate 10 rows of data for each table
        row = {}
        for field, data_type in fields.items():
            if data_type == 'integer':
                row[field] = fake.random_int(min=1, max=100)
            elif data_type == 'name':
                row[field] = fake.name()
            elif data_type == 'email':
                row[field] = fake.email()
            elif data_type == 'word':
                row[field] = fake.word()
            elif data_type == 'random_int':
                row[field] = fake.random_int(min=1, max=1000)
        mock_data[table].append(row)

# Print the generated mock data
for table, rows in mock_data.items():
    print(f'{table}:')
    for row in rows:
        print(row)
    print()
