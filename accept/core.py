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
import string
import random
import locale
import sys
from .uncertain import umean
from collections import namedtuple


EVALSCRIPT = 'eval.py'
CONFIGFILE = 'accept_config.txt'
DESCFILE = 'accept_config_desc.txt'
BASEDIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUTS_DIR = os.path.join(BASEDIR, 'saved_outputs')
MAX_ERROR = 0.3
TIMEOUT_FACTOR = 3


# Utilities.

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

def normpath(path):
    enc = locale.getpreferredencoding()
    return os.path.normpath(os.path.abspath(os.path.expanduser(
        path.decode(enc)
    )))


# Command execution (system()-like) with timeouts.

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


# Building and executing using our makefile system.

def _make_args():
    return [
        'ENERCDIR={}'.format(BASEDIR),
        'APP_MK={}'.format(os.path.join(BASEDIR, 'apps', 'app.mk')),
        'CLANGARGS=-fcolor-diagnostics',
    ]

class BuildError(Exception):
    """The application failed to build.
    """
    pass

def execute(timeout, approx=False):
    """Run the application in the working directory and return the
    wall-clock duration (in seconds) of the execution and the exit
    status (or None if the process timed out).
    """
    command = ['make', 'run_opt' if approx else 'run_orig']
    command += _make_args()
    start_time = time.time()
    status = run_cmd(command, timeout)
    end_time = time.time()
    return end_time - start_time, status

