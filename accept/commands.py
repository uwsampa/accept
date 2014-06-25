from __future__ import print_function
from __future__ import division
from __future__ import absolute_import
import click
import logging
import sys
import os
import subprocess
import json
import traceback
from collections import namedtuple
from . import experiments
from . import core
from . import cwmemo


LOCAL_REPS = 1
CLUSTER_REPS = 5
APPS = ['streamcluster', 'sobel', 'canneal', 'fluidanimate',
        'x264']
RESULTS_JSON = 'results.json'


GlobalConfig = namedtuple('GlobalConfig', 'client reps keep_sandboxes')


@click.group(help='the ACCEPT approximate compiler driver')
@click.option('--verbose', '-v', count=True, default=0,
              help='log more output')
@click.option('--cluster', '-c', is_flag=True,
              help='execute on Slurm cluster')
@click.option('--force', '-f', is_flag=True,
              help='clear memoized results')
@click.option('--reps', '-r', type=int, default=None,
              help='replication factor')
@click.option('--keep-sandboxes', '-k', is_flag=True,
              help='do not delete sandbox dirs')
@click.pass_context
def cli(ctx, verbose, cluster, force, reps, keep_sandboxes):
    # Set up logging.
    logging.getLogger().addHandler(logging.StreamHandler(sys.stderr))
    if verbose >= 2:
        logging.getLogger().setLevel(logging.DEBUG)
    elif verbose >= 1:
        logging.getLogger().setLevel(logging.INFO)

    # Set up the parallelism/memoization client.
    client = cwmemo.get_client(cluster=cluster, force=force)

    # Default reps.
    if reps is None:
        reps = CLUSTER_REPS if cluster else LOCAL_REPS

    ctx.obj = GlobalConfig(client, reps, keep_sandboxes)


# Run the experiments.

@cli.command()
@click.argument('appnames', metavar='NAME', default=APPS, nargs=-1,
                type=unicode)
@click.option('--json', '-j', 'as_json', default=False)
@click.option('--time', '-t', 'include_time', default=False)
@click.option('--only', '-o', 'only', multiple=True)
@click.option('--verbose', '-v', is_flag=True,
              help='show suboptimal results')
@click.pass_context
def exp(ctx, appnames, verbose, as_json, include_time, only):
    """Run the entire ACCEPT pipeline, by default on all apps
    """
    # Load the current results, if any.
    if as_json:
        try:
            with open(RESULTS_JSON) as f:
                results_json = json.load(f)
        except IOError:
            results_json = {}

    for appname in appnames:
        logging.info(appname)
        res = experiments.evaluate(ctx.obj.client, appname, verbose,
                                   ctx.obj.reps, as_json, only)

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


@cli.command()
@click.argument('appdir', default='.')
@click.option('--verbose', '-v', is_flag=True,
              help='show suboptimal results')
@click.option('--test', '-t', is_flag=True,
              help='test optimal configurations')
@click.pass_context
def run(ctx, appdir, verbose, test):
    """Run the ACCEPT workflow for a benchmark.

    Unlike the full experiments command (`accept exp`), this only gets
    the "headline" results for the benchmark; no characterization
    results for the paper are collected.
    """
    exp = core.Evaluation(appdir, ctx.obj.client, ctx.obj.reps)

    with ctx.obj.client:
        results = exp.run()

        # If we're getting test executions, run the optimal
        # configurations and instead print those results.
        if test:
            optimal, _, _ = core.triage_results(results)
            results = exp.test_runs([r.config for r in optimal])

    output = experiments.dump_results_human(results, exp.pout, verbose)
    for line in output:
        print(line)


# Get the compilation log or compiler output.

def log_and_output(directory, fn='accept_log.txt', keep=False):
    """Build the benchmark in `directory` and return the contents of the
    compilation log.
    """
    with core.chdir(directory):
        with core.sandbox(True, keep):
            if keep:
                logging.info('building in directory: {0}'.format(os.getcwd()))

            if os.path.exists(fn):
                os.remove(fn)

            output = core.build(require=False)

            if os.path.exists(fn):
                with open(fn) as f:
                    log = f.read()
            else:
                log = ''

            return log, output


@cli.command()
@click.argument('appdir', default='.')
@click.pass_context
def log(ctx, appdir):
    """Compile a program and show ACCEPT optimization log
    """
    appdir = core.normpath(appdir)
    with ctx.obj.client:
        ctx.obj.client.submit(log_and_output, appdir,
                              keep=ctx.obj.keep_sandboxes)
        logtxt, _ = ctx.obj.client.get(log_and_output, appdir)

    # Pass the log file through c++filt.
    filtproc = subprocess.Popen(['c++filt'], stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE)
    out, _ = filtproc.communicate(logtxt)
    return out


@cli.command()
@click.argument('appdir', default='.')
@click.pass_context
def build(ctx, appdir):
    """Compile a program and show compiler output
    """
    appdir = core.normpath(appdir)
    with ctx.obj.client:
        ctx.obj.client.submit(log_and_output, appdir, ctx.obj.keep_sandboxes)
        _, output = ctx.obj.client.get(log_and_output, appdir)
    return output


# Parts of the experiments.

@cli.command()
@click.argument('appdir', default='.')
@click.pass_context
def precise(ctx, appdir):
    """Execute the precise (baseline) version of a program
    """
    ev = core.Evaluation(appdir, ctx.obj.client, ctx.obj.reps)
    with ctx.obj.client:
        ev.setup()
        times = list(ev.precise_times())

    print('output:', ev.pout)
    print('time:')
    for t in times:
        print('  {:.2f}'.format(t))


@cli.command()
@click.argument('num', type=int, default=None)
@click.argument('appdir', default='.')
@click.pass_context
def approx(ctx, num, appdir):
    """Execute approximate versions of a program
    """
    ev = core.Evaluation(appdir, ctx.obj.client, ctx.obj.reps)
    with ctx.obj.client:
        experiments.run_experiments(ev)
    results = ev.results

    # Possibly choose a specific result.
    results = [results[num]] if num is not None else results

    for result in results:
        print(experiments.dump_config(result.config))
        print('output:')
        for output in result.outputs:
            print('  {}'.format(output))
        print('time:')
        for t in result.durations:
            if t is None:
                print('  (error)')
            else:
                print('  {:.2f}'.format(t))
        print()


def main():
    try:
        cli()
    except core.UserError as exc:
        logging.debug(traceback.format_exc())
        logging.error(exc.log())
        sys.exit(1)


if __name__ == '__main__':
    main()
