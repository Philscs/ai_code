import random
from enum import Enum
from datetime import datetime, timedelta
from typing import Dict, List, Callable

class Feature(Enum):
    FEATURE_A = "FeatureA"
    FEATURE_B = "FeatureB"

class User:
    def __init__(self, user_id: int):
        self.user_id = user_id

class FeatureFlagSystem:
    def __init__(self):
        self.features: Dict[str, Feature] = {}
        self.users: Dict[int, User] = {}
        self.experiments: Dict[str, Dict[str, str]] = {}

    def add_feature(self, name: str, feature: Feature):
        if name in self.features:
            raise ValueError(f"Feature '{name}' bereits existiert.")
        self.features[name] = feature

    def create_user(self, user_id: int):
        if user_id in self.users:
            raise ValueError(f"Benutzer mit ID '{user_id}' bereits existiert.")
        self.users[user_id] = User(user_id)

    def add_experiment(self, experiment_name: str, feature_name: Feature, duration: timedelta, 
probability: float):
        if experiment_name in self.experiments:
            raise ValueError(f"Experiment mit Name '{experiment_name}' bereits existiert.")
        self.experiments[experiment_name] = {
            "feature": feature_name.value,
            "duration": duration.total_seconds(),
            "probability": probability
        }

    def update_user_feature(self, user_id: int, feature_name: Feature):
        if user_id not in self.users:
            raise ValueError(f"Benutzer mit ID '{user_id}' existiert nicht.")
        if feature_name.value not in self.features:
            raise ValueError(f"Feature '{feature_name.value}' existiert nicht.")

    def update_user_feature_probability(self, user_id: int, experiment_name: str):
        if user_id not in self.users:
            raise ValueError(f"Benutzer mit ID '{user_id}' existiert nicht.")
        if experiment_name not in self.experiments:
            raise ValueError(f"Experiment mit Name '{experiment_name}' existiert nicht.")

    def execute_experiment(self, experiment_name: str, current_time: datetime):
        if experiment_name not in self.experiments:
            return

        feature_name = self.experiments[experiment_name]["feature"]
        duration_seconds = self.experiments[experiment_name]["duration"]

        # Graduelle Einführung
        time_elapsed = (current_time - datetime(1970, 1, 1)).total_seconds()
        if time_elapsed < duration_seconds:
            probability = time_elapsed / duration_seconds
        else:
            probability = self.experiments[experiment_name]["probability"]
        
        return random.random() < probability

class ABRoller:
    def __init__(self, feature_flag_system: FeatureFlagSystem):
        self.feature_flag_system = feature_flag_system

    def update_user_feature(self, user_id: int, feature_name: Feature):
        self.feature_flag_system.update_user_feature(user_id, feature_name)

    def get_user_features(self, user_id: int) -> List[Feature]:
        return [feature for feature in self.feature_flag_system.features.values() if 
feature_name.value in self.feature_flag_system.users[user_id].user_id]

# Beispiel usage
if __name__ == "__main__":
    # Erstelle die Feature-Flag-System-Klasse
    feature_flags = FeatureFlagSystem()

    # Füge Features hinzu
    feature_flags.add_feature("FeatureA", Feature.FEATURE_A)
    feature_flags.add_feature("FeatureB", Feature.FEATURE_B)

    # Erstelle Benutzer
    user1 = feature_flags.create_user(1)
    user2 = feature_flags.create_user(2)

    # Füge Experimente hinzu
    feature_flags.add_experiment("ExperimentA", Feature.FEATURE_A, timedelta(days=30), 0.5)
    feature_flags.add_experiment("ExperimentB", Feature.FEATURE_B, timedelta(days=30), 0.3)

    # Roll the experiment for user1 after 30 days from now
    a_b_roller = ABRoller(feature_flags)
    future_time = datetime.now() + timedelta(days=30) 
    current_time = datetime.now()
    result_a = feature_flags.execute_experiment("ExperimentA", current_time)
    result_b = feature_flags.execute_experiment("ExperimentB", current_time)

    # Füge die Ergebnisse der Experimente zu den Benutzern hinzu
    if result_a:
        user1.user_id = "FeatureA"
    else:
        user1.user_id = "FeatureB"

    if result_b:
        user2.user_id = "FeatureA"
    else:
        user2.user_id = "FeatureB"

    print(user1.user_id)
    print(user2.user_id)