def build(approx=False, require=True):
    """Compile the application in the working directory. If `approx`,
    then it is built with ACCEPT relaxation enabled. Return the combined
    stderr/stdout from the compilation process.
    """
    build_cmd = ['make', 'build_opt' if approx else 'build_orig']
    build_cmd += _make_args()

    proc = subprocess.Popen(build_cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    output, _ = proc.communicate()
    if require and proc.returncode:
        sys.stderr.write(output)
        raise BuildError()
    return output


# Manage the relaxation configuration file.

def parse_relax_config(f):
    """Parse a relaxation configuration from a file-like object.
    Generates (ident, param) tuples.
    """
    for line in f:
        line = line.strip()
        if line:
            ident, param = line.split(None, 2)
            yield int(ident), int(param)

def dump_relax_config(config, f):
    """Write a relaxation configuration to a file-like object. The
    configuration should be a sequence of tuples.
    """
    for site in config:
        f.write('{} {}\n'.format(*site))

def parse_relax_desc(f):
    """Parse a relaxation description map from a file-like object.
    Return a dict mapping identifiers to description strings.
    """
    out = {}
    for line in f:
        line = line.strip()
        if line:
            ident, desc = line.split(None, 1)
            out[int(ident)] = desc
    return out


# High-level profiling driver.

Execution = namedtuple('Execution', ['output', 'elapsed', 'status',
                                     'config', 'desc', 'roitime'])

def build_and_execute(directory, relax_config, rep, timeout=None):
    """Build the application in the given directory (which must contain
    both a Makefile and an eval.py), run it, and collect its output.
    Return an Execution object.
    """
    with chdir(directory):
        with sandbox(True):
            # Clean up any residual files.
            subprocess.check_call(['make', 'clean'] + _make_args())

            if relax_config:
                with open(CONFIGFILE, 'w') as f:
                    dump_relax_config(relax_config, f)
            elif os.path.exists(CONFIGFILE):
                os.remove(CONFIGFILE)
            if os.path.exists(DESCFILE):
                os.remove(DESCFILE)

            approx = bool(relax_config)
            build(approx)
            elapsed, status = execute(timeout, approx)
            if elapsed is None or status:
                # Timeout or error.
                output = None
                roitime = None
            else:
                # Load benchmark output.
                mod = imp.load_source('evalscript', EVALSCRIPT)
                try:
                    output = mod.load()
                except Exception as exc:
                    # Error reading benchmark output; this is a broken
                    # execution.
                    output = None
                    status = 'error loading output: ' + str(exc)

                # Load ROI duration.
                try:
                    with open('accept_time.txt') as f:
                        roitime = float(f.read())
                except Exception as exc:
                    # Error loading time.
                    roitime = None
                    status = 'error loading time: ' + str(exc)

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

    return Execution(output, elapsed, status, relax_config,
                     relax_desc, roitime)


# Configuration space exploration.

def permute_config(base):
    """Given a base (null) relaxation configuration, generate new
    (relaxed) configurations.
    """
    for i in range(len(base)):
        config = list(base)
        ident, _ = config[i]
        config[i] = ident, 1
        yield config

def combine_configs(configs):
    """Generate a new configuration that superimposes all the
    optimizations from the set of configurations.
    """
    configs = list(configs)
    if not configs:
        return

    # Combine nonzero parameters in an unordered way.
    sites = {}
    for config in configs:
        for ident, param in config:
            if param:
                sites[ident] = param

    # Modify the first config with the new parameters.
    out = configs[0]
    for i, (ident, param) in enumerate(out):
        if ident in sites:
            out[i] = ident, sites[ident]
    return out


# Results.

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


# Core relaxation search workflow.

class Evaluation(object):
    """The state for the evaluation of a single application.
    """
    def __init__(self, appdir, client, reps):
        """Set up an experiment. Takes an active CWMemo instance,
        `client`, through which jobs will be submitted and outputs
        collected. `reps` is the number of executions per configuration
        to run.
        """
        self.appdir = normpath(appdir)
        self.client = client
        self.reps = reps

        self.appname = os.path.basename(self.appdir)

        # Load scoring function from eval.py.
        with chdir(self.appdir):
            try:
                mod = imp.load_source('evalscript', EVALSCRIPT)
            except IOError:
                raise Exception('no eval.py found in {} directory'.format(
                    self.appname
                ))
        self.scorefunc = mod.score

        # Results, to be populated later.
        self.ptimes = []
        self.pout = None
        self.descs = None
        self.base_elapsed = None
        self.ptimes = []
        self.base_config = None
        self.results = []

    def setup(self):
        """Submit the baseline precise executions and gather some
        information about the first. Set the fields `pout` (the precise
        output), `pout` (precise execution time), `base_config`, and
        `descs`.
        """
        # Precise (baseline) execution.
        for rep in range(self.reps):
            self.client.submit(build_and_execute,
                self.appdir, None, rep,
                timeout=None
            )

        # Get information from the first execution. The rest of the
        # executions are for timing and can finish later.
        pex = self.client.get(build_and_execute, self.appdir, None, 0)
        self.pout = pex.output
        self.base_elapsed = pex.elapsed
        self.base_config = pex.config
        self.descs = pex.desc

    def precise_times(self):
        """Generate the durations for the precise executions. Must be
        called after `setup`.
        """
        for rep in range(self.reps):
            ex = self.client.get(build_and_execute, self.appdir, None, rep)
            yield ex.roitime

    def submit_approx_runs(self, config):
        """Submit the executions for a given configuration.
        """
        for rep in range(self.reps):
            self.client.submit(build_and_execute,
                self.appdir, config, rep,
                timeout=self.base_elapsed * TIMEOUT_FACTOR
            )

    def get_approx_result(self, config):
        """Gather the Result objects from a configuration. Must be
        called after previously submitting the same configuration.
        """
        # Collect all executions for the config.
        exs = [self.client.get(build_and_execute, self.appdir, config, rep)
               for rep in range(self.reps)]

        # Evaluate the result.
        res = Result(self.appname, config, [ex.roitime for ex in exs],
                     exs[0].status, exs[0].output)
        res.evaluate(self.scorefunc, self.pout, self.ptimes)
        return res

    def run_approx(self, configs):
        """Evaluate a set of approximate configurations. Return a list of
        results and append them to `self.results`.
        """
        # Relaxed executions.
        for config in configs:
            self.submit_approx_runs(config)

        # If we don't have the precise times yet, collect them. We need
        # them to evaluate approximate executions.
        if not self.ptimes:
            self.ptimes = list(self.precise_times())

        # Gather relaxed executions.
        res = [self.get_approx_result(config) for config in configs]
        self.results += res
        return res

    def run(self):
        """Execute the experiment.
        """
        self.setup()
        results = self.run_approx(list(permute_config(self.base_config)))

        # Evaluate a configuration that combines all the good ones.
        config = combine_configs(
            r.config for r in results if r.good
        )
        if config:
            self.run_approx([config])


