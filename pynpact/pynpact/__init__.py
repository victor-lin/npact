import logging
import os.path

from pkg_resources import resource_filename


class NullHandler(logging.Handler):
    "Swallow all logging messages"
    def emit(self, record):
        pass

# add a NullHandler so that we don't get "logging not configured"
# messages
logging.getLogger('pynpact').addHandler(NullHandler())


BINPATH = os.path.realpath(
    os.path.join(os.path.dirname(__file__), "bin"))


def binfile(name):
    path = os.path.join(BINPATH, name)
    assert os.path.exists(path), \
        "Missing pynpact/bin/" + name + ", perhaps you need to run make?"
    return path


DATAPATH = resource_filename(__name__, 'data')


class InvalidGBKException(Exception):
    """Indicates an invalid GBK file

    This class should only ever contain messages that are safe to
    present to the user.

    """
    pass
