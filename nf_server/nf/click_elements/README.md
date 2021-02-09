# P4SFC Elements in NF

This directory contains the source codes of customized elements in P4SFC.
These elements should be built together with fastclick.

## Dependency

- Install [DPDK](https://www.dpdk.org/) (version 19.08.2 recommended).

- Clone FastClick to somewhere.

```bash
git clone https://github.com/tbarbette/fastclick.git 
```

- Link source code in FastClick

```bash
# *.hh and *.cc must be symbolically linked in <fastclick>/elements/local.
ln -s *.hh <fastclick>/elements/local/
ln -s *.cc <fastclick>/elements/local/
```

## Building command in FastClick

```bash
cd <fastclick>
autoconf
./configure --enable-dpdk --enable-intel-cpu --verbose --enable-select=poll CFLAGS="-O3" CXXFLAGS="-std=c++11 -O3"  --disable-dynamic-linking --enable-poll --enable-bound-port-transfer --enable-local --enable-flow --disable-task-stats --disable-cpu-load
# if memory is not sufficient (<=16GB): make -j6 or make -j4
make -j
sudo make install
```
