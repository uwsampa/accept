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


EVALSCRIPT = 'eval.py'
CONFIGFILE = 'accept_config.txt'
DESCFILE = 'accept_config_desc.txt'
BASEDIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUTS_DIR = os.path.join(BASEDIR, 'saved_outputs')


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
    ]

class BuildError(Exception):
    """The application failed to build.
    """
    pass

def execute(timeout):
    """Run the application in the working directory and return the
    wall-clock duration (in seconds) of the execution and the exit
    status (or None if the process timed out).
    """
    start_time = time.time()
    status = run_cmd(['make', 'run'] + _make_args(), timeout)
    end_time = time.time()
    return end_time - start_time, status

def build(approx=False, require=True):
    """Compile the application in the working directory. If `approx`,
    then it is built with ACCEPT relaxation enabled. Return the combined
    stderr/stdout from the compilation process.
    """
    subprocess.check_call(['make', 'clean'] + _make_args())

    build_cmd = ['make', 'build'] + _make_args()
    clang_args = '-O3 -fcolor-diagnostics'
    if approx:
        clang_args += '-mllvm -accept-relax'
    build_cmd.append('CLANGARGS={}'.format(clang_args))

    proc = subprocess.Popen(build_cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    output, _ = proc.communicate()
    if require and proc.returncode:
        raise BuildError()
    return output


# Manage the relaxation configuration file.

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


# High-level profiling driver.

def build_and_execute(directory, relax_config, rep, timeout=None):
    """Build the application in the given directory (which must contain
    both a Makefile and an eval.py), run it, and collect its output.
    Return the parsed output, the duration of the execution (or None for
    timeout), the exit status, the relaxation configuration, and the
    relaxation descriptions.
    """
    with chdir(directory):
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


# Configuration space exploration.

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
