import random

def gen_recency_trace(n, hot_size=4, cold_hi=50, hot_prob=0.9, repeat_prob=0.6):
    hot_set = list(range(hot_size))
    trace = []
    last = random.choice(hot_set)
    for _ in range(n):
        if random.random() < hot_prob:
            if random.random() < repeat_prob:
                page = last          # burst: repeat the last page
            else:
                page = random.choice(hot_set)
        else:
            page = random.randint(hot_size, cold_hi)
        trace.append(page)
        last = page
    return trace

print(",".join(map(str, gen_recency_trace(200, hot_size=7, hot_prob=0.7))))