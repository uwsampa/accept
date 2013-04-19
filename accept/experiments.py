from __future__ import print_function
from __future__ import division
import math
import os
import imp
from . import core
from .cwmemo import get_client


MAX_ERROR = 0.3
TIMEOUT_FACTOR = 3
APPSDIR = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'apps')


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


class Experiment(object):
    """The state for the evaluation of a single application.
    """
    def __init__(self, appname, client, scorefunc, reps):
        """Set up an experiment. Takes an active CWMemo instance,
        `client`, through which jobs will be submitted and outputs
        collected. `scorefunc` is the application's output quality
        scoring function. `reps` is the number of executions per
        configuration to run.
        """
        self.appname = appname
        self.client = client
        self.scorefunc = scorefunc
        self.reps = reps

        self.appdir = os.path.join(APPSDIR, appname)

        # Results, to be populated later.
        self.ptimes = []
        self.pout = None
        self.descs = None
        self.ptime = None
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
            self.client.submit(core.build_and_execute,
                self.appdir, None, rep,
                timeout=None
            )

        # Get information from the first execution. The rest of the
        # executions are for timing and can finish later.
        self.pout, self.ptime, _, self.base_config, self.descs = \
                self.client.get(
                    core.build_and_execute,
                    self.appdir, None, 0
                )

    def precise_times(self):
        """Generate the durations for the precise executions. Must be
        called after `setup`.
        """
        for rep in range(self.reps):
            _, dur, _, _, _ = self.client.get(core.build_and_execute,
                                              self.appdir, None, rep)
            yield dur

    def submit_approx_runs(self, config):
        """Submit the executions for a given configuration.
        """
        for rep in range(self.reps):
            self.client.submit(core.build_and_execute,
                self.appdir, config, rep,
                timeout=self.ptime * TIMEOUT_FACTOR
            )

    def get_approx_result(self, config):
        """Gather the Result objects from a configuration. Must be
        called after previously submitting the same configuration.
        """
        # Get outputs from first execution.
        aout, _, status, _, _ = self.client.get(core.build_and_execute,
                                                self.appdir, config, 0)

        # Get all execution times.
        atimes = []
        for rep in range(self.reps):
            _, dur, _, _, _ = self.client.get(core.build_and_execute,
                                              self.appdir, config, rep)
            atimes.append(dur)

        # Evaluate the result.
        res = Result(self.appname, config, atimes, status, aout)
        res.evaluate(self.scorefunc, self.pout, self.ptimes)
        return res

    def run_approx(self, configs):
        """Evaluate a set of approximate configurations. Return a list of
        results.
        """
        # Relaxed executions.
        for config in configs:
            self.submit_approx_runs(config)

        # If we don't have the precise times yet, collect them. We need
        # them to evaluate approximate executions.
        if not self.ptimes:
            self.ptimes = list(self.precise_times())

        # Gather relaxed executions.
        return [self.get_approx_result(config) for config in configs]

    def run(self):
        """Execute the experiment.
        """
        self.setup()
        res = self.run_approx(list(core.permute_config(self.base_config)))
        self.results += res


def evaluate(client, appname, verbose=False, reps=1):
    with core.chdir(os.path.join(APPSDIR, appname)):
        try:
            mod = imp.load_source('evalscript', core.EVALSCRIPT)
        except IOError:
            assert False, 'no eval.py found in {} directory'.format(appname)

    exp = Experiment(appname, client, mod.score, reps)
    with client:
        exp.run()
    results, descs = exp.results, exp.descs

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
            print(dump_config(res.config, descs))
            print(res.desc)
