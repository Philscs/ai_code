import numpy as np
from dataclasses import dataclass
from typing import List, Tuple, Optional
import math

@dataclass
class Vertex:
    x: float
    y: float
    z: float
    
    def to_array(self) -> np.ndarray:
        return np.array([self.x, self.y, self.z, 1.0])

@dataclass
class Face:
    vertices: List[int]  # Indices der Vertices
    normal: Optional[Tuple[float, float, float]] = None

class Mesh:
    def __init__(self):
        self.vertices: List[Vertex] = []
        self.faces: List[Face] = []
        
    def add_vertex(self, x: float, y: float, z: float) -> int:
        self.vertices.append(Vertex(x, y, z))
        return len(self.vertices) - 1
        
    def add_face(self, vertex_indices: List[int]) -> None:
        self.faces.append(Face(vertex_indices))
        
    def calculate_normals(self) -> None:
        for face in self.faces:
            if len(face.vertices) >= 3:
                v0 = np.array([self.vertices[face.vertices[0]].x,
                             self.vertices[face.vertices[0]].y,
                             self.vertices[face.vertices[0]].z])
                v1 = np.array([self.vertices[face.vertices[1]].x,
                             self.vertices[face.vertices[1]].y,
                             self.vertices[face.vertices[1]].z])
                v2 = np.array([self.vertices[face.vertices[2]].x,
                             self.vertices[face.vertices[2]].y,
                             self.vertices[face.vertices[2]].z])
                
                # Normale über Kreuzprodukt berechnen
                edge1 = v1 - v0
                edge2 = v2 - v0
                normal = np.cross(edge1, edge2)
                normal = normal / np.linalg.norm(normal)
                face.normal = tuple(normal)

class Transform:
    def __init__(self):
        self.matrix = np.identity(4)
    
    def translate(self, x: float, y: float, z: float) -> 'Transform':
        translation = np.identity(4)
        translation[0:3, 3] = [x, y, z]
        self.matrix = np.dot(translation, self.matrix)
        return self
    
    def rotate_x(self, angle: float) -> 'Transform':
        c = math.cos(angle)
        s = math.sin(angle)
        rotation = np.identity(4)
        rotation[1:3, 1:3] = [[c, -s],
                             [s, c]]
        self.matrix = np.dot(rotation, self.matrix)
        return self
    
    def rotate_y(self, angle: float) -> 'Transform':
        c = math.cos(angle)
        s = math.sin(angle)
        rotation = np.identity(4)
        rotation[0:3:2, 0:3:2] = [[c, -s],
                                 [s, c]]
        self.matrix = np.dot(rotation, self.matrix)
        return self
    
    def rotate_z(self, angle: float) -> 'Transform':
        c = math.cos(angle)
        s = math.sin(angle)
        rotation = np.identity(4)
        rotation[0:2, 0:2] = [[c, -s],
                             [s, c]]
        self.matrix = np.dot(rotation, self.matrix)
        return self
    
    def scale(self, x: float, y: float, z: float) -> 'Transform':
        scaling = np.identity(4)
        scaling[0:3, 0:3] = np.diag([x, y, z])
        self.matrix = np.dot(scaling, self.matrix)
        return self

class Camera:
    def __init__(self, position: Tuple[float, float, float],
                 target: Tuple[float, float, float],
                 up: Tuple[float, float, float] = (0, 1, 0)):
        self.position = np.array(position)
        self.target = np.array(target)
        self.up = np.array(up)
        
    def get_view_matrix(self) -> np.ndarray:
        # Kamera-Koordinatensystem berechnen
        forward = self.target - self.position
        forward = forward / np.linalg.norm(forward)
        
        right = np.cross(forward, self.up)
        right = right / np.linalg.norm(right)
        
        new_up = np.cross(right, forward)
        
        # View-Matrix erstellen
        view_matrix = np.identity(4)
        view_matrix[0:3, 0] = right
        view_matrix[0:3, 1] = new_up
        view_matrix[0:3, 2] = -forward
        view_matrix[0:3, 3] = -self.position
        
        return view_matrix

