import threading
import time
import random

class Participant:
    def __init__(self, name):
        self.name = name
        self.vote = None  # "commit" or "abort"
        self.state = "initial"  # "prepared", "committed", "aborted"

    def prepare(self):
        # Simuliere eine Entscheidung zur Vorbereitung der Transaktion
        self.vote = random.choice(["commit", "abort"])
        print(f"{self.name}: Vote = {self.vote}")
        return self.vote

    def commit(self):
        self.state = "committed"
        print(f"{self.name}: State = {self.state}")

    def abort(self):
        self.state = "aborted"
        print(f"{self.name}: State = {self.state}")

class Coordinator:
    def __init__(self, participants):
        self.participants = participants
        self.global_decision = None  # "commit" or "abort"

    def start_transaction(self):
        print("Coordinator: Starting transaction")
        votes = []

        # Phase 1: Prepare
        for participant in self.participants:
            vote = participant.prepare()
            votes.append(vote)

        # Prüfe, ob alle "commit" gewählt haben
        if all(vote == "commit" for vote in votes):
            self.global_decision = "commit"
        else:
            self.global_decision = "abort"

        print(f"Coordinator: Global decision = {self.global_decision}")

        # Phase 2: Commit oder Abort
        if self.global_decision == "commit":
            for participant in self.participants:
                participant.commit()
        else:
            for participant in self.participants:
                participant.abort()

    def recovery(self):
        print("Coordinator: Recovery started")
        for participant in self.participants:
            if participant.state == "initial":
                if self.global_decision == "commit":
                    participant.commit()
                else:
                    participant.abort()
        print("Coordinator: Recovery complete")

if __name__ == "__main__":
    # Erstelle Teilnehmer*innen
    participants = [Participant(f"Participant-{i}") for i in range(3)]

    # Erstelle Koordinator
    coordinator = Coordinator(participants)

    # Starte Transaktion
    coordinator.start_transaction()

    # Simuliere Ausfall eines/r Teilnehmer*in und Wiederherstellung
    print("\nSimulating failure and recovery")
    failed_participant = participants[1]
    failed_participant.state = "initial"  # Zustand vor dem Ausfall zurücksetzen
    coordinator.recovery()
