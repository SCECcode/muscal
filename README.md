# The Multi-scale Statewide CALifornia Velocity Model (muscal)

<a href="https://github.com/sceccode/muscal.git"><img src="https://github.com/sceccode/muscal/wiki/images/muscal_logo.png"></a>

[![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
![GitHub repo size](https://img.shields.io/github/repo-size/sceccode/muscal)
[![muscal-ucvm-ci Actions Status](https://github.com/SCECcode/muscal/workflows/muscal-ucvm-ci/badge.svg)](https://github.com/SCECcode/muscal/actions)

The Multi-Scale CALifornia (MUSCAL) statewide Vp and Vs velocity models provide 
high-quality integrated description of seismic structures across the state. 
Starting with the CANVAS base model (Doody et al., 2023), MUSCAL incorporates 
multiple regional and local velocity datasets into a unified structure, capturing 
features ranging from broad crustal-mantle structures to fine-scale local 
anomalies such as sedimentary basins.
 
To ensure quality, the merged multi-scale models underwent a data-informed refinement
process guided by simulations of small validation events. A key feature of MUSCAL is 
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
science (e.g. on what paper is the MUSCAL based?) should be directed
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

### muscal_query

