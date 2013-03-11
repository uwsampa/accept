import munkres

def load(fn='output.txt'):
    centers = []

    with open(fn) as f:
        while True:
            ident = f.readline()
            if not ident:
                break
            weight = f.readline()
            coords = f.readline()
            f.readline()

            centers.append((int(ident), float(weight),
                            map(float, coords.split())))

    return centers

def euclidean_dist(a, b):
    assert len(a) == len(b)
    total = 0.0
    for x, y in zip(a, b):
        total += (x - y) ** 2.0
    return total ** 0.5

def score(orig, relaxed):
    matrix = []
    for _, _, a in orig:
        row = []
        for _, _, b in relaxed:
            row.append(euclidean_dist(a, b))
        matrix.append(row)

    total = 0.0
    indices = munkres.Munkres().compute(matrix)
    for i, j in indices:
        a = orig[i][2]
        b = relaxed[j][2]
        dist = euclidean_dist(a, b)
        dist /= len(a) ** 0.5
        total += dist
    return total / len(orig)
