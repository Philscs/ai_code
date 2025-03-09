import pygame
import numpy as np
import math

class Punkt:
    def __init__(self, x=0, y=0, z=0):
        self.x = x
        self.y = y
        self.z = z

    def rotateX(self, angle):
        rad = math.radians(angle)
        self.y = self.y * math.cos(rad) - self.x * math.sin(rad)
        self.x = self.x * math.cos(rad) + self.y * math.sin(rad)

    def rotateY(self, angle):
        rad = math.radians(angle)
        self.x = self.x * math.cos(rad) + self.y * math.sin(rad)
        self.y = -self.x * math.sin(rad) + self.y * math.cos(rad)

    def rotXZ(self, angle):
        rad = math.radians(angle)
        self.z = self.z * math.cos(rad) - self.y * math.sin(rad)
        self.y = self.y * math.cos(rad) + self.z * math.sin(rad)

class Grafik:
    def __init__(self, punkte=None):
        if punkte is None:
            punkte = []
        self.punkte = punkte
        self.projection_matrix = np.array([
            [1, 0, 0, 0],
            [0, 1, 0, 0],
            [0, 0, 1, 0],
            [0, 0, -5, 1]
        ])

    def importe(self, datei):
        self.punkte = []
        with open(datei, 'r') as f:
            for line in f:
                x, y, z = map(float, line.split(' '))
                punkt = Punkt(x, y, z)
                self.punkte.append(punkt)

    def exportiere(self, datei):
        mit_punkten = ''
        with open(datei, 'w') as f:
            for punkt in self.punkte:
                x, y, z = punkt.x, punkt.y, punkt.z
                f.write(f'{x} {y} {z}\n')

    def zeichne(self, screen):
        pygame.init()
        screen = pygame.display.set_mode((800, 600))
        clock = pygame.time.Clock()

        running = True
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False

            screen.fill((0,0,0))

            # Rotationen
            for punkt in self.punkte:
                punkt.rotateX(5)
                punkt.rotateY(10)

            # Projektion
            projection_matrix = np.eye(4)
            projection_matrix[2][2] = -1 / len(self.punkte)[0]
            for i, punkt in enumerate(self.punkte):
                punkt.x = (punkt.x + punkt.y + punkt.z) * 100 / (np.linalg.norm([punkt.x, 
punkt.y, punkt.z]) + 5)
                punkt.y = (punkt.x - punkt.y + punkt.z) * 200 / (np.linalg.norm([punkt.x, 
punkt.y, punkt.z]) + 5)

            for i, punkt in enumerate(self.punkte):
                x, y = int(punkt.x), int(punkt.y)
                screen.set_at((x, y), (255,0,0))  # Rotes

            pygame.display.flip()
            clock.tick(60)

        pygame.quit()

