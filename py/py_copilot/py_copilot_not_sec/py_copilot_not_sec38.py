import random

class FeatureFlagManager:
    def __init__(self, feature_name, rollout_percentage):
        self.feature_name = feature_name
        self.rollout_percentage = rollout_percentage

    def is_enabled(self, user_id):
        if self.rollout_percentage == 100:
            return True
        elif self.rollout_percentage == 0:
            return False
        else:
            random.seed(user_id)
            random_percentage = random.randint(1, 100)
            return random_percentage <= self.rollout_percentage

# Example usage
flag_manager = FeatureFlagManager("my_feature", 50)
user_id = "user123"
if flag_manager.is_enabled(user_id):
    print("Feature is enabled for user", user_id)
else:
    print("Feature is disabled for user", user_id)
