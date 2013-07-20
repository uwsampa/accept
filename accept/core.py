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
import logging
from .uncertain import umean
from collections import namedtuple
import itertools


EVALSCRIPT = 'eval.py'
CONFIGFILE = 'accept_config.txt'
BASEDIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUTS_DIR = os.path.join(BASEDIR, 'saved_outputs')
MAX_ERROR = 0.3
TIMEOUT_FACTOR = 3
MAX_GENERATIONS = 10  # How aggressively to apply each optimization.
PARAM_MAX = {
    'loop': 10,
    'lock': 1,
    'barrier': 1,
    'alias': 1,
}
EPSILON_ERROR = 0.001
EPSILON_SPEEDUP = 0.01
BUILD_TIMEOUT = 60 * 20

# Utilities.

@contextmanager
def chdir(d):
    """A context manager that temporary changes the working directory.
    """
    olddir = os.getcwd()
    os.chdir(d)
    yield
    os.chdir(olddir)

def symlink_all(src, dst):
    """Recursively symlink a file or a directory's contents.
    (Directories themselves are not symlinked.
    """
    if os.path.isdir(src):
        # A directory. Create the destination directory and symlink its
        # contents.
        os.mkdir(dst)
        for fn in os.listdir(src):
            symlink_all(os.path.join(src, fn), os.path.join(dst, fn))

    else:
        # A file. Just symlink the file itself.
        os.symlink(src, dst)

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
            symlink_all(src, dst)
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
        self.proc = subprocess.Popen(command, stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT)
        self.output = None

    def run(self):
        self.output, _ = self.proc.communicate()

def run_cmd(command, timeout=None):
    """Run a process with an optional timeout. Return the process' exit
    status and combined stdout/studerr text (or None for both if the
    process timed out).
    """
    thread = CommandThread(command)
    thread.start()
    thread.join(timeout)
    if thread.is_alive():
        thread.proc.terminate()
        return None, None
    else:
        return thread.proc.returncode, thread.output


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
    def __str__(self):
        return '\n' + self.args[0]

def execute(timeout, approx=False):
    """Run the application in the working directory and return the
    wall-clock duration (in seconds) of the execution and the exit
    status (or None if the process timed out).
    """
    command = ['make', 'run_opt' if approx else 'run_orig']
    command += _make_args()
    start_time = time.time()
    status, _ = run_cmd(command, timeout)
    end_time = time.time()
    return end_time - start_time, status

def build(approx=False, require=True):
    """Compile the application in the working directory. If `approx`,
    then it is built with ACCEPT relaxation enabled. Return the combined
    stderr/stdout from the compilation process.
    """
    build_cmd = ['make', 'build_opt' if approx else 'build_orig']
    build_cmd += _make_args()
    status, output = run_cmd(build_cmd, BUILD_TIMEOUT)
    if status is None:
        raise BuildError('build timed out')
    if require and status:
        raise BuildError(output)
    return output


# Manage the relaxation configuration file.

def parse_relax_config(f):
    """Parse a relaxation configuration from a file-like object.
    Generates (ident, param) tuples.
    """
    for line in f:
        line = line.strip()
        if line:
            param, ident = line.split(None, 1)
            yield ident, int(param)

def dump_relax_config(config, f):
    """Write a relaxation configuration to a file-like object. The
    configuration should be a sequence of tuples.
    """
    for ident, param in config:
        f.write('{} {}\n'.format(param, ident))


# Loading the evaluation script.

def load_eval_funcs(appdir):
    """Read the evaluation script (eval.py) for the application and
    return the load and score functions.
    """
    try:
        mod = imp.load_source('evalscript',
                              os.path.join(appdir, EVALSCRIPT))
    except IOError:
        raise Exception('no eval.py found in {}'.format(appdir))
    return mod.load, mod.score


# High-level profiling driver.

Execution = namedtuple('Execution', ['output', 'elapsed', 'status',
                                     'config', 'roitime'])

def build_and_execute(directory, relax_config, rep, timeout=None):
    """Build the application in the given directory (which must contain
    both a Makefile and an eval.py), run it, and collect its output.
    Return an Execution object.
    """
    with chdir(directory):
        with sandbox(True):
            # Clean up any residual files.
            run_cmd(['make', 'clean'] + _make_args())

            if relax_config:
                with open(CONFIGFILE, 'w') as f:
                    dump_relax_config(relax_config, f)
            elif os.path.exists(CONFIGFILE):
                os.remove(CONFIGFILE)

            approx = bool(relax_config)
            build(approx)
            elapsed, status = execute(timeout, approx)
            if elapsed is None or status:
                # Timeout or error.
                output = None
                roitime = None
            else:
                # Load benchmark output.
                loadfunc, _ = load_eval_funcs(directory)
                try:
                    output = loadfunc()
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
                try:
                    shutil.copyfile(fn, output)
                except IOError as exc:
                    # Error copying output file.
                    roitime = None
                    status = 'error copying output file: {}'.format(exc)

            if not relax_config:
                with open(CONFIGFILE) as f:
                    relax_config = list(parse_relax_config(f))

    return Execution(output, elapsed, status, relax_config, roitime)


