from __future__ import print_function
from __future__ import division
from __future__ import absolute_import
import argh
import logging
import sys
import os
import subprocess
import locale
from . import experiments
from . import core
from . import cwmemo


LOCAL_REPS = 1
CLUSTER_REPS = 5
APPS = ['streamcluster', 'blackscholes', 'sobel', 'canneal']


_client = None

def global_config(opts):
    global _client
    _client = cwmemo.get_client(cluster=opts.cluster, force=opts.force)

def exppath(path):
    enc = locale.getpreferredencoding()
    return os.path.abspath(os.path.expanduser(path.decode(enc)))


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
    appdir = exppath(appdir)
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
    appdir = exppath(appdir)
    with _client:
        _client.submit(log_and_output, appdir)
        _, output = _client.get(log_and_output, appdir)
    return output


# Parts of the experiments.

@argh.arg('appdir', nargs='?', help='application directory')
def precise(appdir='.'):
    appdir = exppath(appdir)
    with _client:
        _client.submit(core.build_and_execute, appdir, None, 0)
        pout, ptime, _, base_config, descs = \
                _client.get(core.build_and_execute, appdir, None, 0)

    print('output:', pout)
    print('time: {:.2f}'.format(ptime))
    print('sites: {}'.format(len(descs)))
    for desc in descs.values():
        print('  ', desc)

@argh.arg('num', nargs='?', type=int, help='which configuration')
@argh.arg('appdir', nargs='?', help='application directory')
def approx(num=None, appdir='.'):
    appdir = exppath(appdir)
    with _client:
        _client.submit(core.build_and_execute, appdir, None, 0)
        _, ptime, _, base_config, descs = \
                _client.get(core.build_and_execute, appdir, None, 0)

        # Get configurations (and possibly narrow to only one).
        configs = list(core.permute_config(base_config))
        if num is not None:
            configs = [configs[num]]
        configs = [configs[num]] if num is not None else configs

        for config in configs:
            _client.submit(core.build_and_execute,
                appdir, config, 0,
                timeout=ptime * experiments.TIMEOUT_FACTOR
            )
            aout, atime, _, _, _ = \
                _client.get(core.build_and_execute, appdir, config, 0)

            print('output:', aout)
            print('time: {:.2f}'.format(atime))
            print()


def main():
    logging.getLogger().addHandler(logging.StreamHandler(sys.stderr))
    parser = argh.ArghParser()
    parser.add_commands([exp, log, build, precise, approx])
    parser.add_argument('--cluster', '-c', default=False, action='store_true',
                        help='execute on Slurm cluster')
    parser.add_argument('--force', '-f', default=False, action='store_true',
                        help='clear memoized results')
    parser.dispatch(pre_call=global_config)

if __name__ == '__main__':
    main()
