import random
from typing import Dict, Any, Optional
from datetime import datetime
import json
import hashlib

class FeatureFlag:
    def __init__(self, name: str, enabled: bool = False, rollout_percentage: float = 0,
                 a_b_test: bool = False, a_percentage: float = 50):
        self.name = name
        self.enabled = enabled
        self.rollout_percentage = max(0, min(100, rollout_percentage))  # Zwischen 0 und 100
        self.a_b_test = a_b_test
        self.a_percentage = max(0, min(100, a_percentage))  # Zwischen 0 und 100

class FeatureFlagSystem:
    def __init__(self):
        self.flags: Dict[str, FeatureFlag] = {}
        self.user_assignments: Dict[str, Dict[str, bool]] = {}
    
    def add_flag(self, name: str, enabled: bool = False, rollout_percentage: float = 0,
                 a_b_test: bool = False, a_percentage: float = 50) -> None:
        """Fügt ein neues Feature Flag hinzu"""
        self.flags[name] = FeatureFlag(name, enabled, rollout_percentage, a_b_test, a_percentage)
    
    def _get_deterministic_value(self, user_id: str, flag_name: str) -> float:
        """Erzeugt einen deterministischen Wert zwischen 0 und 100 für einen Benutzer und Flag"""
        hash_input = f"{user_id}:{flag_name}"
        hash_value = hashlib.md5(hash_input.encode()).hexdigest()
        return float(int(hash_value, 16) % 100)
    
    def is_feature_enabled(self, flag_name: str, user_id: str) -> bool:
        """Prüft, ob ein Feature für einen bestimmten Benutzer aktiviert ist"""
        if flag_name not in self.flags:
            return False
        
        flag = self.flags[flag_name]
        
        # Wenn das Flag global deaktiviert ist
        if not flag.enabled:
            return False
            
        # Wenn der Benutzer bereits zugewiesen wurde
        if user_id in self.user_assignments and flag_name in self.user_assignments[user_id]:
            return self.user_assignments[user_id][flag_name]
            
        # Deterministischer Wert für konsistente Benutzererfahrung
        random_value = self._get_deterministic_value(user_id, flag_name)
        
        # A/B Testing Logik
        if flag.a_b_test:
            is_enabled = random_value < flag.a_percentage
        # Graduelle Einführung Logik
        else:
            is_enabled = random_value < flag.rollout_percentage
            
        # Speichere die Zuweisung
        if user_id not in self.user_assignments:
            self.user_assignments[user_id] = {}
        self.user_assignments[user_id][flag_name] = is_enabled
        
        return is_enabled
    
    def get_flag_statistics(self, flag_name: str) -> Dict[str, Any]:
        """Gibt Statistiken für ein Feature Flag zurück"""
        if flag_name not in self.flags:
            return {}
            
        total_users = len(self.user_assignments)
        enabled_count = sum(1 for user in self.user_assignments.values() 
                          if flag_name in user and user[flag_name])
        
        return {
            "total_users": total_users,
            "enabled_count": enabled_count,
            "enabled_percentage": (enabled_count / total_users * 100) if total_users > 0 else 0
        }
    
    def save_state(self, filename: str) -> None:
        """Speichert den aktuellen Zustand in eine JSON-Datei"""
        state = {
            "flags": {name: vars(flag) for name, flag in self.flags.items()},
            "user_assignments": self.user_assignments
        }
        with open(filename, 'w') as f:
            json.dump(state, f, indent=4)
    
    def load_state(self, filename: str) -> None:
        """Lädt den Zustand aus einer JSON-Datei"""
        with open(filename, 'r') as f:
            state = json.load(f)
            
        self.flags = {
            name: FeatureFlag(**flag_data)
            for name, flag_data in state["flags"].items()
        }
        self.user_assignments = state["user_assignments"]

# Beispiel zur Verwendung
if __name__ == "__main__":
    # System erstellen
    system = FeatureFlagSystem()
    
    # Feature Flags hinzufügen
    system.add_flag("new_ui", enabled=True, rollout_percentage=30)  # Graduelle Einführung
    system.add_flag("payment_system", enabled=True, a_b_test=True, a_percentage=50)  # A/B Test
    
    # Teste verschiedene Benutzer
    test_users = ["user1", "user2", "user3", "user4", "user5"]
    
    print("Graduelle Einführung (new_ui):")
    for user in test_users:
        enabled = system.is_feature_enabled("new_ui", user)
        print(f"User {user}: {'Aktiviert' if enabled else 'Deaktiviert'}")
    
    print("\nA/B Test (payment_system):")
    for user in test_users:
        enabled = system.is_feature_enabled("payment_system", user)
        print(f"User {user}: {'Gruppe A' if enabled else 'Gruppe B'}")
    
    # Statistiken ausgeben
    print("\nStatistiken:")
    print("New UI:", system.get_flag_statistics("new_ui"))
    print("Payment System:", system.get_flag_statistics("payment_system"))
    
    # Zustand speichern
    system.save_state("feature_flags_state.json")