# Configuration space exploration.

def permute_config(base):
    """Given a base (null) relaxation configuration, generate new
    (relaxed) configurations.
    """
    for i in range(len(base)):
        config = list(base)
        ident, _ = config[i]
        config[i] = ident, 1
        yield tuple(config)

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
    out = list(configs[0])
    for i, (ident, param) in enumerate(out):
        if ident in sites:
            out[i] = ident, sites[ident]
    return tuple(out)

def increase_config(config, amount=1):
    """Generate a new configuration that applies the given configuration
    more aggressively.
    """
    out = []
    for ident, param in config:
        if param:
            out.append((ident, param + amount))
        else:
            out.append((ident, param))
    return tuple(out)

def config_subsumes(a, b):
    """Given two configurations with the same sites, determine whether
    the first subsumes the second (all parameters in a are greater than
    or equal to those in b).
    """
    a_dict = dict(a)
    b_dict = dict(b)
    assert a_dict.keys() == b_dict.keys()

    for ident, a_param in a_dict.items():
        if a_param < b_dict[ident]:
            return False
    return True

def cap_config(config):
    """Reduce configuration parameters that exceed their maxima,
    returning a new configuration. `descs` describes each optimization
    site and dictates the maxima.
    """
    out = []
    for ident, param in config:
        max_param = PARAM_MAX[ident.split()[0]]
        if param > max_param:
            param = max_param
        out.append((ident, param))
    return tuple(out)

def _ratsum(vals):
    """Reciprocal of the sum or the reciprocal. Suitable for composing
    speedups, etc.
    """
    total = 0.0
    num = 0
    for v in vals:
        if v:
            total += v ** -1.0
            num += 1
    if num:
        return (total - (num - 1)) ** -1.0
    else:
        return 0.0

def _powerset(iterable, minsize=0):
    """From the itertools recipes."""
    s = list(iterable)
    return itertools.chain.from_iterable(
        itertools.combinations(s, r) for r in range(minsize, len(s)+1)
    )

def best_combined_configs(results):
    """Given a set of results, generate new configurations that compose
    the configurations used in the results. A heuristic chooses
    combinations that "should" give good results. Specifically, it
    generates the Pareto-optimal set of quality/performance under a
    linear combination hypothesis.
    """
    # Get flat tuples for efficient search. Note that we currently strip
    # away the uncertainty of the speedup for efficiency.
    components = [(r.config, r.speedup.value, r.error) for r in results]

    # Find all viable composed configurations.
    candidates = []
    for comps in _powerset(components, 2):
        component_configs = [c[0] for c in comps]
        config = combine_configs(component_configs)
        if config in component_configs:
            continue

        speedup = _ratsum(c[1] for c in comps)
        if speedup <= 0.0:
            continue

        error = sum(c[2] for c in comps)
        if error > MAX_ERROR:
            continue

        candidates.append((config, speedup, error))

    logging.info('combination candidates: {}'.format(len(candidates)))

    # Prune to Pareto-optimal configs.
    optimal = []
    for config, speedup, error in candidates:
        for _, other_speedup, other_error in candidates:
            if other_speedup >= speedup and other_error < error:
                break
        else:
            optimal.append((config, speedup, error))

    logging.info('optimal combinations: {}'.format(len(optimal)))
    return [o[0] for o in optimal]

def bce_greedy(results, max_error=MAX_ERROR):
    """Given a set of results and a maximum error constraint, choose a
    good combined configuration that composes the component
    configurations. Uses a greedy approximation to the Knapsack Problem.
    """
    components = [(r.config, r.speedup.value, r.error.value) for r in results]

    # Sort components by "value per unit weight".
    scored = []
    for i, (_, speedup, error) in enumerate(components):
        # Linear-combining performance score: 1 - s^-1
        value = 1.0 - speedup ** -1.0
        error = max(error, 0.001)  # Avoid divide-by-zero.
        score = value / error
        scored.append((score, i))
    scored.sort(reverse=True)

    # Greedily choose top-scoring components.
    knapsack = []  # Indices of chosen components.
    cur_combined = None  # Current merged config.
    cur_error = 0.0
    for _, i in scored:
        config, speedup, error = components[i]

        if cur_combined is None:
            # Add first item to knapsack.
            knapsack.append(i)
            cur_combined = config
            cur_error = error
        elif config_subsumes(cur_combined, config):
            # Don't bother trying to add configurations that are
            # subsumed by the current knapsack contents.
            continue
        elif cur_error + error <= max_error:
            # Add to knapsack.
            knapsack.append(i)
            cur_combined = combine_configs([cur_combined, config])
            cur_error += error

    # Here, we could predict the speedup for validation.

    for num in range(1, len(knapsack) + 1):
        yield combine_configs([components[i][0] for i in knapsack[:num]])


# Results.

