from distutils.core import setup, Extension

setup(
    ext_modules = [Extension("pystarcode", ["pystarcode.c",
                                            "../src/starcode.c",
                                            "../src/trie.c"])],
)
