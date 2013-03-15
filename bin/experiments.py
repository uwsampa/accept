#!/usr/bin/env python
from __future__ import print_function
from __future__ import division
import os
from contextlib import contextmanager
import imp
import dumptruck
import functools
import time
import subprocess
import threading
import argh
import shutil
import tempfile

APPS = ['streamcluster', 'blackscholes', 'sobel']
APPSDIR = 'apps'
EVALSCRIPT = 'eval.py'
CONFIGFILE = 'accept_config.txt'
DESCFILE = 'accept_config_desc.txt'
TIMEOUT_FACTOR = 3
MAX_ERROR = 0.3
BASEDIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Work around DumpTruck bugs.
dumptruck.PYTHON_SQLITE_TYPE_MAP[tuple] = u'json text'
class Pickle(object):
    def __init__(self, obj):
        self.obj = obj
dumptruck.Pickle = Pickle
dumptruck.adapters_and_converters.Pickle = Pickle
dumptruck.PYTHON_SQLITE_TYPE_MAP[Pickle] = u'pickle'

@contextmanager
def chdir(d):
    """A context manager that temporary changes the working directory.
    """
    olddir = os.getcwd()
    os.chdir(d)
    yield
    os.chdir(olddir)

@contextmanager
def sandbox():
    """Create a temporary sandbox directory, copy everything from the
    current directory into it, and enter the directory. Afterwards,
    change back and clean up.
    """
    basedir = tempfile.mkdtemp()
    sandbox_dir = os.path.join(basedir, os.path.basename(os.getcwd()))
    shutil.copytree(os.getcwd(), sandbox_dir)
    with chdir(sandbox_dir):
        yield
    shutil.rmtree(basedir)

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
                (Pickle(args),)
            )
            if memoized:
                return memoized[0]['res']

        res = self.func(*args, **kwargs)
        self.dt.insert(
            {'args': Pickle(args), 'res': Pickle(res)},
            self.table
        )
        return res

memoize = functools.partial(Memoized, 'memo.db')

class CommandThread(threading.Thread):
    def __init__(self, command):
        super(CommandThread, self).__init__()
        self.proc = subprocess.Popen(command)

    def run(self):
        self.proc.wait()

def run_cmd(command, timeout=None):
    """Run a process with an optional timeout. Return the process' exit
    status or None if the process timed out.
    """
    thread = CommandThread(command)
    thread.start()
    thread.join(timeout)
    if thread.is_alive():
        thread.proc.terminate()
        return None
    else:
        return thread.proc.returncode

def _make_args():
    return [
        'ENERCDIR={}'.format(BASEDIR),
        'APP_MK={}'.format(os.path.join(BASEDIR, 'apps', 'app.mk')),
    ]

def execute(timeout):
    """Run the application in the working directory and return the
    wall-clock duration (in seconds) of the execution and the exit
    status (or None if the process timed out).
    """
    start_time = time.time()
    status = run_cmd(['make', 'run'] + _make_args(), timeout)
    end_time = time.time()
    return end_time - start_time, status

def build(approx=False):
    """Compile the application in the working directory. If `approx`,
    then it is built with ACCEPT relaxation enabled.
    """
    subprocess.check_call(['make', 'clean'] + _make_args())
    build_cmd = ['make', 'build'] + _make_args()
    if approx:
        build_cmd.append('CLANGARGS=-mllvm -accept-relax')
    subprocess.check_call(build_cmd)

def parse_relax_config(f):
    """Parse a relaxation configuration from a file-like object.
    Generates (module, ident, param) tuples.
    """
    for line in f:
        line = line.strip()
        if line:
            mod, ident, param = line.split(None, 2)
            yield mod, int(ident), int(param)

def dump_relax_config(config, f):
    """Write a relaxation configuration to a file-like object. The
    configuration should be a sequence of tuples.
    """
    for site in config:
        f.write('{} {} {}\n'.format(*site))

def parse_relax_desc(f):
    """Parse a relaxation description map from a file-like object.
    Return a dict mapping (module, ident) pairs to description strings.
    """
    out = {}
    for line in f:
        line = line.strip()
        if line:
            mod, ident, desc = line.split(None, 2)
            out[mod, int(ident)] = desc
    return out