class Renderer:
    def __init__(self, width: int, height: int):
        self.width = width
        self.height = height
        self.clear_color = (0, 0, 0)
        self.frame_buffer = np.zeros((height, width, 3), dtype=np.uint8)
        self.depth_buffer = np.full((height, width), np.inf)
        
    def clear(self) -> None:
        self.frame_buffer.fill(0)
        self.depth_buffer.fill(np.inf)
        
    def draw_mesh(self, mesh: Mesh, transform: Transform, camera: Camera) -> None:
        # Transformationsmatrizen kombinieren
        view_matrix = camera.get_view_matrix()
        mvp_matrix = np.dot(view_matrix, transform.matrix)
        
        # Vertices transformieren
        transformed_vertices = []
        for vertex in mesh.vertices:
            pos = np.dot(mvp_matrix, vertex.to_array())
            pos = pos[:3] / pos[3]  # Perspektivische Division
            transformed_vertices.append(pos)
        
        # Faces zeichnen (vereinfachte Version)
        for face in mesh.faces:
            if len(face.vertices) >= 3:
                # Backface Culling
                v0 = transformed_vertices[face.vertices[0]]
                v1 = transformed_vertices[face.vertices[1]]
                v2 = transformed_vertices[face.vertices[2]]
                
                # Normale im View-Space berechnen
                edge1 = v1 - v0
                edge2 = v2 - v0
                normal = np.cross(edge1, edge2)
                
                # Face überspringen, wenn sie vom Betrachter weg zeigt
                if np.dot(normal, v0) >= 0:
                    continue
                
                # Hier würde der eigentliche Rasterisierungscode folgen
                # (Vereinfacht - nur Vertices als Punkte darstellen)
                for vertex_idx in face.vertices:
                    pos = transformed_vertices[vertex_idx]
                    if -1 <= pos[0] <= 1 and -1 <= pos[1] <= 1:
                        screen_x = int((pos[0] + 1) * self.width * 0.5)
                        screen_y = int((1 - pos[1]) * self.height * 0.5)
                        
                        if 0 <= screen_x < self.width and 0 <= screen_y < self.height:
                            if pos[2] < self.depth_buffer[screen_y, screen_x]:
                                self.depth_buffer[screen_y, screen_x] = pos[2]
                                self.frame_buffer[screen_y, screen_x] = (255, 255, 255)

class OBJLoader:
    @staticmethod
    def load(filename: str) -> Mesh:
        mesh = Mesh()
        
        with open(filename, 'r') as f:
            for line in f:
                if line.startswith('v '):  # Vertex
                    _, x, y, z = line.split()
                    mesh.add_vertex(float(x), float(y), float(z))
                elif line.startswith('f '):  # Face
                    _, *vertices = line.split()
                    # Vereinfachte Version - nimmt nur Vertex-Indices
                    face_vertices = [int(v.split('/')[0]) - 1 for v in vertices]
                    mesh.add_face(face_vertices)
        
        mesh.calculate_normals()
        return mesh

# Example usage
processor = GraphicsProcessor()
processor.load_object("object1.obj")
processor.load_object("object2.stl")
processor.apply_transformation("translate(10, 0, 0)")
processor.render("ray_tracing")

def main():
    # 3D-Modell laden
    mesh = OBJLoader.load('model.obj')
    
    # Transformation einrichten
    transform = Transform()
    transform.translate(0, 0, -5).rotate_y(math.radians(45))
    
    # Kamera einrichten
    camera = Camera(
        position=(0, 0, 10),
        target=(0, 0, 0),
        up=(0, 1, 0)
    )
    
    # Renderer einrichten und Szene rendern
    renderer = Renderer(800, 600)
    renderer.clear()
    renderer.draw_mesh(mesh, transform, camera)
    
    # Hier könnte man das Frame-Buffer als Bild speichern
    # z.B. mit PIL oder OpenCV

if __name__ == "__main__":
    main()
