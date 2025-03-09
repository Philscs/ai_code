from datetime import datetime, timedelta
from collections import defaultdict

class StreamingDataProcessor:
    def __init__(self):
        self.data = defaultdict(list)

    def process_data(self, timestamp, value):
        self.data[timestamp].append(value)

    def process_window(self, window_size, aggregation_func):
        current_time = datetime.now()
        start_time = current_time - timedelta(seconds=window_size)
        aggregated_data = []

        for timestamp, values in self.data.items():
            if timestamp >= start_time:
                aggregated_value = aggregation_func(values)
                aggregated_data.append((timestamp, aggregated_value))

        return aggregated_data

# Example usage
processor = StreamingDataProcessor()

# Process some data
processor.process_data(datetime.now(), 10)
processor.process_data(datetime.now(), 20)
processor.process_data(datetime.now(), 30)

# Process a window with sum aggregation
window_size = 60  # in seconds
aggregation_func = sum
aggregated_data = processor.process_window(window_size, aggregation_func)

print(aggregated_data)
