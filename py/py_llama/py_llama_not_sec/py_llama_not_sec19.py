import pandas as pd
import numpy as np
from sklearn.model_selection import GridSearchCV
from sklearn.linear_model import LinearRegression, Ridge
from sklearn.ensemble import RandomForestRegressor
from sklearn.ensemble import IsolationForest
import matplotlib.pyplot as plt

# Daten laden
df = pd.read_csv('data.csv')

# Vorhersagemodelle initialisieren
models = {
    "Linear Regression": LinearRegression(),
    "Ridge Regression": Ridge(),
    "Random Forest Regressor": RandomForestRegressor()
}

# Trendanalyse durchführen
def calculate_trend(df):
    trend = np.polyfit(df.index, df.values, 1)
    return trend

trends = {}
for model_name, model in models.items():
    trend = calculate_trend(df)
    trends[model_name] = trend

# Anomalieerkennung durchführen
def detect_anomalies(df, model):
    iso = IsolationForest(model_order=0)
    iso.fit(df.values.reshape(-1, 1))
    anomalies = iso.predict(df.values.reshape(-1, 1))
    return anomalies

anomalies = {}
for model_name, model in models.items():
    iso = IsolationForest(model_order=0)
    iso.fit(df.values.reshape(-1, 1))
    anomalies[model_name] = detect_anomalies(df, iso)

# Vorhersagemodelle trainieren
grid_search = GridSearchCV(models["Random Forest Regressor"], {"n_estimators": [100, 200, 300]}, 
cv=5)
grid_search.fit(df.values.reshape(-1, 1), df.values)

# Ergebnisse visualisieren
plt.figure(figsize=(10,6))

# Trendvisualisierung
for model_name, trend in trends.items():
    plt.plot(df.index, df.values, label=model_name)
    plt.plot(df.index, np.polyval(trend, df.index), 'r--', label=f"Trend: {trend}")

# Anomalievisualisierung
plt.scatter(df.index[anomalies["Random Forest Regressor"] == -1], df.values[anomalies["Random Forest Regressor"] == -1], color='red')
plt.scatter(df.index[anomalies["Random Forest Regressor"] == 1], df.values[anomalies["Random Forest Regressor"] == 1], color='green')

# Vorhersagemodellevisualisierung
plt.plot(grid_search.best_estimator_.predict(df.values.reshape(-1, 1)), color='blue', 
label="Vorhergesagt")

plt.legend()
plt.show()