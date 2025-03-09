import numpy as np
from pywavefront import Wavefront

class Object3D:
    def __init__(self, vertices, faces):
        self.vertices = np.array(vertices, dtype=float)
        self.faces = np.array(faces, dtype=int)

    def apply_transformation(self, matrix):
        """Applies a transformation matrix to the vertices."""
        ones = np.ones((self.vertices.shape[0], 1))
        homogeneous_vertices = np.hstack((self.vertices, ones))
        transformed = homogeneous_vertices.dot(matrix.T)
        self.vertices = transformed[:, :3]


class FileHandler:
    @staticmethod
    def load_obj(file_path):
        """Loads a Wavefront OBJ file."""
        scene = Wavefront(file_path, collect_faces=True)
        vertices = scene.vertices
        faces = [face for mesh in scene.mesh_list for face in mesh.faces]
        return Object3D(vertices, faces)

    @staticmethod
    def save_obj(object3d, file_path):
        """Saves an Object3D as an OBJ file."""
        with open(file_path, 'w') as file:
            for vertex in object3d.vertices:
                file.write(f"v {vertex[0]} {vertex[1]} {vertex[2]}\n")
            for face in object3d.faces:
                file.write(f"f {' '.join(str(idx + 1) for idx in face)}\n")


class Transformation:
    @staticmethod
    def translation(dx, dy, dz):
        """Returns a translation matrix."""
        return np.array([
            [1, 0, 0, dx],
            [0, 1, 0, dy],
            [0, 0, 1, dz],
            [0, 0, 0, 1]
        ])

    @staticmethod
    def scaling(sx, sy, sz):
        """Returns a scaling matrix."""
        return np.array([
            [sx, 0, 0, 0],
            [0, sy, 0, 0],
            [0, 0, sz, 0],
            [0, 0, 0, 1]
        ])

    @staticmethod
    def rotation_x(angle):
        """Returns a rotation matrix around the X-axis."""
        c, s = np.cos(angle), np.sin(angle)
        return np.array([
            [1, 0, 0, 0],
            [0, c, -s, 0],
            [0, s, c, 0],
            [0, 0, 0, 1]
        ])

    @staticmethod
    def rotation_y(angle):
        """Returns a rotation matrix around the Y-axis."""
        c, s = np.cos(angle), np.sin(angle)
        return np.array([
            [c, 0, s, 0],
            [0, 1, 0, 0],
            [-s, 0, c, 0],
            [0, 0, 0, 1]
        ])

    @staticmethod
    def rotation_z(angle):
        """Returns a rotation matrix around the Z-axis."""
        c, s = np.cos(angle), np.sin(angle)
        return np.array([
            [c, -s, 0, 0],
            [s, c, 0, 0],
            [0, 0, 1, 0],
            [0, 0, 0, 1]
        ])


class Renderer:
    @staticmethod
    def render_wireframe(object3d):
        """Renders the object in wireframe mode (console visualization)."""
        for face in object3d.faces:
            print("Face:")
            for idx in face:
                print(f"\tVertex: {object3d.vertices[idx]}")


# Example Usage
if __name__ == "__main__":
    # Load an OBJ file
    object3d = FileHandler.load_obj("example.obj")

    # Apply transformations
    translation_matrix = Transformation.translation(1, 2, 3)
    object3d.apply_transformation(translation_matrix)

    scaling_matrix = Transformation.scaling(2, 2, 2)
    object3d.apply_transformation(scaling_matrix)

    # Render the object
    Renderer.render_wireframe(object3d)

    # Save the transformed object
    FileHandler.save_obj(object3d, "transformed_example.obj")
