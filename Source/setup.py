from distutils.core import setup, Extension

module = Extension("Floof", sources=["Scripts.cpp"])
setup(
    name="Floof",
    version="1.0",
    description="Floof Scripting Package",
    ext_modules=[module]
    )

print("setup.py initalized")