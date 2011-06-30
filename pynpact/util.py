import os, os.path, logging, subprocess, threading, time, errno
from subprocess import PIPE
from contextlib import contextmanager

def ensure_dir(dir, logger=None) :
    if not os.path.exists(dir) :
        try:
            if logger: logger.debug("Making dir: %s", dir)
            os.makedirs(dir)
            if logger: logger.info("Created dir: %s", dir)
        except OSError, e :
            #not entirely sure why we are getting errors,
            #http://docs.python.org/library/os.path.html#os.path.exists
            #says this could be related to not being able to call
            #os.stat, but I can do so from the command line python
            #just fine.
            if os.path.exists(dir) :
                if logger: logger.debug("Erred, already exists: e.errno: %s",e.errno)
                return
            else:
                raise


def withDir(dir, fn, *args,**kwargs) :
    olddir = os.getcwd()
    try :
        os.chdir(dir)
        return fn(*args,**kwargs)
    finally :
        os.chdir(olddir)


def pprint_bytes(bytes) :
    suffix = 'B'
    bytes= float(bytes)
    if bytes >= 1024 :
        bytes = bytes / 1024
        suffix = 'KB'
    if bytes >= 1024 :
        bytes = bytes / 1024
        suffix = 'MB'
    if bytes >= 1024 :
        bytes = bytes / 1024
        suffix = 'GB'
    if bytes >= 1024 :
        bytes = bytes / 1024
        suffix = 'TB'
    return '%.2f%s' % (bytes,suffix)




def exec_external( proc ):
    return subprocess.call(['python', proc])


def capturedCall(cmd, **kwargs) :
    """Do the equivelent of the subprocess.call except
    log the stderr and stdout where appropriate."""
    p= capturedPopen(cmd,**kwargs)
    rc = p.wait()
    #this is a cheap attempt to make sure the monitors
    #are scheduled and hopefully finished.
    time.sleep(0.01)
    time.sleep(0.01)
    return rc


def capturedPopen(cmd, stdin=None, stdout=None, stderr=None,
                  logger=logging,
                  stdout_level=logging.INFO,
                  stderr_level=logging.WARNING, **kwargs) :
    """Equivalent to subprocess.Popen except log stdout and stderr
    where appropriate. Also log the command being called."""
    #we use None as sigil values for stdin,stdout,stderr above so we
    # can distinguish from the caller passing in Pipe.

    if not kwargs.has_key("close_fds") and os.name == 'posix' :
        #http://old.nabble.com/subprocess.Popen-pipeline-bug--td16026600.html
        kwargs['close_fds'] = True

    if(logger):
        #if we are logging, record the command we're running,
        #trying to strip out passwords.
        logger.debug("Running cmd: %s",
                     isinstance(cmd,str) and cmd or subprocess.list2cmdline(cmd))

    p = subprocess.Popen(cmd, stdin=stdin,
                         stdout=(stdout or (logger and PIPE)),
                         stderr=(stderr or (logger and PIPE)),
                         **kwargs)
    if logger :
        def monitor(level, src, name) :
            lname = "%s.%s" % (cmd[0], name)
            if(hasattr(logger, 'name')) :
                lname = "%s.%s" % (logger.name, lname)
            sublog = logging.getLogger(lname)

            def tfn() :
                l = src.readline()
                while l != "":
                    sublog.log(level,l.strip())
                    l = src.readline()

            th = threading.Thread(target=tfn,name=lname)
            p.__setattr__("std%s_thread" % name, th)
            th.start()

        if stdout == None : monitor(stdout_level, p.stdout,"out")
        if stderr == None : monitor(stderr_level, p.stderr,"err")
    return p


@contextmanager
def guardPopen(cmd, timeout=0.1, timeout_count=2, **kwargs) :
    popen = None
    logger = kwargs.get('logger')
    try :
        popen = capturedPopen(cmd,**kwargs)
        yield popen
    finally :
        if popen:
            while popen.poll() == None and timeout and timeout_count > 0 :
                if logger:
                    logger.debug("Things don't look right yet waiting for %ss (%s tries left)", 
                                 timeout, timeout_count)
                time.sleep(timeout)
                timeout_count -=1
            if popen.poll() == None:
                if logger:
                    logger.exception("Terminating %s", cmd[0])
                popen.terminate()

def selfCaptured(klass) :
    def mungekw(self,kwargs) :
        if(not kwargs.has_key("logger")): kwargs["logger"] = self.logger
        return kwargs
    def add(func) :
        def newfunc(self,cmd,**kwargs) :
            return func(cmd, **mungekw(self,kwargs))
        setattr(klass,func.__name__,newfunc)

    add(capturedCall)
    add(capturedPopen)
    add(guardPopen)
    return klass

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


def stream_to_file(stream,path,bufsize=8192) :
    with open(path, "wb") as f:
        bytes=0
        while True :
            buf = stream.read(bufsize)
            if buf == "" :  break #EOF
            bytes += len(buf)
            f.write(buf)
        return bytes



# Copright 2011  Accelerated Data Works
# All rights reserved.