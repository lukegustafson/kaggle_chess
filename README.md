# kaggle_chess
This repo is the source code used to produce my entry to the FIDE & Google Efficient Chess AI Challenge hosted by Kaggle. The competition was to develop a chess engine fitting into a 64KB file and using no more than 5MB of RAM---these are very tight constraints, which made the competition interesting. Please note, this was a learning project for me, and not intended to be useful for any other purpose. In particular, it doesn't support UCI (the standard interface for chess engines), so it's not so easy to play with it outside the Kaggle competition.

The goal of this project was for me to learn about modern chess engines and experiment with training neural networks, particularly NNUEs (Efficiently Updatable Neural Network). As I am no chess engine expert, this is not a very strong engine (although much stronger than myself!). Nor is it portable: I only needed it to work on Kaggle and my machine.

There are actually five programs in this repo. Two are part of the Kaggle submission:
* `runner.py` is the Python wrapper code. Tersely written---I didn't want to waste any bytes on this script.
* `main.cpp` is the C++ chess engine

The other three programs are used for development, but not submitted to Kaggle:
* `trainer.cpp` trains the neural networks (note there is a lot of cut-and-paste here because of many experiments performed)
* `nnue_compressor.js` compresses the neural network (why JavaScript? Because I re-used this compression script from my [Wordle code golf](https://github.com/lukegustafson/golf-horse-zymic) project)
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
* Pondering
* Fits into a 64KB file and 5MB of RAM (not including RAM used by shared libraries already loaded in the Kaggle environment, particularly the C standard library)

I attempted to add some other search algorithm improvements, but they did not make the cut because of unclear benefits to playing strength: aspiration windows, futility pruning, singular extensions, and some extensions/reductions. As to why these were not performing well, I can only speculate because of the limited time I had to test and tune. My guess is that some combination of missing features (such as PVS rather than alpha-beta) and lack of tuning may have prevented these ideas from being effective, not to mention the possibility of bugs in my attempts.

## Building

This project is not intended to be portable (sorry!), and needs to be built with gcc on Linux. 

For the engine `main.cpp`, the script `go.sh` will compile the source, strip the binary, and compress it. It will create two files: `a`, the uncompressed program, and `b`, the compressed file. The other scripts (`training.cpp` and `playoff.cpp`) can be compiled by themselves; the command I used is in the first line of the scripts.

## Overview

The Kaggle competition was in the form of head-to-head matchups of game pairs using an opening book. The time control was 0.1 seconds/move (no increment) plus a reserve of 10 seconds. The system had 50-100ms of latency, so to avoid timeouts, the engine needed to move very quickly and rely on pondering (i.e. thinking outside its turn) for most of its searching. The memory limit seemed a bit unreliable: I believe my engine made no allocations after startup, but would still sometimes crash from out-of-memory in longer games (and many others reported the same issue). 

I started with [chess-library](https://github.com/Disservin/chess-library) for the fundamentals: board representation, game state, move generation, etc. The modifications I made included:
* Remove code not needed for my engine
* Remove most calls into the standard library (to minimize memory usage from paging in library code and allocations within libary functions)
* Remove all memory allocations (e.g. replace `vector` with arrays)
* Changed move generation for rooks. The original implementation used a single lookup table which required on the order of 1MB RAM. I replaced it with two lookup tables---one for horizontal movement and one for vertical--which are much smaller.

I wrote the search and evaluation code from scratch. For the most part, the search is based on standard techniques, which are listed in the "Features" section above. The evaluation is entirely based on a NNUE. A big part of the challenge was building a decent NNUE which fit into the 64KB constraint---I had about 40KB left for neural network data. My approach to the NNUE architecture was as follows:

* The only really feasible choice for the first layer is 768 inputs---one input for each piece-square combination. In fact, it's just 736 inputs because pawns cannot be on the 1st or 8th rank, but 768 is what I see used in the chess literature.
* For the remaining layers, my experimentation suggested that deeper networks were a beneficial tradeoff given that wider is not possible (due to memory constraints). I tried a number of combinations and settled on 64 -> 16 -> 8 for the middle layers.
* I had just enough memory to do 4 buckets for the middle layers---i.e., use different weights for these layers based on some function of the game state. It's a standard technique to bucket based on early vs late game (determined by values of non-pawn pieces). I also found it helpful to bucket based on the presence of queens. So my 4 buckets are early+queen, late+queen, early+no_queen, late+no_queen.
* I tried various activation functions and ReLU was best overall (gains from smoothed activation functions were tiny, but come with a performance penalty). However, because I used uncapped ReLU, I had to use int32 for calculations in my engine. This is in contrast to strong engines like Stockfish, which use capped ReLUs so that int16 is possible for calculations. I did not do any testing of capped ReLU + int16 evaluation, so that very well could've been a better approach.
* I experimented with sparse networks by using L1 regularization to force many weights to go to 0, with the idea that a sparse network could be more memory efficient. However, the idea proved wrong, as I found a sparse network with N non-zero weights was hardly better (as measured by the loss function) than a dense network with N weights.

This architecture resulted in about 54000 weights. I compressed them into ~40KB by using a combination of delta encoding, quantization, and arithmetic coding:

* Weights were often correlated for the same piece inputs, so I delta-encoded (i.e. only stored the difference from the previous) the weights for the same pieces.
* Then, I quantized to 1/64 absolute precision (i.e. `round(x * 64) / 64`). This level of precision resulted in no measurable loss in the evaluations, so it was very safe.
* Then, each weight was encoded as a unary representation of the length in bits (N 1's followed by a 0), followed by the bits themselves.
* Finally, the data was compressed with an arithmetic encoder, using pre-calculated probabilities for the unary bits and 50% probabilities for the remaining bits.

The unpacking code is in `nnue_compressed.cpp` and the compressor is in `nnue_compressor.js`.

I trained my NNUE on about 1 billion positions+evaluations from [https://huggingface.co/datasets/linrock/test80-2024/tree/main](https://huggingface.co/datasets/linrock/test80-2024/tree/main). It takes a couple hours to train on a single thread. I wrote the gradient descent from scratch because honestly, it took me less time than figuring out how to wire up PyTorch to do all the stuff I wanted to try. No regularization was needed as I had more data than parameters, so overfitting was not possible (although as mentioned, I experimented with L1 regularization to test sparse networks).

## Things I learned

Again, this was a learning project for me, so, here are interesting things I learned:

* Modern chess engines have a ton of heuristics and parameter tuning, and it's wrong to think of them as "brute force" or "just alpha-beta". Hundreds of Elo, if not a thousand, are coming from these heuristics and tuning. And it's a huge amount of work to implement all these heuristics and tunings.
* A well-trained neural network does not compress well. It seems that gradient descent is very effective at packing information into the weights.
* Initialization is incredibly important for training a multi-layer neural network. PyTorch makes this transparent, but it's important if you ever do training from scratch. "Xavier initialization" is the key technique. Otherwise gradient descent gets stuck in bad places.
* Though ReLU is a very simple activation function, a network of them still does a good job fitting complex functions.
* Even very small networks can somehow learn interesting positional features of chess. If I had unlimited time, I think it'd be interesting to look at some very small NNUEs and reverse engineer what information they are encoding and evaluating in the network.
