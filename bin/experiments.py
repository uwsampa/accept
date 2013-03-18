#!/usr/bin/env python
from __future__ import print_function
from __future__ import division
import os
from contextlib import contextmanager
import imp
import time
import subprocess
import threading
import argh
import shutil
import tempfile
import math
import cw.client
from sqlite3dbm import sshelve
try:
    import cPickle as pickle
except ImportError:
    import pickle

APPS = ['streamcluster', 'blackscholes', 'sobel']
APPSDIR = 'apps'
EVALSCRIPT = 'eval.py'
CONFIGFILE = 'accept_config.txt'
DESCFILE = 'accept_config_desc.txt'
TIMEOUT_FACTOR = 3
MAX_ERROR = 0.3
LOCAL_REPS = 1
CLUSTER_REPS = 5
BASEDIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

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

class Uncertain(object):
    def __init__(self, value, error):
        self.value = value
        self.error = error

    def __unicode__(self):
        return u'{:.2f} +/- {:.2f}'.format(self.value, self.error)

    def __str__(self):
        return str(unicode(self))

    def __repr__(self):
        return 'Uncertain({}, {})'.format(self.value, self.error)

    def __mul__(self, other):
        if isinstance(other, Uncertain):
            value = self.value * other.value
            return Uncertain(
                value,
                abs(value) * math.sqrt(
                    (self.error / self.value) ** 2 +
                    (other.error / other.value) ** 2
                )
            )
        else:
            return Uncertain(self.value * other, abs(self.error * other))

    def __rmul__(self, other):
        return self * other

    def __truediv__(self, other):
        return self * other ** -1.0

    def __pow__(self, other):
        if isinstance(other, Uncertain):
            raise NotImplementedError()
        else:
            value = self.value ** other
            return Uncertain(
                value,
                abs(value) * abs(other) * self.error / abs(self.value)
            )

    def __add__(self, other):
        if isinstance(other, Uncertain):
            return Uncertain(
                self.value + other.value,
                math.sqrt(
                    self.error ** 2 +
                    other.error ** 2
                )
            )
        else:
            return Uncertain(self.value + other, self.error)

    def __sub__(self, other):
        return self + (-other)

    def __neg__(self):
        return self * -1.0

    def __gt__(self, other):
        if isinstance(other, Uncertain):
            return self.value - self.error > other.value + other.error
        else:
            return self.value - self.error > other

    def __lt__(self, other):
        return -self > -other

def umean(nums):
    """Given a list of numbers, return their mean with standard error as
    an Uncertain.
    """
    mean = sum(nums) / len(nums)
    stdev = math.sqrt(sum((x - mean) ** 2 for x in nums) * (1.0 / len(nums)))
    stderr = stdev / math.sqrt(len(nums))
    return Uncertain(mean, stderr)

class CWMemo(object):
    """A proxy for performing function calls that are both memoized and
    parallelized. Its interface is akin to futures: call `submit(func,
    arg, ...)` to request a job and, later, call `get(func, arg, ...)`
    to block until the result is ready. If the call was made previously,
    its result is read back from an on-disk database, and `get` returns
    immediately.

    `dbname` is the filename of the SQLite database to use for
    memoization. `host` is the cluster-workers master hostname address.
    If `force` is true, then no memoized values will be used; every job
    is recomputed and overwrites any previous result.
    """
    def __init__(self, dbname='memo.db', host='localhost', force=False):
        self.dbname = dbname
        self.db = sshelve.open(self.dbname)
        self.completion_thread_db = None
        self.client = cw.client.ClientThread(self.completion, host)
        self.jobs = {}
        self.completion_cond = self.client.jobs_cond
        self.force = force

    def __enter__(self):
        self.client.start()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if not exc_type:
            self.client.wait()
        self.client.stop()
        self.db.close()

    def completion(self, jobid, output):
        # Called in a different thread, so we need a separate SQLite
        # connection.
        if not self.completion_thread_db:
            self.completion_thread_db = sshelve.open(self.dbname)
        db = self.completion_thread_db

        # Get the arguments and save them with the result.
        with self.client.jobs_cond:
            key = self.jobs.pop(jobid)
        db[key] = output

    def _key_for(self, func, args, kwargs):
        # Keyed on function name and arguments. Keyword arguments are
        # ignored.
        return pickle.dumps((func.__module__, func.__name__, args))

    def submit(self, func, *args, **kwargs):
        # Check whether this call is memoized.
        key = self._key_for(func, args, kwargs)
        if key in self.db:
            if self.force:
                del self.db[key]
            else:
                return

        # If not, submit a job to execute it.
        jobid = cw.randid()
        with self.client.jobs_cond:
            self.jobs[jobid] = key
        self.client.submit(jobid, func, *args, **kwargs)

    def get(self, func, *args, **kwargs):
        """Block until the result for a call is ready and return that
        result. (If the value is memoized, this call does not block.)
        """
        key = self._key_for(func, args, kwargs)
        with self.client.jobs_cond:
            while key in self.jobs.values() and \
                  not self.client.remote_exception:
                self.client.jobs_cond.wait()

            if self.client.remote_exception:
                exc = self.client.remote_exception
                self.client.remote_exception = None
                raise exc

            elif key in self.db:
                return self.db[key]

            else:
                raise KeyError(u'no job or result for {} on {}'.format(
                    func, args
                ))

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

