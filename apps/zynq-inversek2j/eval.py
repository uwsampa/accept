def load():
    return 'file:output.txt'


def _load(fn):
    pairs = []

    with open(fn) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            parts = line.split()
            if len(parts) == 2:
                try:
                    parts = map(float, parts)
                except ValueError:
                    continue

                pairs.append(parts)

    return pairs


def euclidean_dist(a, b):
    total = 0.0
    for x, y in zip(a, b):
        total += (x - y) ** 2.0
    return total ** 0.5


def score(orig_fn, relaxed_fn):
    orig = _load(orig_fn)
    relaxed = _load(relaxed_fn)

    # Check that lengths match.
    if len(orig) != len(relaxed):
        return 1.0

    dists = [euclidean_dist(a, b) for a, b in zip(orig, relaxed)]
    dists = [min(d, 1.0) for d in dists]
    return sum(dists) / len(dists)
