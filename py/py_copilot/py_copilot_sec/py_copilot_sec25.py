class TypeChecker:
    def __init__(self):
        self.constraints = {}

    def add_constraint(self, type_name, constraint_func):
        self.constraints[type_name] = constraint_func

    def check_type(self, value, type_name):
        if type_name in self.constraints:
            constraint_func = self.constraints[type_name]
            if not constraint_func(value):
                raise TypeError(f"Value {value} does not satisfy the constraint for type {type_name}")

        if not isinstance(value, type_name):
            raise TypeError(f"Value {value} is not of type {type_name}")

        return value

    def type_check(self, func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            arg_types = get_type_hints(func)
            for arg_name, arg_value in zip(func.__code__.co_varnames, args):
                if arg_name in arg_types:
                    arg_type = arg_types[arg_name]
                    if not isinstance(arg_value, arg_type):
                        raise TypeError(f"Value {arg_value} is not of type {arg_type} for argument {arg_name}")
            return func(*args, **kwargs)
        return wrapper

if __name__ == "__main__":
    type_checker = TypeChecker()
    # Add your constraints and type checks here

    # Example usage
    value = 10
    type_name = int
    type_checker.check_type(value, type_name)
    print(f"Value {value} is of type {type_name}")

    # Example usage of type_check decorator
    @type_checker.type_check
    def add(a: int, b: int) -> int:
        return a + b

    print(add(2, 3))  # Output: 5

    try:
        print(add('a', 'b'))  # Exception
    except TypeError as e:
        print(e)

    @type_checker.type_check
    def greet(name: str) -> None:
        print(f"Hello, {name}!")

    greet('Alice')  # Output: Hello, Alice!

    try:
        greet(123)  # Exception
    except TypeError as e:
        print(e)
