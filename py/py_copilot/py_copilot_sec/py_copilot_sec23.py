import time
import time

def levenshtein_distance(s1, s2):
    m, n = len(s1), len(s2)
    dp = [[0] * (n + 1) for _ in range(m + 1)]

    for i in range(m + 1):
        dp[i][0] = i

    for j in range(n + 1):
        dp[0][j] = j

    for i in range(1, m + 1):
        for j in range(1, n + 1):
            if s1[i - 1] == s2[j - 1]:
                dp[i][j] = dp[i - 1][j - 1]
            else:
                dp[i][j] = min(dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]) + 1

    return dp[m][n]


def fuzzy_search(query, texts, threshold, timeout):
    results = []
    start_time = time.time()

    for text in texts:
        distance = levenshtein_distance(query, text)
        similarity = 1 - (distance / max(len(query), len(text)))
        if similarity >= threshold:
            results.append((text, similarity))

        elapsed_time = time.time() - start_time
        if elapsed_time >= timeout:
            break

    return results

if __name__ == "__main__":
    query = "example"
    texts = ["example1", "example2", "example3"]
    threshold = 0.8
    timeout = 5

    results = fuzzy_search(query, texts, threshold, timeout)

    for result in results:
        print(f"Text: {result[0]}, Similarity: {result[1]}")
