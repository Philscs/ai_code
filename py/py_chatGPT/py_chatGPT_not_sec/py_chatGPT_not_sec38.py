import hashlib
import random

class FeatureFlagManager:
    def __init__(self):
        self.flags = {}

    def create_flag(self, name, description, default=False, rollout=0):
        """
        Create a new feature flag.

        :param name: Name of the flag
        :param description: Description of the flag
        :param default: Default value of the flag (True/False)
        :param rollout: Rollout percentage (0-100) for gradual rollout
        """
        self.flags[name] = {
            "description": description,
            "default": default,
            "rollout": rollout,
            "ab_testing": False,
            "variants": {}
        }

    def enable_ab_testing(self, name, variants):
        """
        Enable A/B testing for a feature flag.

        :param name: Name of the flag
        :param variants: Dictionary of variant names and their rollout percentages
        """
        if name not in self.flags:
            raise ValueError(f"Feature flag '{name}' does not exist.")

        if sum(variants.values()) != 100:
            raise ValueError("Rollout percentages for variants must sum to 100.")

        self.flags[name]["ab_testing"] = True
        self.flags[name]["variants"] = variants

    def is_feature_enabled(self, name, user_id):
        """
        Check if a feature is enabled for a specific user.

        :param name: Name of the flag
        :param user_id: Unique identifier for the user
        :return: Boolean or variant name
        """
        if name not in self.flags:
            raise ValueError(f"Feature flag '{name}' does not exist.")

        flag = self.flags[name]

        if flag["ab_testing"]:
            return self._get_ab_variant(name, user_id)

        if flag["rollout"] == 100:
            return True

        rollout_threshold = self._get_hash_percentage(user_id, name)
        return rollout_threshold < flag["rollout"]

    def _get_ab_variant(self, name, user_id):
        """
        Determine which A/B variant a user falls into.

        :param name: Name of the flag
        :param user_id: Unique identifier for the user
        :return: Variant name
        """
        flag = self.flags[name]
        hash_value = self._get_hash_percentage(user_id, name)

        cumulative = 0
        for variant, percentage in flag["variants"].items():
            cumulative += percentage
            if hash_value < cumulative:
                return variant

    def _get_hash_percentage(self, user_id, name):
        """
        Generate a deterministic percentage based on user_id and feature name.

        :param user_id: Unique identifier for the user
        :param name: Name of the flag
        :return: Percentage (0-100)
        """
        hash_input = f"{user_id}-{name}".encode("utf-8")
        hash_value = hashlib.md5(hash_input).hexdigest()
        return int(hash_value, 16) % 100

    def update_rollout(self, name, percentage):
        """
        Update the rollout percentage for a feature flag.

        :param name: Name of the flag
        :param percentage: New rollout percentage (0-100)
        """
        if name not in self.flags:
            raise ValueError(f"Feature flag '{name}' does not exist.")

        if not (0 <= percentage <= 100):
            raise ValueError("Rollout percentage must be between 0 and 100.")

        self.flags[name]["rollout"] = percentage

# Beispiel für die Verwendung
if __name__ == "__main__":
    ff_manager = FeatureFlagManager()

    # Feature-Flag erstellen
    ff_manager.create_flag("new_dashboard", "Testet ein neues Dashboard.", default=False, rollout=50)

    # A/B-Test aktivieren
    ff_manager.enable_ab_testing("new_dashboard", {"variant_a": 50, "variant_b": 50})

    # Benutzer*innen prüfen
    user_ids = ["user1", "user2", "user3", "user4"]
    for user_id in user_ids:
        enabled = ff_manager.is_feature_enabled("new_dashboard", user_id)
        print(f"Benutzer*in {user_id}: {enabled}")
