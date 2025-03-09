import re

def lookbehind(text, pattern):
    match = re.search(pattern, text)
    if match:
        print(match.group())
    else:
        print("Keine Passage in der Text gefunden")

def lookahead(text, pattern):
    match = re.search(pattern, text)
    if match:
        print(match.group())
    else:
        print("Keine Passage in der Text gefunden")

def bedingt(ausdruck, text):
    return r"(.*)" + ausdruck + r"(.*)"

text = "ababa"
pattern = r"(?!abc)(\w)+"
lookbehind(text, pattern)

pattern = r"(?=\d)"  # \d ist eine Zahl, aber auch das 'a' in 'aba'
lookahead(text, pattern)

bedingt_pattern = bedingt(r"(\w){3}")
match = re.search(bedingt_pattern, text)
if match:
    print(match.group())  
else:
    print("Keine Passage in der Text gefunden")
