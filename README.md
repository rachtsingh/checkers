tasks:
- add support for plotting indices to the existing render code
- does the existing render code read multi-step moves (i.e. including hops, END, etc.) as a single move?
- write a benchmark script for generating with Python implementation (no C++ extension) and with the C++ code (ython + Cext, not pure C++ binary)
- make sure the python visualization script is good and can read log files independently
- look for more optimizations - are there any obvious parts of the C++ code that are not good? how should I profile the "generate" script?
- set up the framework to write this in CUDA
