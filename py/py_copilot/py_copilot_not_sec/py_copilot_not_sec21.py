import nltk
from nltk.tokenize import word_tokenize
from nltk.tag import pos_tag
from nltk.chunk import ne_chunk
from nltk.sentiment import SentimentIntensityAnalyzer

def pos_tagging(text):
    tokens = word_tokenize(text)
    tagged = pos_tag(tokens)
    return tagged

def named_entity_recognition(text):
    tokens = word_tokenize(text)
    tagged = pos_tag(tokens)
    entities = ne_chunk(tagged)
    return entities

def sentiment_analysis(text):
    sid = SentimentIntensityAnalyzer()
    sentiment_scores = sid.polarity_scores(text)
    return sentiment_scores

# Example usage
text = "I love using GitHub Copilot. It's an amazing tool!"
print(pos_tagging(text))
print(named_entity_recognition(text))
print(sentiment_analysis(text))
