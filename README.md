# pegasus

## Source Builds and Development
pegasus uses CMake as a build configuration system. We recommend building
out-of-source. If you are not familiar with this terminology:

* **In-source build**: ``cmake`` is invoked directly from the ``cpp``
  directory. This can be inflexible when you wish to maintain multiple build
  environments (e.g. one for debug builds and another for release builds)
* **Out-of-source build**: ``cmake`` is invoked from another directory,
  creating an isolated build environment that does not interact with any other
  build environment. For example, you could create ``cpp/build`` and
  invoke ``cmake $CMAKE_ARGS ..`` from this directory

### Building:

```
git clone https://gitlab.devtools.intel.com/intel-bigdata/pegasus
cd pegasus/cpp
mkdir build
cd build
cmake ..
make
```
