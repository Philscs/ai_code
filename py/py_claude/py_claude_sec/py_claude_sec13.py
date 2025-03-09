import logging
import re
import json
from datetime import datetime
from typing import Dict, List, Optional, Pattern
import threading
from collections import defaultdict
from dataclasses import dataclass
from queue import Queue
import html

@dataclass
class LogAggregate:
    count: int
    first_occurrence: datetime
    last_occurrence: datetime
    sample_message: str

class SecureLogHandler(logging.Handler):
    def __init__(
        self,
        destinations: List[logging.Handler],
        sensitive_patterns: Dict[str, Pattern] = None,
        aggregation_interval: int = 300,  # 5 minutes
        max_queue_size: int = 1000
    ):
        super().__init__()
        self.destinations = destinations
        self.sensitive_patterns = sensitive_patterns or {
            'email': re.compile(r'[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}'),
            'credit_card': re.compile(r'\b\d{4}[- ]?\d{4}[- ]?\d{4}[- ]?\d{4}\b'),
            'password': re.compile(r'password[\s]*[=:]\s*\S+', re.IGNORECASE),
            'api_key': re.compile(r'api[_-]?key[\s]*[=:]\s*\S+', re.IGNORECASE)
        }
        
        self.aggregation_interval = aggregation_interval
        self.aggregates = defaultdict(
            lambda: LogAggregate(0, datetime.now(), datetime.now(), "")
        )
        self.aggregate_lock = threading.Lock()
        
        # Queue für asynchrone Verarbeitung
        self.log_queue = Queue(maxsize=max_queue_size)
        
        # Start worker thread
        self.worker = threading.Thread(target=self._process_queue, daemon=True)
        self.worker.start()
        
        # Periodisches Aggregat-Flushing
        self.flush_thread = threading.Thread(target=self._flush_aggregates, daemon=True)
        self.flush_thread.start()

    def _sanitize_log_message(self, message: str) -> str:
        """
        Sanitize log message by escaping HTML/XML and masking sensitive data
        """
        # HTML escaping gegen Log-Injection
        message = html.escape(message)
        
        # Sensitive Daten maskieren
        for pattern_name, pattern in self.sensitive_patterns.items():
            message = pattern.sub(f"[MASKED_{pattern_name.upper()}]", message)
            
        return message

    def _should_aggregate(self, record: logging.LogRecord) -> bool:
        """
        Determine if the log message should be aggregated based on criteria
        """
        # Aggregate ERROR und WARNING level messages
        return (
            record.levelno >= logging.WARNING and
            not record.exc_info  # Keine Exceptions aggregieren
        )

    def _get_aggregate_key(self, record: logging.LogRecord) -> str:
        """
        Generate a key for aggregating similar log messages
        """
        # Kombiniere Level und einen vereinfachten Message-Hash
        simplified_msg = re.sub(r'\b\d+\b', '#', record.getMessage())
        return f"{record.levelno}:{simplified_msg}"

    def emit(self, record: logging.LogRecord):
        """
        Put the log record into the processing queue
        """
        try:
            self.log_queue.put(record)
        except Exception:
            self.handleError(record)

    def _process_queue(self):
        """
        Process log records from the queue
        """
        while True:
            record = self.log_queue.get()
            try:
                self._process_record(record)
            except Exception:
                self.handleError(record)
            finally:
                self.log_queue.task_done()

    def _process_record(self, record: logging.LogRecord):
        """
        Process a single log record
        """
        # Sanitize the message
        record.msg = self._sanitize_log_message(str(record.msg))
        
        if self._should_aggregate(record):
            self._aggregate_record(record)
        else:
            self._forward_record(record)

    def _aggregate_record(self, record: logging.LogRecord):
        """
        Aggregate similar log messages
        """
        key = self._get_aggregate_key(record)
        
        with self.aggregate_lock:
            agg = self.aggregates[key]
            agg.count += 1
            agg.last_occurrence = datetime.now()
            if agg.count == 1:
                agg.first_occurrence = datetime.now()
                agg.sample_message = record.getMessage()

    def _forward_record(self, record: logging.LogRecord):
        """
        Forward the log record to all configured destinations
        """
        for dest in self.destinations:
            try:
                dest.handle(record)
            except Exception as e:
                # Log Fehler beim Forwarding, aber verhindere Rekursion
                if not getattr(record, 'forwarding_error', False):
                    error_record = logging.LogRecord(
                        name='SecureLogHandler',
                        level=logging.ERROR,
                        pathname=__file__,
                        lineno=0,
                        msg=f"Error forwarding log: {str(e)}",
                        args=(),
                        exc_info=None
                    )
                    error_record.forwarding_error = True
                    self._forward_record(error_record)

    def _flush_aggregates(self):
        """
        Periodically flush aggregated logs
        """
        while True:
            threading.Event().wait(self.aggregation_interval)
            
            with self.aggregate_lock:
                current_time = datetime.now()
                
                for key, agg in list(self.aggregates.items()):
                    # Flush wenn Intervall abgelaufen
                    if (current_time - agg.last_occurrence).total_seconds() >= self.aggregation_interval:
                        if agg.count > 1:
                            summary_record = logging.LogRecord(
                                name='LogAggregator',
                                level=logging.INFO,
                                pathname=__file__,
                                lineno=0,
                                msg=(
                                    f"Aggregated {agg.count} similar logs from "
                                    f"{agg.first_occurrence} to {agg.last_occurrence}. "
                                    f"Sample: {agg.sample_message}"
                                ),
                                args=(),
                                exc_info=None
                            )
                            self._forward_record(summary_record)
                        
                        del self.aggregates[key]

# Beispiel Verwendung:
if __name__ == "__main__":
    # Handler für verschiedene Ziele einrichten
    file_handler = logging.FileHandler("app.log")
    console_handler = logging.StreamHandler()
    
    # Secure Log Handler mit mehreren Zielen
    secure_handler = SecureLogHandler(
        destinations=[file_handler, console_handler],
        aggregation_interval=60  # 1 Minute für Demo
    )
    
    # Logger konfigurieren
    logger = logging.getLogger("app")
    logger.setLevel(logging.INFO)
    logger.addHandler(secure_handler)
    
    # Beispiel Logs
    logger.info("Application started")
    logger.warning("Database connection slow - response time: 2.5s")
    logger.error("Failed login attempt with password: secretpass123")  # wird maskiert
    logger.error("Invalid API key: ak_123456789")  # wird maskiert
    logger.warning("Database connection slow - response time: 3.1s")  # wird aggregiert