import numpy as np

class NeuralNetwork:
    def __init__(self, layers, activation='sigmoid'):
        self.layers = layers
        self.activation = activation
        self.weights = []
        self.biases = []
        self.initialize_weights()

    def initialize_weights(self):
        for i in range(1, len(self.layers)):
            self.weights.append(np.random.randn(self.layers[i], self.layers[i-1]))
            self.biases.append(np.random.randn(self.layers[i], 1))

    def activate(self, x):
        if self.activation == 'sigmoid':
            return 1 / (1 + np.exp(-x))
        elif self.activation == 'relu':
            return np.maximum(0, x)
        elif self.activation == 'tanh':
            return np.tanh(x)
        else:
            raise ValueError('Invalid activation function')

    def forward_propagation(self, x):
        a = x
        for i in range(len(self.layers) - 1):
            z = np.dot(self.weights[i], a) + self.biases[i]
            a = self.activate(z)
        return a

    def train(self, X, y, learning_rate=0.1, epochs=1000):
        for epoch in range(epochs):
            for i in range(len(X)):
                x = X[i].reshape(-1, 1)
                y_true = y[i].reshape(-1, 1)

                # Forward propagation
                a = self.forward_propagation(x)

                # Backpropagation
                delta = a - y_true
                for j in range(len(self.layers) - 2, -1, -1):
                    dz = delta * self.activate(a, derivative=True)
                    dw = np.dot(dz, a.T)
                    db = dz
                    delta = np.dot(self.weights[j].T, dz)
                    self.weights[j] -= learning_rate * dw
                    self.biases[j] -= learning_rate * db

    def predict(self, X):
        predictions = []
        for x in X:
            x = x.reshape(-1, 1)
            y_pred = self.forward_propagation(x)
            predictions.append(y_pred)
        return predictions
