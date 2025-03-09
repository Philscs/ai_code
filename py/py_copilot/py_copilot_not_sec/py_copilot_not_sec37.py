import cProfile
from pstats import Stats

def measure_performance(func):
    def wrapper(*args, **kwargs):
        profiler = cProfile.Profile()
        profiler.enable()
        result = func(*args, **kwargs)
        profiler.disable()

        stats = Stats(profiler)
        stats.sort_stats('cumulative')
        stats.print_stats()

        return result

    return wrapper

@measure_performance
def example_function(n):
    result = 0
    for i in range(n):
        if i % 100 == 0:
            pass
        else:
            result += i

example_function(100000)
