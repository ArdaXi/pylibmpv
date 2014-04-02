from distutils.core import setup, Extension

module1 = Extension('mpv',
                    libraries = ['mpv'],
                    sources = ['mpvmodule.c'])

setup (name = "MPV",
       version = "0.1",
       description = "MPV",
       ext_modules = [module1])
