TPrime
======

### What

TPrime is a postprocessing adjunct to SpikeGLX.

SpikeGLX is a recording system that acquires data from multiple asynchronous
data streams. To align these streams, during recording, one channel in each
stream records a (1Hz, 50% duty cycle) squarewave from a common generator.
In post processing. event times from stream A can be referenced to a nearby
rising edge. That same edge can be located in stream B.

TPrime takes tables of edge times from A and B, and tables of event times
from A, and outputs a new table of those event times relative to stream B:

**Tprime = T(B) = T(A) - T(Aedge) + T(Bedge)**

### Who

TPrime is developed by [Bill Karsh](https://www.janelia.org/people/bill-karsh)
of the [Tim Harris Lab](https://www.janelia.org/lab/harris-lab-apig) at
HHMI/Janelia Research Campus.

### Building in Windows

I build using Qt 5.12.0 (MinGW 64-bit). Compiled releases include the
app and supporting DLLs from that Qt version.

### Building in Linux

I build (under WSL2 on my Windows laptop) using ubuntu 16.04 and Qt
5.9.9 (MinGW 64-bit). Compiled releases include the app and supporting
DLLs from those versions of ubuntu and Qt, to make the package complete.
I build against old versions for (hopefully) better portability.

### Compiled Software

Official release software and support materials are here:
[**SpikeGLX Download Page**](http://billkarsh.github.io/SpikeGLX).

### Licensing

Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:
[http://license.janelia.org/license](http://license.janelia.org/license).


_fin_

