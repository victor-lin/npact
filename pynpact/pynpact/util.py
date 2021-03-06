import errno
import os
import os.path
import logging
import time
import hashlib
import tempfile
from contextlib import contextmanager
from functools import wraps
from path import path as pathlib


def reducehashdict(dict, keys):
    """pull the given keys out of the dictionary, return the reduced
    dictionary and the sha1 hash of that set of key values.
    """
    outdict = {}
    h = hashlib.sha1()
    # We go through in sorted order to ensure stability of the
    # ordering between runs.
    for k in sorted(keys):
        val = dict.get(k)
        if val is not None:
            h.update(k)
            h.update(str(val))
            outdict[k] = val

    if len(outdict):
        return outdict, h.hexdigest()
    else:
        return outdict, None


def reducedict(dict_, keys):
    out = {}
    for k in keys:
        if k in dict_:
            out[k] = dict_[k]
    return out


def hashdict(dict_):
    h = hashlib.sha1()
    for k in sorted(dict_.keys()):
        val = dict_.get(k)
        if val is not None:
            h.update(k)
            h.update(str(val))
    return h.hexdigest()


class Hasher(object):
    def __init__(self):
        self.state = hashlib.sha1()

    def hashdict(self, dict_):
        for k in sorted(dict_.keys()):
            val = dict_.get(k)
            if val is not None:
                self.state.update(k)
                self.state.update(str(val))
        return self

    def hashfiletime(self, filename):
        self.state.update(str(os.path.getmtime(filename)))
        return self

    def hashlist(self, lst):
        for item in lst:
            self.state.update(str(item))
        return self

    def hash(self, str_):
        self.state.update(str_)
        return self

    def hexdigest(self):
        return self.state.hexdigest()

def ensure_dir(dir, logger=None):
    if not os.path.exists(dir):
        try:
            if logger: logger.debug("Making dir: %s", dir)
            os.makedirs(dir)
            if logger: logger.info("Created dir: %s", dir)
        except OSError, e:
            #not entirely sure why we are getting errors,
            #http://docs.python.org/library/os.path.html#os.path.exists
            #says this could be related to not being able to call
            #os.stat, but I can do so from the command line python
            #just fine.
            if os.path.exists(dir):
                if logger:
                    logger.debug("Erred, already exists: e.errno: %s", e.errno)
                return
            else:
                raise


def withDir(dir, fn, *args, **kwargs):
    olddir = os.getcwd()
    try:
        os.chdir(dir)
        return fn(*args, **kwargs)
    finally:
        os.chdir(olddir)


def pprint_bytes(bytes):
    suffix = 'B'
    bytes = float(bytes)
    if bytes >= 1024:
        bytes = bytes / 1024
        suffix = 'KB'
    if bytes >= 1024:
        bytes = bytes / 1024
        suffix = 'MB'
    if bytes >= 1024:
        bytes = bytes / 1024
        suffix = 'GB'
    if bytes >= 1024:
        bytes = bytes / 1024
        suffix = 'TB'
    return '%.2f%s' % (bytes, suffix)


def which(program):
    def is_exe(fpath):
        return os.path.exists(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None


def stream_to_handle(stream, handle, bufsize=8192):
    bytes = 0
    while True:
        buf = stream.read(bufsize)
        if buf == "": break  # EOF
        bytes += len(buf)
        handle.write(buf)
    return bytes


def stream_to_file(stream, path, bufsize=8192):
    if hasattr(path, 'write'):
        return stream_to_handle(stream, path, bufsize)
    else:
        with open(path, "wb") as h:
            return stream_to_handle(stream, h, bufsize)


@contextmanager
def mkstemp_rename(destination, **kwargs):
    """For writing to a temporary file and then move it ontop of a
    (possibly) existing file only when finished.  This enables us to
    perform long running operations on a file that other people might
    be using and let everyone else see a consistent version.

    * other args are passed to tempfile.mkstemp

    Example::

        with mkstemp_rename('foobar.txt') as f:
            f.write('stuff\n')

    """
    kwargs.setdefault('dir', os.path.dirname(destination))

    (fd, path) = tempfile.mkstemp(**kwargs)
    path = pathlib(path)
    try:
        filelike = os.fdopen(fd, 'wb')
        yield filelike
        filelike.close()
        path.rename(destination)
    finally:
        path.remove_p()


@contextmanager
def mkdtemp_rename(destination, **kwargs):
    """A wrapper for tempfile.mkdtemp that always cleans up.

    This wrapper sets defaults based on the class values."""
    dest = pathlib(destination).normpath()
    kwargs.setdefault('dir', dest.parent)
    tmppath = pathlib(tempfile.mkdtemp(**kwargs))
    try:
        yield tmppath
        try:
            tmppath.rename(dest)
        except OSError as e:
            if e.errno == errno.EEXIST:
                # I don't think we'll ever get here.
                dest.rmtree_p()
                tmppath.rename(dest)
            else:
                raise
    finally:
        tmppath.rmtree_p()


def replace_ext(base, newext):
    base = pathlib(base)
    if newext[0] == '.':
        newext = newext[1:]
    return base.stripext() + '.' + newext


def is_outofdate(filename, *dependencies):
    """Return true if the file is missing or not newer than all of its dependencies."""
    if not os.path.exists(filename):
        return True

    mtime = os.path.getmtime(filename)
    return any(os.path.getmtime(d) > mtime for d in dependencies if d)


def derivative_filename(base, part, replace_ext=True, outputdir=None):
    """Build the filename of a derivative product of the original
    file."""

    if not part[0] == ".":
        part = "." + part

    if outputdir is None:
        outputdir = os.path.dirname(base)
    filename = os.path.basename(base)

    if replace_ext:
        filename = os.path.splitext(filename)[0]

    return os.path.join(outputdir, filename + part)


def safe_produce_new(outfilename, func, force=False, dependencies=[], **kwargs):
    logger = kwargs.get('logger')
    if force or is_outofdate(outfilename, *dependencies):
        if logger:
            logger.debug(
                "Regenerating, checked:%d force:%r", len(dependencies), force)

        with mkstemp_rename(outfilename, **kwargs) as f:
            func(f)
    return outfilename


def log_time(logger=logging, level=logging.INFO):
    def decorator(func):
        def wrapper(*args, **kwargs):
            t1 = time.time()
            result = func(*args, **kwargs)
            t2 = time.time()
            logger.log(level, "%s done, took %fs" % (func.func_name, (t2-t1)))
            return result
        return wraps(func)(wrapper)
    return decorator


class Task(object):
    """Small object to hold state for a function call to happen later.

    The point of this is to be a pickable closure looking thing.

    E.g.

        def adder(a,b):
            return a + b

        Task(adder, 1, 2)() == 3
    """
    func = None
    args = None
    kwargs = None

    def __init__(self, func, *args, **kwargs):
        self.func = func
        self.args = args
        self.kwargs = kwargs

    def __call__(self):
        return self.func(*self.args, **self.kwargs)


def delay(fn):
    """Create a Task out of the target function(and arguments)

    I.e. make the target function serializable.

    E.g.

       delay(sum)([1, 2])

    results in a callable Task object that can be serialized.

       delay(sum)([1, 2])() == 3
    """
    @wraps(fn)
    def wrapper(*args, **kwargs):
        return Task(fn, *args, **kwargs)
    return wrapper


# Copright (c) 2011,2012,2013,2014  Accelerated Data Works
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:

#     Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.

#     Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following
#     disclaimer in the documentation and/or other materials provided
#     with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