def build_and_execute(appname, relax_config, rep, timeout=None):
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
            mod = imp.load_source('evalscript', EVALSCRIPT)
            output = mod.load()

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
    """Represents the result of executing one relaxed configuration of a
    program.
    """
    def __init__(self, app, config, durations, status, output):
        self.app = app
        self.config = config
        self.durations = durations
        self.status = status
        self.output = output

        self.duration = umean(self.durations)

    def evaluate(self, scorefunc, precise_output, precise_durations):
        p_dur = umean(precise_durations)

        if self.status is None:
            # Timed out.
            self.good = False
            self.desc = 'timed out'
            return

        if self.status:
            # Error status.
            self.good = False
            self.desc = 'exited with status {}'.format(self.status)
            return

        # Get output error and speedup.
        self.error = scorefunc(precise_output, self.output)
        self.speedup = p_dur / self.duration

        if self.error > MAX_ERROR:
            # Large output error.
            self.good = False
            self.desc = 'large error: {:.2%}'.format(self.error)
            return

        if self.speedup < 1.0:
            # Slowdown.
            self.good = False
            self.desc = 'slowdown: {} vs. {}'.format(self.duration, p_dur)
            return

        # All good!
        self.good = True
        self.desc = 'good'

def triage_results(results):
    """Given a list of Result objects, splits the data set into three
    lists: optimal, suboptimal, and bad results.
    """
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

def get_results(appname, client, scorefunc, reps):
    """Get a list of Result objects for a given application along with
    its location descriptions. Takes an active CWMemo instance,
    `client`, through which jobs will be submitted and outputs
    collected. `scorefunc` is the application's output quality scoring
    function. `reps` is the number of executions per configuration to
    run.
    """
    # Precise (baseline) execution.
    for rep in range(reps):
        client.submit(build_and_execute,
            appname, None, rep,
            timeout=None
        )

    # Relaxed executions.
    pout, ptime, _, base_config, descs = client.get(build_and_execute,
                                                    appname, None, 0)
    configs = list(permute_config(base_config))
    for config in configs:
        for rep in range(reps):
            client.submit(build_and_execute,
                appname, config, rep,
                timeout=ptime * TIMEOUT_FACTOR
            )

    # Get execution times for the precise version.
    ptimes = []
    for rep in range(reps):
        _, dur, _, _, _ = client.get(build_and_execute,
                                     appname, None, rep)
        ptimes.append(dur)

    # Gather relaxed executions.
    results = []
    for config in configs:
        # Get outputs from first execution.
        aout, _, status, _, _ = client.get(build_and_execute,
                                           appname, config, 0)

        # Get all execution times.
        atimes = []
        for rep in range(reps):
            _, dur, _, _, _ = client.get(build_and_execute,
                                         appname, config, rep)
            atimes.append(dur)

        # Evaluate the result.
        res = Result(appname, config, atimes, status, aout)
        res.evaluate(scorefunc, pout, ptimes)
        results.append(res)

    return results, descs

def evaluate(appname, verbose=False, cluster=False, force=False, reps=1):
    memo_db = os.path.abspath('memo.db')
    master_host = cw.slurm_master_host() if cluster else 'localhost'

    with chdir(os.path.join(APPSDIR, appname)):
        try:
            mod = imp.load_source('evalscript', EVALSCRIPT)
        except IOError:
            assert False, 'no eval.py found in {} directory'.format(appname)

        with CWMemo(dbname=memo_db, host=master_host, force=force) as client:
            results, descs = get_results(appname, client, mod.score, reps)

        optimal, suboptimal, bad = triage_results(results)
        print('{} optimal, {} suboptimal, {} bad'.format(
            len(optimal), len(suboptimal), len(bad)
        ))
        for res in optimal:
            print(dump_config(res.config, descs))
            print('{:.1%} error'.format(res.error))
            print('{} speedup'.format(res.speedup))

        if verbose:
            print('\nsuboptimal configs:')
            for res in suboptimal:
                print(dump_config(res.config, descs))
                print('{:.1%} error'.format(res.error))
                print('{} speedup'.format(res.speedup))

            print('\nbad configs:')
            for res in bad:
                print(res.desc)

@argh.arg('appnames', metavar='NAME', default=APPS, nargs='*', type=unicode,
          help='applications')
@argh.arg('-r', '--reps', metavar='N', default=None, type=int,
          help='replications')
def experiments(appnames, verbose=False, cluster=False, force=False,
                reps=None):
    if not reps:
        reps = CLUSTER_REPS if cluster else LOCAL_REPS
    for appname in appnames:
        print(appname)
        evaluate(appname, verbose, cluster, force, reps)

if __name__ == '__main__':
    argh.dispatch_command(experiments)
