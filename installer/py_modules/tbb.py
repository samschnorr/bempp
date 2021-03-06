#Copyright (C) 2011 by the BEM++ Authors
#
#Permission is hereby granted, free of charge, to any person obtaining a copy
#of this software and associated documentation files (the "Software"), to deal
#in the Software without restriction, including without limitation the rights
#to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#copies of the Software, and to permit persons to whom the Software is
#furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in
#all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#THE SOFTWARE.

import os,urllib,shutil,subprocess,sys
from py_modules import tools
from py_modules import python_patch as py_patch
import struct

tbb_fname_mac='tbb42_20131003oss_osx.tgz'
tbb_fname_linux='tbb42_20131003oss_lin.tgz'
tbb_url_mac='http://threadingbuildingblocks.org/sites/default/files/software_releases/mac/tbb42_20131003oss_osx.tgz'
tbb_url_linux='http://threadingbuildingblocks.org/sites/default/files/software_releases/linux/tbb42_20131003oss_lin.tgz'
tbb_extract_dir='tbb42_20131003oss'
tbb_dir='tbb'

if sys.platform.startswith('darwin'):
    tbb_fname = tbb_fname_mac
elif sys.platform.startswith('linux'):
    tbb_fname = tbb_fname_linux
else:
    raise Exception("Platform not supported")

def download(root,config,force=False):
    dep_build_dir=config.get('Main','dependency_build_dir')
    dep_download_dir=config.get('Main','dependency_download_dir')
    tbb_full_dir=dep_build_dir+"/"+tbb_dir
    if sys.platform.startswith('darwin'):
        tbb_download_name=dep_download_dir+"/"+tbb_fname_mac
        tbb_url=tbb_url_mac
    elif sys.platform.startswith('linux'):
        tbb_download_name=dep_download_dir+"/"+tbb_fname_linux
        tbb_url=tbb_url_linux
    else:
        raise Exception("Platform not supported")
    tools.download(tbb_fname,tbb_url,dep_download_dir,force)

def prepare(root,config):
    dep_build_dir=config.get('Main','dependency_build_dir')
    dep_download_dir=config.get('Main','dependency_download_dir')
    prefix=config.get('Main','prefix')

    print "Extracting Tbb"

    tools.checkDeleteDirectory(dep_build_dir+"/tbb")
    try:
        tools.extract_file(dep_download_dir+"/"+tbb_fname,dep_build_dir)
    except IOError:
        # Possibly a corrupted/truncated file. Try to download once again
        download(root,config,force=True)
        tools.extract_file(dep_download_dir+"/"+tbb_fname,dep_build_dir)
    os.rename(dep_build_dir+"/"+tbb_extract_dir,dep_build_dir+"/tbb")
    print "Patching Tbb"
    cwd=os.getcwd()

    os.chdir(dep_build_dir+"/tbb")
    patch=py_patch.fromfile(root+"/installer/patches/tbb_pipeline.patch")
    patch.apply()
    os.chdir(cwd)

    subprocess.check_call("cp -R "+dep_build_dir+"/tbb/include/* "+
                          prefix+"/bempp/include/",shell=True)


    if sys.platform.startswith('darwin'):
        libdir_orig = dep_build_dir+"/tbb/lib"
        tbb_lib_name="libtbb.dylib"
        tbb_lib_name_debug="libtbb_debug.dylib"
        tbb_libmalloc_name="libtbbmalloc.dylib"
        tbb_libmalloc_name_debug="libtbbmalloc_debug.dylib"
    elif sys.platform.startswith('linux'):
        tbb_lib_name = "libtbb.so"
        tbb_lib_name_debug = "libtbb_debug.so"
        tbb_libmalloc_name = "libtbbmalloc.so"
        tbb_libmalloc_name_debug = "libtbbmalloc_debug.so"
        arch = config.get('Main','architecture')
        if arch in ('intel64','ia32','ia64'):
            libdir_orig = (dep_build_dir+"/tbb/lib/"+arch+
                           "/gcc4.4")
        else:
            raise Exception("Unrecognized architecture: '"+arch+"'")
    else:
        raise Exception("Platform not supported")

    subprocess.check_call("cp -R "+libdir_orig+"/* "+prefix+"/bempp/lib/",shell=True)

    tools.setDefaultConfigOption(config,"Tbb",'lib',prefix+"/bempp/lib/"+tbb_lib_name,overwrite=True)
    tools.setDefaultConfigOption(config,"Tbb","lib_debug",prefix+"/bempp/lib/"+tbb_lib_name_debug,overwrite=True)
    tools.setDefaultConfigOption(config,"Tbb",'libmalloc',prefix+"/bempp/lib/"+tbb_libmalloc_name,overwrite=True)
    tools.setDefaultConfigOption(config,"Tbb","libmalloc_debug",prefix+"/bempp/lib/"+tbb_libmalloc_name_debug,overwrite=True)
    tools.setDefaultConfigOption(config,"Tbb",'include_dir',prefix+"/bempp/include",overwrite=True)

def configure(root,config):
    pass

def build(root,config):
    pass

def install(root,config):
    pass
