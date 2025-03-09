import logging
import schedule
import os
import datetime

# Konfiguration für die Log-Datei
LOG_FILE = "system.log"
LOG_DIR = "/var/log"

def init_logger():
    # Erstellen Sie den Log-Folder, wenn es nicht existiert
    if not os.path.exists(LOG_DIR):
        os.makedirs(LOG_DIR)

    # Konfigurieren Sie den Log-Formatter
    log_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')

    # Erstellen Sie einen neuen Logger
    logger = logging.getLogger('system_logger')
    logger.setLevel(logging.INFO)

    # Schreiben Sie die Log-Messages in eine Datei
    file_handler = logging.FileHandler(os.path.join(LOG_DIR, LOG_FILE))
    file_handler.setFormatter(log_formatter)
    logger.addHandler(file_handler)

def create_alarm_function():
    def alarm_function(event):
        # Überprüfen Sie ob der Alarm ausgelöst wurde
        if event['level'] == 'ALARM':
            print(f"Alarm ausgelöst! ({event['timestamp']})")

    return alarm_function

# Erstellen Sie den Alarm-Function
alarm_func = create_alarm_function()

def rotate_log_file():
    # Überprüfen Sie, ob eine Log-Datei existiert und neu erstellen
    if os.path.exists(os.path.join(LOG_DIR, LOG_FILE)):
        os.rename(os.path.join(LOG_DIR, LOG_FILE), os.path.join(LOG_DIR, 
f"system_{datetime.date.today()}.log"))
    else:
        print("Keine Log-Datei gefunden.")

def schedule_rotation():
    # Überprüfen Sie, ob eine Log-Datei existiert
    if os.path.exists(os.path.join(LOG_DIR, LOG_FILE)):
        # Rufen Sie die Log-Daten aus
        with open(os.path.join(LOG_DIR, LOG_FILE), "r") as file:
            log_data = file.read()
        # Überprüfen Sie ob der Alarm ausgelöst wurde
        alarm_level = 'ALARM' in log_data
    else:
        print("Keine Log-Datei gefunden.")

    # Legen Sie die Alarm-Funktion an, wenn ein Alarm ausgegeben wird
    schedule.every(1).minutes.do(alarm_func)

def monitor_log_file():
    while True:
        # Warten Sie für eine Minute
        schedule.run_pending()
        time.sleep(60)

# Initialisieren Sie den Logger
init_logger()

# Schreiben Sie die Log-Funktion in den Scheduling-Scheduler ein
schedule.every().day.at("00:00").do(rotate_log_file)
monitor_log_file()
