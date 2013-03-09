#!/usr/bin/env python
import sys
import os
from contextlib import contextmanager
import imp
import dumptruck
import functools
import time
import subprocess

APPS = ['streamcluster']
APPSDIR = 'apps'
EVALSCRIPT = 'eval.py'

dumptruck.PYTHON_SQLITE_TYPE_MAP[tuple] = u'json text'

@contextmanager
def chdir(d):
    """A context manager that temporary changes the working directory.
    """
    olddir = os.getcwd()
    os.chdir(d)
    yield
    os.chdir(olddir)

class Memoized(object):
    """Wrap a function and memoize it in a persistent database. Only
    positional arguments, not keyword arguments, are used as memo table
    keys. These (and the return value) must be serializable.
    """
    def __init__(self, dbname, func):
        self.func = func
        self.dt = dumptruck.DumpTruck(dbname)
        self.table = 'memo_{}'.format(func.__name__)

    def __call__(self, *args, **kwargs):
        if self.table in self.dt.tables():
            memoized = self.dt.execute(
                'SELECT res FROM {} WHERE args=? LIMIT 1'.format(self.table),
                (args,)
            )
            if memoized:
                return memoized[0]['res']

        res = self.func(*args, **kwargs)
        self.dt.insert(
            {'args': args, 'res': res},
            self.table
        )
        return res

memoize = functools.partial(Memoized, 'memo.db')

def execute():
    """Run the application in the working directory and return the
    wall-clock duration (in seconds) of the execution.
    """
    start_time = time.time()
    subprocess.check_call(['make', 'run'])
    end_time = time.time()
    return end_time - start_time

def build(approx=False):
    """Compile the application in the working directory. If `approx`,
    then it is built with ACCEPT relaxation enabled.
    """
    subprocess.check_call(['make', 'clean'])
    build_cmd = ['make', 'build']
    if approx:
        build_cmd.append('CLANGARGS=-mllvm -accept-relax')
    subprocess.check_call(build_cmd)

@memoize
def build_and_execute(appname, relax_config, loadfunc=None):
    """Build an application, run it, and collect its output. Return the
    parsed output and the duration of the execution.
    """
    if relax_config:
        with open('accept_config.txt', 'w') as f:
            f.write(relax_config)
    build(bool(relax_config))
    elapsed = execute()
    output = loadfunc()
    return output, elapsed

def evaluate(appname):
    with chdir(os.path.join(APPSDIR, appname)):
        mod = imp.load_source('evalscript', EVALSCRIPT)
        pout, ptime = build_and_execute(appname, None, loadfunc=mod.load)
        r = '0 0 1\n0 2 0\n0 6 0\n'
        aout, atime = build_and_execute(appname, r, loadfunc=mod.load)
        error = mod.score(pout, aout)
        print('{:.1%} error'.format(error))
        print('{:.2f} vs. {:.2f}: {:.2f}x'.format(ptime, atime, ptime / atime))

def experiments(appnames):
    for appname in appnames:
        evaluate(appname)

if __name__ == '__main__':
    experiments(sys.argv[1:] or APPS)
