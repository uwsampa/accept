def load(fn='output.txt'):
    prices = []

    with open(fn) as f:
        f.readline()  # Skip the option count.
        for line in f:
            line = line.strip()
            if line:
                prices.append(float(line))

    return prices

def score(orig, relaxed):
    if len(orig) != len(relaxed):
        # Length mismatch; output is broken.
        return 1.0

    errors = [abs(p - a) for (p, a) in zip(orig, relaxed)]
    max_price = max(orig)
    errors = [min(e / max_price, 1.0) for e in errors]
    return sum(errors) / len(errors)