@memoize
def build_and_execute(appname, relax_config, timeout=None, loadfunc=None):
    """Build an application, run it, and collect its output. Return the
    parsed output, the duration of the execution (or None for timeout),
    the exit status, the relaxation configuration, and the relaxation
    descriptions.
    """
    with sandbox():

        if relax_config:
            with open(CONFIGFILE, 'w') as f:
                dump_relax_config(relax_config, f)
        elif os.path.exists(CONFIGFILE):
            os.remove(CONFIGFILE)
        if os.path.exists(DESCFILE):
            os.remove(DESCFILE)

        build(bool(relax_config))
        elapsed, status = execute(timeout)
        if elapsed is None or status:
            # Timeout or error.
            output = None
        else:
            output = loadfunc()

        if not relax_config:
            with open(CONFIGFILE) as f:
                relax_config = list(parse_relax_config(f))
            with open(DESCFILE) as f:
                relax_desc = parse_relax_desc(f)
        else:
            relax_desc = None

    return output, elapsed, status, relax_config, relax_desc

def permute_config(base):
    """Given a base (null) relaxation configuration, generate new
    (relaxed) configurations.
    """
    for i in range(len(base)):
        config = list(base)
        site = config[i]
        site = site[0], site[1], 1
        config[i] = site
        yield config

class Result(object):
    def __init__(self, app, config, duration, status, output):
        self.app = app
        self.config = config
        self.duration = duration
        self.status = status
        self.output = output

    def evaluate(self, scorefunc, precise_output, precise_duration):
        if self.status is None:
            # Timed out.
            self.good = False
            self.desc = 'timed out'
        elif self.status:
            # Error status.
            self.good = False
            self.desc = 'exited with status {}'.format(self.status)
        elif self.duration > precise_duration:
            # Slowdown.
            self.good = False
            self.desc = 'slowdown: {:.2f} vs. {:.2f}'.format(
                self.duration, precise_duration
            )
        else:
            self.speedup = precise_duration / self.duration
            self.error = scorefunc(precise_output, self.output)
            if self.error > MAX_ERROR:
                # Large output error.
                self.good = False
                self.desc = 'large error: {:.2%}'.format(self.error)
            else:
                self.good = True
                self.desc = 'good'

def triage_results(results):
    good = []
    bad = []
    for res in results:
        (good if res.good else bad).append(res)

    optimal = []
    suboptimal = []
    for res in good:
        for other in good:
            if res == good:
                continue
            if other.error < res.error and other.speedup > res.speedup:
                # other dominates res, so eliminate res from the optimal
                # set.
                suboptimal.append(res)
                break
        else:
            # No other result dominates res.
            optimal.append(res)

    assert len(optimal) + len(suboptimal) + len(bad) == len(results)
    return optimal, suboptimal, bad

def dump_config(config, descs):
    """Given a relaxation configuration and an accompanying description
    map, returning a human-readable string describing it.
    """
    optimizations = [r for r in config if r[2]]
    if not optimizations:
        return u'no optimizations'

    out = []
    for mod, ident, param in optimizations:
        out.append(u'{} @ {}'.format(descs[mod, ident], param))
    return u', '.join(out)

def evaluate(appname, verbose=False):
    with chdir(os.path.join(APPSDIR, appname)):
        try:
            mod = imp.load_source('evalscript', EVALSCRIPT)
        except IOError:
            assert False, 'no eval.py found in {} directory'.format(appname)

        # Precise (baseline) execution.
        pout, ptime, _, base_config, descs = build_and_execute(
            appname, None,
            timeout=None, loadfunc=mod.load
        )

        results = []
        for config in permute_config(base_config):
            aout, atime, status, _, _ = build_and_execute(
                appname, config,
                timeout=ptime * TIMEOUT_FACTOR, loadfunc=mod.load
            )
            res = Result(appname, config, atime, status, aout)
            res.evaluate(mod.score, pout, ptime)
            results.append(res)

        optimal, suboptimal, bad = triage_results(results)
        print('{} optimal, {} suboptimal, {} bad'.format(
            len(optimal), len(suboptimal), len(bad)
        ))
        for res in optimal:
            print(dump_config(res.config, descs))
            print('{:.1%} error'.format(res.error))
            print('{:.2f}x speedup'.format(res.speedup))

        if verbose:
            print('\nsuboptimal configs:')
            for res in suboptimal:
                print(dump_config(res.config, descs))
                print('{:.1%} error'.format(res.error))
                print('{:.2f}x speedup'.format(res.speedup))

            print('\nbad configs:')
            for res in bad:
                print(res.desc)

@argh.arg('appnames', metavar='NAME', default=APPS, nargs='*',
          help='applications', type=unicode)
def experiments(appnames, verbose=False):
    for appname in appnames:
        print(appname)
        evaluate(appname, verbose)

if __name__ == '__main__':
    argh.dispatch_command(experiments)
