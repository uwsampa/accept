from __future__ import print_function
from __future__ import division
from __future__ import absolute_import
import argh
import logging
import sys
import os
import subprocess
from . import experiments
from . import core
from . import cwmemo


LOCAL_REPS = 1
CLUSTER_REPS = 5
APPS = ['streamcluster', 'blackscholes', 'sobel', 'canneal']


_client = None
_reps = 1

def global_config(opts):
    global _client
    global _reps
    _client = cwmemo.get_client(cluster=opts.cluster, force=opts.force)
    if opts.reps:
        _reps = opts.reps
    else:
        _reps = CLUSTER_REPS if opts.cluster else LOCAL_REPS


# Run the experiments.

@argh.arg('appnames', metavar='NAME', default=APPS, nargs='*', type=unicode,
          help='applications')
def exp(appnames, verbose=False):
    for appname in appnames:
        print(appname)
        experiments.evaluate(_client, appname, verbose, _reps)


# Get the compilation log or compiler output.

def log_and_output(directory, fn='accept_log.txt'):
    """Build the benchmark in `directory` and return the contents of the
    compilation log.
    """
    with core.chdir(directory):
        with core.sandbox(True):
            if os.path.exists(fn):
                os.remove(fn)

            output = core.build(require=False)

            if os.path.exists(fn):
                with open(fn) as f:
                    log = f.read()
            else:
                log = ''

            return log, output

@argh.arg('appdir', nargs='?', help='application directory')
def log(appdir='.'):
    appdir = core.normpath(appdir)
    with _client:
        _client.submit(log_and_output, appdir)
        logtxt, _ = _client.get(log_and_output, appdir)

    # Pass the log file through c++filt.
    filtproc = subprocess.Popen(['c++filt'], stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE)
    out, _ = filtproc.communicate(logtxt)
    return out

@argh.arg('appdir', nargs='?', help='application directory')
def build(appdir='.'):
    appdir = core.normpath(appdir)
    with _client:
        _client.submit(log_and_output, appdir)
        _, output = _client.get(log_and_output, appdir)
    return output


# Parts of the experiments.

@argh.arg('appdir', nargs='?', help='application directory')
def precise(appdir='.'):
    ev = experiments.Evaluation(appdir, _client, _reps)
    with _client:
        ev.setup()
        times = list(ev.precise_times())

    print('output:', ev.pout)
    print('time:')
    for t in times:
        print('  {:.2f}'.format(t))
    print('sites: {}'.format(len(ev.descs)))
    for desc in ev.descs.values():
        print(' ', desc)

@argh.arg('num', nargs='?', type=int, help='which configuration')
@argh.arg('appdir', nargs='?', help='application directory')
def approx(num=None, appdir='.'):
    ev = experiments.Evaluation(appdir, _client, _reps)
    with _client:
        ev.run()
    results = ev.results

    # Possibly choose a specific result.
    results = [results[num]] if num is not None else results

    for result in results:
        print(experiments.dump_config(result.config, ev.descs))
        print('output:', result.output)
        print('time:')
        for t in result.durations:
            print('  {:.2f}'.format(t))
        print()


def main():
    logging.getLogger().addHandler(logging.StreamHandler(sys.stderr))
    parser = argh.ArghParser()
    parser.add_commands([exp, log, build, precise, approx])
    parser.add_argument('--cluster', '-c', default=False, action='store_true',
                        help='execute on Slurm cluster')
    parser.add_argument('--force', '-f', default=False, action='store_true',
                        help='clear memoized results')
    parser.add_argument('--reps', '-r', type=int,
                        help='replication runs')
    parser.dispatch(pre_call=global_config)

if __name__ == '__main__':
    main()
