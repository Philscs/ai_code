from pyspark.sql import SparkSession
from pyspark.sql.functions import window, col, count, avg
from pyspark.sql.types import StructType, StructField, StringType, TimestampType

def start_streaming():
    # SparkSession erstellen
    spark = SparkSession.builder \
        .appName("Streaming mit Windowing") \
        .getOrCreate()

    spark.sparkContext.setLogLevel("WARN")

    # Beispiel-Schema für eingehende Streaming-Daten
    schema = StructType([
        StructField("event", StringType(), True),
        StructField("timestamp", TimestampType(), True)
    ])

    # Streaming-Datenquelle simulieren (z. B. Socket oder Kafka)
    stream_data = spark.readStream \
        .format("csv") \
        .schema(schema) \
        .option("header", "true") \
        .load("/path/to/streaming/data")

    # Beispiel 1: Tumbling Window (zeitlich festgelegt)
    tumbling_window = stream_data \
        .groupBy(window(col("timestamp"), "10 minutes")) \
        .agg(count("event").alias("event_count"))

    # Beispiel 2: Sliding Window (zeitliche Überlappung)
    sliding_window = stream_data \
        .groupBy(window(col("timestamp"), "10 minutes", "5 minutes")) \
        .agg(count("event").alias("event_count"))

    # Beispiel 3: Aggregation mit Durchschnittswert
    avg_window = stream_data \
        .groupBy(window(col("timestamp"), "15 minutes")) \
        .agg(avg(col("event").cast("int")).alias("average_value"))

    # Ausgabefunktion - Ergebnisse auf die Konsole schreiben
    tumbling_query = tumbling_window.writeStream \
        .outputMode("complete") \
        .format("console") \
        .start()

    sliding_query = sliding_window.writeStream \
        .outputMode("complete") \
        .format("console") \
        .start()

    avg_query = avg_window.writeStream \
        .outputMode("complete") \
        .format("console") \
        .start()

    # Streams laufen lassen
    tumbling_query.awaitTermination()
    sliding_query.awaitTermination()
    avg_query.awaitTermination()

if __name__ == "__main__":
    start_streaming()
