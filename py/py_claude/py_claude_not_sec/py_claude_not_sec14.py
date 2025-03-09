import numpy as np
from typing import List, Callable, Union, Optional

class Layer:
    def __init__(self, input_size: int, output_size: int, activation: str = 'relu'):
        """Initialisiert eine Layer mit zufälligen Gewichten und Bias."""
        self.weights = np.random.randn(input_size, output_size) * 0.01
        self.bias = np.zeros((1, output_size))
        self.activation = self._get_activation_function(activation)
        self.activation_name = activation
        
    def _get_activation_function(self, name: str) -> Callable:
        """Wählt die Aktivierungsfunktion basierend auf dem Namen."""
        activations = {
            'relu': lambda x: np.maximum(0, x),
            'sigmoid': lambda x: 1 / (1 + np.exp(-x)),
            'tanh': lambda x: np.tanh(x),
            'linear': lambda x: x
        }
        return activations.get(name, activations['relu'])
    
    def forward(self, inputs: np.ndarray) -> np.ndarray:
        """Forward Pass durch die Layer."""
        self.inputs = inputs
        self.z = np.dot(inputs, self.weights) + self.bias
        self.output = self.activation(self.z)
        return self.output

    def backward(self, gradient: np.ndarray, learning_rate: float) -> np.ndarray:
        """Backward Pass mit Gradienten-Update."""
        if self.activation_name == 'relu':
            activation_gradient = gradient * (self.z > 0)
        elif self.activation_name == 'sigmoid':
            activation_gradient = gradient * self.output * (1 - self.output)
        elif self.activation_name == 'tanh':
            activation_gradient = gradient * (1 - np.square(self.output))
        else:  # linear
            activation_gradient = gradient
            
        weights_gradient = np.dot(self.inputs.T, activation_gradient)
        input_gradient = np.dot(activation_gradient, self.weights.T)
        
        # Gewichte und Bias aktualisieren
        self.weights -= learning_rate * weights_gradient
        self.bias -= learning_rate * np.sum(activation_gradient, axis=0, keepdims=True)
        
        return input_gradient

class NeuralNetwork:
    def __init__(self, layer_sizes: List[int], activations: List[str]):
        """Initialisiert ein neuronales Netz mit den gegebenen Layer-Größen und Aktivierungsfunktionen."""
        self.layers = []
        for i in range(len(layer_sizes) - 1):
            self.layers.append(Layer(layer_sizes[i], layer_sizes[i + 1], activations[i]))
    
    def forward(self, x: np.ndarray) -> np.ndarray:
        """Forward Pass durch das gesamte Netzwerk."""
        current_input = x
        for layer in self.layers:
            current_input = layer.forward(current_input)
        return current_input
    
    def backward(self, gradient: np.ndarray, learning_rate: float):
        """Backward Pass durch das gesamte Netzwerk."""
        current_gradient = gradient
        for layer in reversed(self.layers):
            current_gradient = layer.backward(current_gradient, learning_rate)
    
    def train(self, X: np.ndarray, y: np.ndarray, epochs: int, learning_rate: float, 
              batch_size: Optional[int] = None) -> List[float]:
        """Trainiert das Netzwerk mit den gegebenen Daten."""
        losses = []
        n_samples = X.shape[0]
        
        if batch_size is None:
            batch_size = n_samples
            
        for epoch in range(epochs):
            # Daten mischen
            indices = np.random.permutation(n_samples)
            X_shuffled = X[indices]
            y_shuffled = y[indices]
            
            total_loss = 0
            # Mini-Batch Training
            for i in range(0, n_samples, batch_size):
                X_batch = X_shuffled[i:i + batch_size]
                y_batch = y_shuffled[i:i + batch_size]
                
                # Forward Pass
                predictions = self.forward(X_batch)
                
                # Verlust berechnen (MSE)
                loss = np.mean(np.square(predictions - y_batch))
                total_loss += loss
                
                # Gradient für MSE
                gradient = 2 * (predictions - y_batch) / batch_size
                
                # Backward Pass
                self.backward(gradient, learning_rate)
            
            avg_loss = total_loss / (n_samples / batch_size)
            losses.append(avg_loss)
            
            if epoch % 100 == 0:
                print(f'Epoch {epoch}, Loss: {avg_loss:.4f}')
                
        return losses

# Beispiel für die Verwendung:
if __name__ == "__main__":
    # XOR Problem
    X = np.array([[0, 0], [0, 1], [1, 0], [1, 1]])
    y = np.array([[0], [1], [1], [0]])
    
    # Netzwerk mit 2 Hidden Layers
    network = NeuralNetwork(
        layer_sizes=[2, 4, 4, 1],
        activations=['relu', 'relu', 'sigmoid']
    )
    
    # Training
    losses = network.train(X, y, epochs=1000, learning_rate=0.01, batch_size=4)
    
    # Vorhersagen
    predictions = network.forward(X)
    print("\nVorhersagen:")
    for i in range(len(X)):
        print(f"Input: {X[i]}, Target: {y[i][0]}, Prediction: {predictions[i][0]:.4f}")