import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

class CSVDataAnalysis:
    def __init__(self, file_path):
        """
        Initialize the CSVDataAnalysis class.
        :param file_path: Path to the CSV file.
        """
        self.data = pd.read_csv(file_path)

    def filter_data(self, filters):
        """
        Filter the data based on the provided conditions.
        :param filters: A dictionary where keys are column names and values are filter conditions.
        :return: Filtered DataFrame.
        """
        filtered_data = self.data.copy()
        for column, condition in filters.items():
            filtered_data = filtered_data.query(f"{column} {condition}")
        return filtered_data

    def group_and_aggregate(self, group_by_columns, aggregations):
        """
        Group data by specified columns and apply aggregations.
        :param group_by_columns: List of columns to group by.
        :param aggregations: Dictionary of column:aggregation_function.
        :return: Grouped and aggregated DataFrame.
        """
        return self.data.groupby(group_by_columns).agg(aggregations).reset_index()

    def calculate_statistics(self):
        """
        Calculate basic statistics of the dataset.
        :return: DataFrame containing descriptive statistics.
        """
        return self.data.describe()

    def visualize_data(self, x=None, y=None, kind="scatter", hue=None):
        """
        Visualize data using various plot types.
        :param x: Column for x-axis.
        :param y: Column for y-axis.
        :param kind: Type of plot (scatter, line, bar, box, hist, etc.).
        :param hue: Column to use for color encoding (optional).
        """
        if kind == "scatter":
            sns.scatterplot(data=self.data, x=x, y=y, hue=hue)
        elif kind == "line":
            sns.lineplot(data=self.data, x=x, y=y, hue=hue)
        elif kind == "bar":
            sns.barplot(data=self.data, x=x, y=y, hue=hue)
        elif kind == "box":
            sns.boxplot(data=self.data, x=x, y=y, hue=hue)
        elif kind == "hist":
            sns.histplot(data=self.data, x=x, hue=hue, kde=True)
        else:
            raise ValueError("Unsupported plot type. Supported types: scatter, line, bar, box, hist.")

        plt.show()

    def apply_custom_function(self, func):
        """
        Apply a custom function to the dataset.
        :param func: Function to apply. The function should accept and return a DataFrame.
        :return: DataFrame after applying the function.
        """
        return func(self.data)

# Example usage:
if __name__ == "__main__":
    # Initialize the tool with a CSV file
    analysis_tool = CSVDataAnalysis("data.csv")

    # Example: Filter data
    filters = {"Age": "> 30", "Salary": "<= 50000"}
    filtered_data = analysis_tool.filter_data(filters)
    print(filtered_data)

    # Example: Group and aggregate
    grouped_data = analysis_tool.group_and_aggregate(["Department"], {"Salary": "mean", "Age": "max"})
    print(grouped_data)

    # Example: Calculate statistics
    stats = analysis_tool.calculate_statistics()
    print(stats)

    # Example: Visualize data
    analysis_tool.visualize_data(x="Age", y="Salary", kind="scatter")

    # Example: Apply custom function
    def custom_analysis(df):
        df["Salary_Per_Age"] = df["Salary"] / df["Age"]
        return df

    custom_data = analysis_tool.apply_custom_function(custom_analysis)
    print(custom_data)
