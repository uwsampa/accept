from __future__ import print_function
from __future__ import division
import os
from contextlib import contextmanager
import imp
import time
import subprocess
import threading
import shutil
import tempfile
from sqlite3dbm import sshelve
try:
    import cPickle as pickle
except ImportError:
    import pickle
import logging
import string
import random
try:
    import cw.client
except ImportError:
    pass


EVALSCRIPT = 'eval.py'
CONFIGFILE = 'accept_config.txt'
DESCFILE = 'accept_config_desc.txt'
BASEDIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUTS_DIR = os.path.join(BASEDIR, 'saved_outputs')


@contextmanager
def chdir(d):
    """A context manager that temporary changes the working directory.
    """
    olddir = os.getcwd()
    os.chdir(d)
    yield
    os.chdir(olddir)


@contextmanager
def sandbox(symlink=False):
    """Create a temporary sandbox directory, copy (or symlink)
    everything from the current directory into it, and enter the
    directory. Afterwards, change back and clean up.
    """
    sandbox_dir = tempfile.mkdtemp()

    for name in os.listdir(os.getcwd()):
        src = os.path.join(os.getcwd(), name)
        dst = os.path.join(sandbox_dir, name)
        if symlink:
            os.symlink(src, dst)
        else:
            if os.path.isdir(src):
                shutil.copytree(src, dst)
            else:
                shutil.copyfile(src, dst)

    with chdir(sandbox_dir):
        yield

    shutil.rmtree(sandbox_dir)


def _random_string(length=20, chars=(string.ascii_letters + string.digits)):
    return ''.join(random.choice(chars) for i in range(length))


class CWMemo(object):
    """A proxy for performing function calls that are both memoized and
    parallelized. Its interface is akin to futures: call `submit(func,
    arg, ...)` to request a job and, later, call `get(func, arg, ...)`
    to block until the result is ready. If the call was made previously,
    its result is read back from an on-disk database, and `get` returns
    immediately.

    `dbname` is the filename of the SQLite database to use for
    memoization. `host` is the cluster-workers master hostname address
    or None to run everything locally and eagerly on the main thread
    (for slow debugging of small runs). If `force` is true, then no
    memoized values will be used; every job is recomputed and overwrites
    any previous result.
    """
    def __init__(self, dbname='memo.db', host=None, force=False):
        self.dbname = dbname
        self.force = force
        self.logger = logging.getLogger('cwmemo')
        self.logger.setLevel(logging.INFO)
        self.host = host
        self.local = host is None

        if not self.local:
            self.completion_thread_db = None
            self.client = cw.client.ClientThread(self.completion, host)
            self.jobs = {}
            self.completion_cond = self.client.jobs_cond

    def __enter__(self):
        if not self.local:
            self.client.start()
        self.db = sshelve.open(self.dbname)
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if not self.local:
            if not exc_type:
                self.client.wait()
            self.client.stop()
        self.db.close()

    def completion(self, jobid, output):
        """Callback invoked when a cluster-workers job finishes. We
        store the result for a later get() call and wake up any pending
        get()s.
        """
        self.logger.info(u'job completed ({} pending)'.format(
            len(self.jobs) - 1
        ))

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
        """Start a job (if it is not already memoized).
        """
        # Check whether this call is memoized.
        key = self._key_for(func, args, kwargs)
        if key in self.db:
            if self.force:
                del self.db[key]
            else:
                return

        # If executing locally, just run the function.
        if self.local:
            output = func(*args, **kwargs)
            self.db[key] = output
            return

        # Otherwise, submit a job to execute it.
        jobid = cw.randid()
        with self.client.jobs_cond:
            self.jobs[jobid] = key
        self.logger.info(u'submitting {}({})'.format(
            func.__name__, u', '.join(repr(a) for a in args)
        ))
        self.client.submit(jobid, func, *args, **kwargs)

    def get(self, func, *args, **kwargs):
        """Block until the result for a call is ready and return that
        result. (If the value is memoized, this call does not block.)
        """
        key = self._key_for(func, args, kwargs)

        if self.local:
            if key in self.db:
                return self.db[key]
            else:
                raise KeyError(u'no result for {} on {}'.format(
                    func, args
                ))

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
        build_cmd.append('CLANGARGS=-mllvm -accept-relax -O3')
    else:
        build_cmd.append('CLANGARGS=-O3')
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
    with sandbox(True):

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

        # Sequester filesystem output.
        if isinstance(output, basestring) and output.startswith('file:'):
            fn = output[len('file:'):]
            _, ext = os.path.splitext(fn)
            if not os.path.isdir(OUTPUTS_DIR):
                os.mkdir(OUTPUTS_DIR)
            output = os.path.join(OUTPUTS_DIR, _random_string() + ext)
            shutil.copyfile(fn, output)

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
