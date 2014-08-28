"Provide a bunch of test fixtures for pynpact testing"
import random
import string
import logging
import pytest
import mock as mocklib
from path import path

from pynpact import parsing, executors


@pytest.fixture(scope="module", autouse=True)
def setup_logging():
    logging.root.setLevel(logging.DEBUG)
    logging.basicConfig()


class Patcher(object):
    def __init__(self):
        self._patchers = []

    def _stop(self):
        for patcher in self._patchers:
            patcher.stop()

    def _patch(self, method, *args, **kwds):
        if method:
            patch_fn = getattr(mocklib.patch, method)
        else:
            patch_fn = mocklib.patch

        patcher = patch_fn(*args, **kwds)
        self._patchers.append(patcher)

        return patcher.start()

    def patch(self, *args, **kwds):
        return self._patch(None, *args, **kwds)

    def patch_object(self, *args, **kwds):
        return self._patch('object', *args, **kwds)

    def patch_dict(self, *args, **kwds):
        return self._patch('dict', *args, **kwds)

    def patch_multiple(self, *args, **kwds):
        return self._patch('multiple', *args, **kwds)


@pytest.fixture
def patcher(request):
    p = Patcher()
    request.addfinalizer(p._stop)
    return p


@pytest.fixture
def magmock(request):
    return mocklib.MagicMock()



TESTGBK = "testdata/NC_017123.gbk"
TESTFNA = "testdata/NC_017123.fna"


@pytest.fixture()
def executor():
    return executors.InlineExecutor()



@pytest.fixture()
def gbkfile():
    gbk = path(__file__).dirname().joinpath(TESTGBK)
    assert gbk.exists()
    return str(gbk)


@pytest.fixture()
def gbkconfig(gbkfile, tmpdir):
    return parsing.initial(gbkfile, outputdir=str(tmpdir))


@pytest.fixture()
def fnafile():
    fna = path(__file__).dirname().joinpath(TESTFNA)
    assert fna.exists()
    return str(fna)


@pytest.fixture()
def fnaconfig(fnafile, tmpdir):
    return parsing.initial(fnafile, outputdir=str(tmpdir))
