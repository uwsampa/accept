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

def score(orig, relaxed):
    for c, d in zip(orig, relaxed):
        print c[0], c[1]
        print d[0], d[1]
