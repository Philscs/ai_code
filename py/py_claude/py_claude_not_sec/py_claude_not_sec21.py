import spacy
import nltk
from nltk.sentiment import SentimentIntensityAnalyzer
from typing import List, Dict, Tuple
import logging

class GermanNLPSystem:
    """
    Ein System zur Verarbeitung deutscher Texte mit verschiedenen NLP-Funktionen.
    """
    
    def __init__(self):
        """
        Initialisiert das NLP-System und lädt die notwendigen Modelle.
        """
        # Logging konfigurieren
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # SpaCy-Modell für Deutsch laden
        try:
            self.nlp = spacy.load('de_core_news_lg')
            self.logger.info("SpaCy-Modell erfolgreich geladen")
        except OSError:
            self.logger.error("SpaCy-Modell nicht gefunden. Bitte installieren Sie: python -m spacy download de_core_news_lg")
            raise
        
        # NLTK-Ressourcen laden
        try:
            nltk.download('vader_lexicon', quiet=True)
            self.sia = SentimentIntensityAnalyzer()
            self.logger.info("NLTK-Ressourcen erfolgreich geladen")
        except Exception as e:
            self.logger.error(f"Fehler beim Laden der NLTK-Ressourcen: {str(e)}")
            raise

    def pos_tagging(self, text: str) -> List[Tuple[str, str]]:
        """
        Führt Part-of-Speech-Tagging für den gegebenen Text durch.
        
        Args:
            text (str): Zu analysierender Text
            
        Returns:
            List[Tuple[str, str]]: Liste von (Wort, POS-Tag) Paaren
        """
        doc = self.nlp(text)
        return [(token.text, token.pos_) for token in doc]

    def named_entity_recognition(self, text: str) -> List[Dict[str, str]]:
        """
        Erkennt und klassifiziert Named Entities im Text.
        
        Args:
            text (str): Zu analysierender Text
            
        Returns:
            List[Dict[str, str]]: Liste von erkannten Entities mit Text und Label
        """
        doc = self.nlp(text)
        entities = []
        for ent in doc.ents:
            entities.append({
                'text': ent.text,
                'label': ent.label_,
                'start_char': ent.start_char,
                'end_char': ent.end_char
            })
        return entities

    def sentiment_analysis(self, text: str) -> Dict[str, float]:
        """
        Führt eine Sentiment-Analyse des Textes durch.
        
        Args:
            text (str): Zu analysierender Text
            
        Returns:
            Dict[str, float]: Dictionary mit Sentiment-Scores
        """
        scores = self.sia.polarity_scores(text)
        return {
            'positiv': scores['pos'],
            'negativ': scores['neg'],
            'neutral': scores['neu'],
            'gesamt': scores['compound']
        }

    def analyze_text(self, text: str) -> Dict:
        """
        Führt eine vollständige Analyse des Textes mit allen verfügbaren Funktionen durch.
        
        Args:
            text (str): Zu analysierender Text
            
        Returns:
            Dict: Dictionary mit allen Analyseergebnissen
        """
        return {
            'pos_tags': self.pos_tagging(text),
            'entities': self.named_entity_recognition(text),
            'sentiment': self.sentiment_analysis(text)
        }

    def get_word_statistics(self, text: str) -> Dict:
        """
        Berechnet grundlegende Statistiken über den Text.
        
        Args:
            text (str): Zu analysierender Text
            
        Returns:
            Dict: Dictionary mit Textstatistiken
        """
        doc = self.nlp(text)
        return {
            'anzahl_woerter': len([token for token in doc if not token.is_punct]),
            'anzahl_saetze': len(list(doc.sents)),
            'durchschnittliche_wortlaenge': sum(len(token.text) for token in doc if not token.is_punct) / 
                                          len([token for token in doc if not token.is_punct]) if doc else 0
        }

# Beispielverwendung
if __name__ == "__main__":
    # System initialisieren
    nlp_system = GermanNLPSystem()
    
    # Beispieltext
    text = """
    Die Technische Universität Berlin kündigte gestern ein neues Forschungsprojekt 
    zur künstlichen Intelligenz an. Prof. Dr. Schmidt zeigte sich sehr erfreut über 
    die Finanzierung in Höhe von 2,5 Millionen Euro durch das BMBF.
    """
    
    # Vollständige Analyse durchführen
    ergebnisse = nlp_system.analyze_text(text)
    statistiken = nlp_system.get_word_statistics(text)
    
    # Ergebnisse ausgeben
    print("POS-Tags:", ergebnisse['pos_tags'])
    print("\nErkannte Entities:", ergebnisse['entities'])
    print("\nSentiment-Analyse:", ergebnisse['sentiment'])
    print("\nTextstatistiken:", statistiken)