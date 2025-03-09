import spacy
from textblob import TextBlob

# Laden des SpaCy-Modells für NLP
nlp = spacy.load("en_core_web_sm")

def pos_tagging(text):
    """
    Führt Part-of-Speech-Tagging auf dem Text aus.
    """
    doc = nlp(text)
    return [(token.text, token.pos_) for token in doc]

def named_entity_recognition(text):
    """
    Führt Named Entity Recognition (NER) auf dem Text aus.
    """
    doc = nlp(text)
    entities = [(ent.text, ent.label_) for ent in doc.ents]
    return entities

def sentiment_analysis(text):
    """
    Führt Sentiment-Analyse auf dem Text aus.
    """
    blob = TextBlob(text)
    sentiment = blob.sentiment
    return {"Polarity": sentiment.polarity, "Subjectivity": sentiment.subjectivity}

def main():
    """
    Hauptfunktion zur Demonstration des Systems.
    """
    text = input("Bitte geben Sie einen Text ein: ")
    print("\n--- POS-Tagging ---")
    print(pos_tagging(text))
    
    print("\n--- Named Entity Recognition ---")
    print(named_entity_recognition(text))
    
    print("\n--- Sentiment-Analyse ---")
    print(sentiment_analysis(text))

if __name__ == "__main__":
    main()
