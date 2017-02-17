import os
import os.path
import time
import librepo
import sys
from lettuce import *

CACHE = "fastestmirror.cache"

def remove_fn(fn):
    if os.path.exists(fn):
        os.remove(fn)

@before.each_feature
def feature_setup(feature):
    world.handle = None
    world.handle_options = {}
    world.packages = []
    world.filenames = []

@step(r'(\w+): (\S+)')
def handle_set_str(step, option, val):
    world.handle_options[option] = val

@step(r'Want to download location: (\S+)')
def package_to_download(step, path):
    world.filenames.append(os.path.basename(path))
    world.packages.append(path)

@step('I download the package three times')
def download_package(step):
    remove_fn(CACHE)
    for filename in world.filenames:
        remove_fn(filename)

    h = librepo.Handle()
    h.repotype = librepo.YUMREPO
    for opt, val in world.handle_options.iteritems():
        setattr(h, opt, val)

    # Download the package for the first time
    packages = []
    for package in world.packages:
        packages.append(librepo.PackageTarget(package, handle=h))
    t = time.time()
    librepo.download_packages(packages, failfast=True)
    world.t1 = time.time() - t
    for package in packages:
        assert (package.err is None)

    assert not os.path.exists(CACHE)
    for filename in world.filenames:
        assert os.path.exists(filename)
        remove_fn(filename)

    # Download the package for the second time
    h.fastestmirror = True
    h.fastestmirrorcache = CACHE
    packages = []
    for package in world.packages:
        packages.append(librepo.PackageTarget(package, handle=h))
    t = time.time()
    librepo.download_packages(packages)
    world.t2 = time.time() - t
    for package in packages:
        assert (package.err is None)

    assert os.path.exists(CACHE)
    for filename in world.filenames:
        assert os.path.exists(filename)
        remove_fn(filename)

    # Third download of the package
    h.fastestmirror = True
    h.fastestmirrorcache = CACHE
    packages = []
    for package in world.packages:
        packages.append(librepo.PackageTarget(package, handle=h))
    t = time.time()
    librepo.download_packages(packages)
    world.t3 = time.time() - t
    for package in packages:
        assert (package.err is None)

    assert os.path.exists(CACHE)
    for filename in world.filenames:
        assert os.path.exists(filename)
        remove_fn(filename)

@step('Third download with fastestmirrorcache should be the fastest')
def check_results(step):
    assert world.t2 > world.t3, "Download without cache is faster"

