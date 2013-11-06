from __future__ import print_function
from __future__ import division
from __future__ import absolute_import
import argh
import logging
import sys
import os
import subprocess
import json
from . import experiments
from . import core
from . import cwmemo


LOCAL_REPS = 1
CLUSTER_REPS = 5
APPS = ['streamcluster', 'sobel', 'canneal', 'fluidanimate',
        'x264']
RESULTS_JSON = 'results.json'


_client = None
_reps = 1
_keep_sandboxes = False

def global_config(opts):
    global _client
    global _reps
    global _keep_sandboxes
    _client = cwmemo.get_client(cluster=opts.cluster, force=opts.force)
    if opts.reps:
        _reps = opts.reps
    else:
        _reps = CLUSTER_REPS if opts.cluster else LOCAL_REPS
    if opts.keep_sandboxes:
        _keep_sandboxes = True
    if opts.verbose >= 2:
        logging.getLogger().setLevel(logging.DEBUG)
    elif opts.verbose >= 1:
        logging.getLogger().setLevel(logging.INFO)


# Run the experiments.

@argh.arg('appnames', metavar='NAME', default=APPS, nargs='*', type=unicode,
          help='applications')
@argh.arg('--json', '-j', dest='as_json')
@argh.arg('--time', '-t', dest='include_time')
@argh.arg('--only', '-o', dest='only', action='append')
def exp(appnames, verbose=False, as_json=False, include_time=False, only=None):
    # Load the current results, if any.
    if as_json:
        try:
            with open(RESULTS_JSON) as f:
                results_json = json.load(f)
        except IOError:
            results_json = {}

    for appname in appnames:
        logging.info(appname)
        res = experiments.evaluate(_client, appname, verbose, _reps, as_json,
                                   only)

        if as_json:
            if not include_time:
                del res['time']
            if appname not in results_json:
                results_json[appname] = {}
            results_json[appname].update(res)

        else:
            print(res)

        # Dump the results back to the JSON file.
        if as_json:
            with open(RESULTS_JSON, 'w') as f:
                json.dump(results_json, f, indent=2, sort_keys=True)


# Get the compilation log or compiler output.

def log_and_output(directory, fn='accept_log.txt'):
    """Build the benchmark in `directory` and return the contents of the
    compilation log.
    """
    with core.chdir(directory):
        with core.sandbox(True, _keep_sandboxes):
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
    ev = core.Evaluation(appdir, _client, _reps)
    with _client:
        ev.setup()
        times = list(ev.precise_times())

    print('output:', ev.pout)
    print('time:')
    for t in times:
        print('  {:.2f}'.format(t))

@argh.arg('num', nargs='?', type=int, help='which configuration')
@argh.arg('appdir', nargs='?', help='application directory')
def approx(num=None, appdir='.'):
    ev = core.Evaluation(appdir, _client, _reps)
    with _client:
        experiments.run_experiments(ev)
    results = ev.results

    # Possibly choose a specific result.
    results = [results[num]] if num is not None else results

    for result in results:
        print(experiments.dump_config(result.config))
        print('output:', result.output)
        print('time:')
        for t in result.durations:
            if t is None:
                print('  (error)')
            else:
                print('  {:.2f}'.format(t))
        print()


def main():
    logging.getLogger().addHandler(logging.StreamHandler(sys.stderr))
    parser = argh.ArghParser(prog='accept')
    parser.add_commands([exp, log, build, precise, approx])
    parser.add_argument('--cluster', '-c', default=False, action='store_true',
                        help='execute on Slurm cluster')
    parser.add_argument('--force', '-f', default=False, action='store_true',
                        help='clear memoized results')
    parser.add_argument('--reps', '-r', type=int,
                        help='replication runs')
    parser.add_argument('--verbose', '-v', action='count', default=0,
                        help='enable verbose logging')
    parser.add_argument('--keep-sandboxes', '-k', action='store_true',
                        dest='keep_sandboxes', default=False,
                        help='keep intermediate sandbox directories')
    parser.dispatch(pre_call=global_config)

if __name__ == '__main__':
    main()
