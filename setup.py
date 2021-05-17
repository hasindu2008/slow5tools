#adapted from https://github.com/lh3/minimap2/blob/master/setup.py

try:
	from setuptools import setup, Extension
except ImportError:
	from distutils.core import setup
	from distutils.extension import Extension


cmdclass={}

from Cython.Build import build_ext
module_src = 'python/pyslow5.pyx'
cmdclass['build_ext'] = build_ext


#TODO add all
sources=[module_src, 'src/slow5.c', 'src/slow5_press.c', 'src/slow5_misc.c', 'src/slow5_idx.c' ]
depends=['python/pyslow5.pxd', 'python/pyslow5.h', 'src/config.h', 'src/error.h', 'src/slow5.h', 'src/misc.h', 'src/press.h', 'src/slow5idx.h']
extra_compile_args = ['-g', '-Wall', '-O2', '-std=c++11', '-Wno-strict-prototypes']
libraries = ['z','m','pthread', 'hdf5_serial']
include_dirs = ['.']
library_dirs = ['.']

#py_inc = [get_python_inc()]

#np_lib = os.path.dirname(numpy.__file__)
#np_inc = [os.path.join(np_lib, 'core/include')]
#cmdclass = {'build_py': build_py}


#cmdclass.update({'build_ext': build_ext})
#packages=['test']


extensions = [Extension('pyslow5',
                  sources = sources,
                  depends = depends,
                  extra_compile_args = extra_compile_args,
                  libraries = libraries,
                  include_dirs = include_dirs,
                  library_dirs = library_dirs,
                  language = 'c++' )]

#TODO ad all
setup(name = 'slow5',
      version='0.0',
      url = 'https://github.com/hasindu2008/slow5',
      #requires=['numpy (>=1.3.0)'],
      description='f5c python binding',
      author='Hasindu Gamaarachchi, Sasha Jenner, James ferguson ...',
      author_email='hasindu@garvan.org.au, ...',
      maintainer='Hasindu Gamaarachchi, Sasha Jenner, James ferguson ...',
      maintainer_email='hasindu@garvan.org.au',
      license = 'MIT',
      keywords = ['slow5','nanopore'],
      #packages=packages,
      cmdclass=cmdclass,
      ext_modules=extensions
      #ext_modules=cythonize(extensions),
      )
