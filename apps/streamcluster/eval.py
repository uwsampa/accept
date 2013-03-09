import subprocess
import time
import dumptruck
import functools

dumptruck.PYTHON_SQLITE_TYPE_MAP[tuple] = u'json text'

class Memoized(object):
    def __init__(self, dbname, func):
        self.func = func
        self.dt = dumptruck.DumpTruck(dbname)
        self.table = 'memo_{}'.format(func.__name__)

    def __call__(self, *args, **kwargs):
        if self.table in self.dt.tables():
            memoized = self.dt.execute(
                'SELECT res FROM {} WHERE args=? AND kwargs=? '
                'LIMIT 1'.format(self.table),
                (args, kwargs)
            )
            if memoized:
                return memoized[0]['res']

        res = self.func(*args, **kwargs)
        self.dt.insert(
            {'args': args, 'kwargs': kwargs, 'res': res},
            self.table
        )
        return res

memoize = functools.partial(Memoized, 'memo.db')

def load(fn):
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

def execute():
    start_time = time.time()
    subprocess.check_call(['make', 'run'])
    end_time = time.time()
    return load('output.txt'), end_time - start_time

def build(approx=False):
    if approx:
        with open('accept_config.txt', 'w') as f:
            f.write('0 0 1\n0 2 0\n0 6 0\n')

    subprocess.check_call(['make', 'clean'])
    build_cmd = ['make', 'build']
    if approx:
        build_cmd.append('CLANGARGS=-mllvm -accept-relax')
    subprocess.check_call(build_cmd)

@memoize
def build_and_execute(approx=False):
    build(approx)
    return execute()

def main():
    pout, ptime = build_and_execute(False)
    aout, atime = build_and_execute(True)
    score(pout, aout)
    print('{:.2f} vs. {:.2f}: {:.2f}x'.format(ptime, atime, ptime / atime))

if __name__ == '__main__':
    main()