class Result(object):
    """Represents the result of executing one relaxed configuration of a
    program.
    """
    def __init__(self, app, config, durations, statuses, outputs):
        self.app = app
        self.config = tuple(config)
        self.durations = durations
        self.statuses = statuses
        self.outputs = outputs

    def evaluate(self, scorefunc, precise_output, precise_durations):
        p_dur = umean(precise_durations)

        self.good = False  # A useful configuration?
        self.safe = False  # Not useful, but also not harmful?
        self.desc = 'unknown'  # Why not good?

        # Check for errors.
        for i, status in enumerate(self.statuses):
            if status is None:
                # Timed out.
                self.desc = 'replica {} timed out'.format(i)
                return
            elif status:
                # Error status.
                self.desc = 'error status (replica {}): {}'.format(
                    i, status
                )
                return

        # Get duration and speedup.
        for i, dur in enumerate(self.durations):
            if dur is None:
                self.desc = 'duration {} missing'.format(i)
                break
        self.duration = umean(self.durations)
        self.speedup = p_dur / self.duration

        # Get average output error.
        self.errors = []
        for i, output in enumerate(self.outputs):
            try:
                error = scorefunc(precise_output, output)
            except Exception as exc:
                self.error = 1.0
                self.desc = 'error in scoring function, replica {}: {}'.format(
                    i, exc
                )
                return
            self.errors.append(error)
        self.error = umean(self.errors)

        if self.error > MAX_ERROR:
            # Large output error.
            self.desc = 'large error: {}'.format(self.error)
            return

        # No error or large quality loss.
        self.safe = True

        if not self.speedup > 1.0:
            # Slowdown.
            self.desc = 'no speedup: {} vs. {}'.format(self.duration, p_dur)
            return

        # All good!
        self.good = True
        self.desc = 'good'

def triage_results(results):
    """Given a list of Result objects, splits the data set into three
    lists: optimal, suboptimal, and bad results.
    """
    # Partition bad from good.
    good = []
    bad = []
    for res in results:
        (good if res.good else bad).append(res)

    # Partition good into optimal and suboptimal.
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
            elif other in optimal and \
                    abs(other.error.value - res.error.value) < \
                    EPSILON_ERROR and \
                    abs(other.speedup.value - res.speedup.value) < \
                    EPSILON_SPEEDUP:
                # other and res are too similar, and other is already
                # optimal. Reduce noise by including only one.
                suboptimal.append(res)
                break
        else:
            # No other result dominates res.
            optimal.append(res)

    # Sort optimal by speedup.
    optimal.sort(key=lambda r: r.speedup.value, reverse=True)

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
        _, self.scorefunc = load_eval_funcs(appdir)

        # Results, to be populated later.
        self.ptimes = []
        self.pout = None
        self.base_elapsed = None
        self.ptimes = []
        self.base_config = None
        self.base_configs = None
        self.results = []

    def setup(self):
        """Submit the baseline precise executions and gather some
        information about the first. Set the fields `pout` (the precise
        output), `pout` (precise execution time), `base_config`, and
        `base_configs`.
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
        self.base_configs = list(permute_config(self.base_config))

    def precise_times(self):
        """Generate the durations for the precise executions. Must be
        called after `setup`.
        """
        for rep in range(self.reps):
            ex = self.client.get(build_and_execute, self.appdir, None, rep)
            if ex.status != 0:
                raise Exception('precise run failed: {}'.format(ex.status))
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
        res = Result(self.appname, config,
                     [ex.roitime for ex in exs],
                     [ex.status for ex in exs],
                     [ex.output for ex in exs])
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

    def evaluate_base(self):
        """Get results for all the base (single-opportunity)
        configurations. `setup()` must have been called to get the
        configuration space.
        """
        logging.info('evaluating base configurations')
        return self.run_approx(self.base_configs)

    def parameter_search(self, base_results):
        """Tune parameters for individual opportunity sites by
        increasing the parameters until an error threshold is exceeded
        or no speedup is gained. Pass the results produced by
        `evalute_base`. Returns all safe results found (including some
        of those base results that are passed in).
        """
        survivors = [r for r in base_results if r.safe]
        all_results = list(survivors)
        generation = 0

        while survivors and generation <= MAX_GENERATIONS:
            generation += 1
            logging.info('evaluating generation {}, population {}'.format(
                generation, len(survivors)
            ))

            # Increase the aggressiveness of each configuration and
            # evaluate.
            gen_configs = []
            for res in survivors:
                increased = cap_config(increase_config(res.config))
                if increased != res.config:
                    gen_configs.append(increased)
            gen_res = self.run_approx(gen_configs)

            # Produce the next generation, eliminating any config that
            # has too much error or that offers no speedup over its
            # parent.
            next_gen = []
            for res, old_res in zip(gen_res, survivors):
                if res.safe and res.duration < old_res.duration:
                    next_gen.append(res)
            survivors = next_gen
            all_results += survivors

        return all_results

    def evaluate_composites(self, component_results):
        """Compose the given configurations to evaluate some combined
        configurations.
        """
        logging.info('evaluating combined configs')
        optimal, good, _ = triage_results(component_results)
        candidates = set(bce_greedy(optimal))
        candidates = candidates.union(bce_greedy(optimal + good))
        return self.run_approx(list(candidates))
