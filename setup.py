from distutils.core import setup, Extension

module1 = Extension('pystarcode',
      define_macros = [('MAJOR_VERSION', '1'), ('MINOR_VERSION', '0')],
      include_dirs = ['src/'],
      sources = ['pystarcode.c', 'src/starcode.c', 'src/trie.c'],
      extra_compile_args = ['-std=c99'])

setup(name = 'pystarcode',
    version = '1.0',
    description = 'Starcode library for Python',
    ext_modules = [module1])
