import numpy as np

# Aktivierungsfunktionen
class ActivationFunctions:
    @staticmethod
    def sigmoid(x):
        return 1 / (1 + np.exp(-x))

    @staticmethod
    def sigmoid_derivative(x):
        return x * (1 - x)

    @staticmethod
    def relu(x):
        return np.maximum(0, x)

    @staticmethod
    def relu_derivative(x):
        return np.where(x > 0, 1, 0)

# Netzwerkarchitektur
class NeuralNetwork:
    def __init__(self, layers, activation='sigmoid'):
        self.layers = layers
        self.weights = []
        self.biases = []
        self.activation_name = activation
        self.activation = getattr(ActivationFunctions, activation)
        self.activation_derivative = getattr(ActivationFunctions, f"{activation}_derivative")

        # Initialisierung der Gewichte und Biases
        for i in range(len(layers) - 1):
            self.weights.append(np.random.randn(layers[i], layers[i+1]) * 0.1)
            self.biases.append(np.zeros((1, layers[i+1])))

    def forward(self, X):
        self.layer_inputs = []
        self.layer_outputs = []

        input = X
        for w, b in zip(self.weights, self.biases):
            z = np.dot(input, w) + b
            self.layer_inputs.append(z)
            input = self.activation(z)
            self.layer_outputs.append(input)

        return input

    def backward(self, X, y, learning_rate):
        m = X.shape[0]
        deltas = []

        # Fehlerberechnung der letzten Schicht
        error = self.layer_outputs[-1] - y
        delta = error * self.activation_derivative(self.layer_outputs[-1])
        deltas.append(delta)

        # Rückwärtspropagation
        for i in range(len(self.weights) - 1, 0, -1):
            delta = np.dot(deltas[-1], self.weights[i].T) * self.activation_derivative(self.layer_outputs[i-1])
            deltas.append(delta)

        deltas.reverse()

        # Gewichte und Biases aktualisieren
        for i in range(len(self.weights)):
            self.weights[i] -= learning_rate * np.dot(self.layer_outputs[i-1].T if i > 0 else X.T, deltas[i]) / m
            self.biases[i] -= learning_rate * np.sum(deltas[i], axis=0, keepdims=True) / m

    def train(self, X, y, epochs, learning_rate):
        for epoch in range(epochs):
            output = self.forward(X)
            self.backward(X, y, learning_rate)
            loss = np.mean((y - output) ** 2)
            if epoch % 100 == 0:
                print(f"Epoch {epoch}, Loss: {loss}")

# Trainingsalgorithmus (Beispiel: Batch-Training)
def train_neural_network():
    # Daten generieren (z. B. XOR-Problem)
    X = np.array([[0, 0], [0, 1], [1, 0], [1, 1]])
    y = np.array([[0], [1], [1], [0]])

    # Netzwerk erstellen
    nn = NeuralNetwork(layers=[2, 4, 1], activation='sigmoid')

    # Trainieren
    nn.train(X, y, epochs=1000, learning_rate=0.1)

    # Ausgabe der Ergebnisse
    print("Final Output:")
    print(nn.forward(X))

if __name__ == "__main__":
    train_neural_network()
