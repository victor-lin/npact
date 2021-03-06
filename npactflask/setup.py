#!/usr/bin/env python
import os
from setuptools import setup


def recursive_files(*roots):
    for r in roots:
        for root, directories, files in os.walk(r):
            for i in files:
                yield os.path.join(root, i)

setup(name='npactflask',
      version='0.6.1',
      description='Website for PYNPACT, the Python N-Profile Analysis Computation Tool',
      author='Nathan Bird',
      author_email='nathan@acceleration.net',
      url='http://genome.ufl.edu/npact/',
      packages=['npactflask'],
      package_data={'npactflask': list(recursive_files('static', 'templates'))},
      requires=["biopython(>=1.57)",
                "flask(==0.10)",
                "gunicorn",
                "pytz"],
      scripts=['bin/cleanup.py', 'bin/devserver']
)
