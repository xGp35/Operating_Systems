import random

def gen_locality_trace(n, hot_size=4, cold_hi=50, hot_prob=0.9):
    hot_set = list(range(hot_size))
    trace = []
    for _ in range(n):
        if random.random() < hot_prob:
            trace.append(random.choice(hot_set))          # reused hot page
        else:
            trace.append(random.randint(hot_size, cold_hi)) # cold excursion
    return trace

print(",".join(map(str, gen_locality_trace(200))))