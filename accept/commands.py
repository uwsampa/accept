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
APPS = ['streamcluster', 'blackscholes', 'sobel']


_client = None

def global_config(opts):
    global _client
    _client = cwmemo.get_client(cluster=opts.cluster, force=opts.force)


# Run the experiments.

@argh.arg('appnames', metavar='NAME', default=APPS, nargs='*', type=unicode,
          help='applications')
@argh.arg('-r', '--reps', metavar='N', default=None, type=int,
          help='replications')
def exp(appnames, verbose=False, cluster=False, reps=None):
    if not reps:
        reps = CLUSTER_REPS if cluster else LOCAL_REPS

    for appname in appnames:
        print(appname)
        experiments.evaluate(_client, appname, verbose, reps)


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
    with _client:
        _client.submit(log_and_output, appdir)
        _, output = _client.get(log_and_output, appdir)
    return output


def main():
    logging.getLogger().addHandler(logging.StreamHandler(sys.stderr))
    parser = argh.ArghParser()
    parser.add_commands([exp, log, build])
    parser.add_argument('--cluster', '-c', default=False, action='store_true',
                        help='execute on Slurm cluster')
    parser.add_argument('--force', '-f', default=False, action='store_true',
                        help='clear memoized results')
    parser.dispatch(pre_call=global_config)

if __name__ == '__main__':
    main()
