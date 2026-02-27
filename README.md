# The Multi-scale Statewide CALifornia Velocity Model (mscal)

<a href="https://github.com/sceccode/mscal.git"><img src="https://github.com/sceccode/mscal/wiki/images/mscal_logo.png"></a>

[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
![GitHub repo size](https://img.shields.io/github/repo-size/sceccode/mscal)
[![mscal-ucvm-ci Actions Status](https://github.com/SCECcode/mscal/workflows/mscal-ucvm-ci/badge.svg)](https://github.com/SCECcode/mscal/actions)

The Multi-Scale CALifornia (MSCAL) statewide Vp and Vs velocity models provide 
high-quality integrated description of seismic structures across the state. 
Starting with the CANVAS base model (Doody et al., 2023), MSCAL incorporates 
multiple regional and local velocity datasets into a unified structure, capturing 
features ranging from broad crustal-mantle structures to fine-scale local 
anomalies such as sedimentary basins.
 
To ensure quality, the merged multi-scale models underwent a data-informed refinement
process guided by simulations of small validation events. A key feature of MSCAL is 
the inclusion of a locally optimized near-surface low-velocity taper (LVT), specifically 
designed to better represent under-resolved shallow structures and improve the accuracy 
of ground-motion predictions.

## Installation

This package is intended to be installed as part of the UCVM framework,
version 25.7 or higher. 

## Contact the authors

If you would like to contact the authors regarding this software,
please e-mail software@scec.org. Note this e-mail address should
be used for questions regarding the software itself (e.g. how
do I link the library properly?). Questions regarding the model's
science (e.g. on what paper is the MSCAL based?) should be directed
to the model's authors, located in the AUTHORS file.

## To build in standalone mode

To install this package on your computer, please run the following commands:

<pre>
  aclocal
  autoconf
  automake --add-missing
  ./configure --prefix=/dir/to/install
  make
  make install
</pre>

### mscal_query

