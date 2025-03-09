import nltk
from nltk.tokenize import word_tokenize
from nltk.corpus import stopwords
from nltk.stem import PorterStemmer
import spacy
from spaCy import displacy
from vaderSentiment.vaderSentiment import SentimentIntensityAnalyzer

nlp = spacy.load("de_core_news_sm")

# Funktion für die POS-Tagging
def pos_tagging(text):
    doc = nlp(text)
    tags = [(token.text, token.pos_) for token in doc]
    return tags

# Funktion für das Named Entity Recognition
def ner(text):
    doc = nlp(text)
    ents = [(entity.text, entity.label_) for entity in doc.ents]
    return ents

# Funktion für die Sentiment-Analyse
def sentiment_analyse(text):
    sia = SentimentIntensityAnalyzer()
    sentiment = sia.polarity_scores(text)
    return sentiment

# Testfunktion
def test_nlp():
    text = "Das ist ein Beispiel für eine natürliche Sprachverarbeitung."
    print("POS-Tagging:")
    tags = pos_tagging(text)
    for token, tag in tags:
        print(f"{token}: {tag}")

    print("\nNamed Entity Recognition:")
    ents = ner(text)
    for entity, label in ents:
        print(f"{entity}: {label}")

    print("\nSentiment-Analyse:")
    sentiment = sentiment_analyse(text)
    print(f"Polarity: {sentiment['compound']}")
    print(f"Positives: {sentiment['pos']}")
    print(f"Negativen: {sentiment['neg']}")
    print(f"Beweislast: {sentiment['neu']}")

test_nlp()