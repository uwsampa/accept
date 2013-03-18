def load():
    return 'file:output.txt'

def _read(fn):
    prices = []

    with open(fn) as f:
        f.readline()  # Skip the option count.
        for line in f:
            line = line.strip()
            if line:
                prices.append(float(line))

    return prices

def score(orig, relaxed):
    orig_prices = _read(orig)
    relaxed_prices = _read(relaxed)

    if len(orig_prices) != len(relaxed_prices):
        # Length mismatch; output is broken.
        return 1.0

    errors = [abs(p - a) for (p, a) in zip(orig_prices, relaxed_prices)]
    max_price = max(orig_prices)
    errors = [min(e / max_price, 1.0) for e in errors]
    return sum(errors) / len(errors)
