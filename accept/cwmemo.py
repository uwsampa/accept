from sqlite3dbm import sshelve
try:
    import cPickle as pickle
except ImportError:
    import pickle
import logging
import os
try:
    import cw.client
except ImportError:
    pass


MEMO_DB = os.path.abspath(os.path.join(
    os.path.dirname(os.path.dirname(__file__)),
    'memo.db'
))


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
            self.client = None
            self.jobs = {}
            self.completion_cond = self.client.jobs_cond

    def __enter__(self):
        if not self.local:
            # Create a new thread for each time this memoizing agent is
            # entered. This allows a CWMemo to be reused.
            self.client = cw.client.ClientThread(self.completion, self.host)
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


def get_client(cluster=False, force=False):
    """Return a `CWMemo` instance for ACCEPT computations.
    """
    master_host = cw.slurm_master_host() if cluster else None
    return CWMemo(dbname=MEMO_DB, host=master_host, force=force)
