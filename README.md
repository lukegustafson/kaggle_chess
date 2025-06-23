# kaggle_chess
This repo is the source code used to produce my entry to the FIDE & Google Efficient Chess AI Challenge hosted by Kaggle. The competition was to develop a chess engine fitting into a 64KB file and using no more than 5MB of RAM---these are very tight constraints, which made the competition interesting. Please note, this was a learning project for me, and not intended to be useful for any other purpose. In particular, it doesn't support UCI (the standard interface for chess engines), so it's not so easy to play with it outside the Kaggle competition.

The goal of this project was for me to learn about modern chess engines and experiment with training neural networks, particularly NNUEs (Efficiently Updatable Neural Network). As I am no chess engine expert, this is not a very strong engine. Nor is it portable: I only needed it to work on Kaggle and my machine.

There are actually four programs in this repo. Two are part of the Kaggle submission:
* `runner.py` is the Python wrapper code
* `main.cpp` is the C++ chess engine

The other two programs are used for development, but not submitted to Kaggle:
* `trainer.cpp` trains the neural networks
* `playoff.cpp` runs a head-to-head battle between engines, used to test playing strength

## Features

* NNUE for position evaluation
  * 768 -> 64 -> 16 -> 8 architecture of ReLU neurons
  * 4 buckets for the 64 -> 16 -> 8 portion of the network (queen vs no queen, and early game vs end game)
* Alpha-beta search with fractional depth and quiescent search
* Move and capture history for move ordering
* Killer move heuristic
* Null move pruning
* Late move reduction
* Transposition table, configured to 3MB for my submission

I attempted to add some other search algorithm improvements, but they did not make the cut because of unclear benefits to playing strength: aspiration windows, futility pruning, singular extensions, and some extensions/reductions. As to why these were not performing well, I can only speculate because of the limited time I had to test and tune. My guess is that some combination of missing features (such as PVS rather than alpha-beta) and lack of tuning may have prevented these ideas from being effective, not to mention the possibility of bugs in my attempts.

## Building

This project is not intended to be portable (sorry!), and needs to be built with gcc on Linux. 

For the engine `main.cpp`, the script `go.sh` will compile the source, strip the binary, and compress it. It will create two files: `a`, the uncompressed program, and `b`, the compressed file. The other scripts (`training.cpp` and `playoff.cpp`) can be compiled by themselves; the command I used is in the first line of the scripts.

## Overview of engine architecture


## NNUE training

You will need to download a collection of positions to train on. I used test80-2024-04-apr-2tb7p.min-v2.binpack.zst from [https://huggingface.co/datasets/linrock/test80-2024/tree/main](https://huggingface.co/datasets/linrock/test80-2024/tree/main)---although I do not know if this is actually a good set of positions to use for our purposes.

