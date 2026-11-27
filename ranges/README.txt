K15 GTO preflop range library
=============================

One file per table format and stack depth:

  6max_100bb.txt  6max_200bb.txt  6max_300bb.txt
  9max_100bb.txt  9max_200bb.txt  9max_300bb.txt

They are solver output from K15 GTO's own preflop solver (Monte Carlo CFR
over the 169 hand classes, engine gto::mc), made with

  gto-cli preflop --format 6max --stack 100 --seconds 400 \
      --min-frequency 0.00001 --out ranges/6max_100bb.txt

(9-max: --seconds 440). The settings of each preset are written in the
file's "settings:" line and documented in engine/include/gto/mc/preflop.h.


How they were made (i9-14900K, 32 threads, 27 Nov 2026)
-------------------------------------------------------

  preset   decisions info sets memory time  iterations open (% of hands, UTG..SB)  expl.* scenarios
  6max100    9,772   1.65 M    30 MB  400 s 1.77 G     14 17 20 26 54               0.27%  113
  6max200   13,483   2.28 M    41 MB  400 s 2.02 G     14 17 20 27 54               0.51%  111
  6max300   13,874   2.34 M    43 MB  400 s 2.02 G     15 17 20 27 54               0.63%  114
  9max100   11,408   1.93 M    36 MB  440 s 0.43 G     11 11 12 14 16 20 26 54      0.46%  312
  9max200   11,408   1.93 M    36 MB  440 s 0.35 G     11 11 13 14 16 20 27 53      0.77%  318
  9max300   11,408   1.93 M    36 MB  440 s 0.35 G     11 12 13 15 17 20 27 53      1.18%  319

  * estimated exploitability, % of the starting pot (no Nash guarantee).
  Between half-time and the end every position's opening frequency moved by
  less than 0.5 points; single hand classes near indifference still flip
  (up to ~30% at 6-max, ~70% for one 9-max class), so read mixed cells as
  "either action".

Main approximation
------------------

There is no postflop play. When the preflop betting ends with two or more
players, the board is run out and the pot goes to the best hand: everybody
realises 100% of their equity. Real play under-realises equity out of
position and with weak, uncoordinated hands, so these ranges

  * defend the big blind (and call opens in general) wider than a full
    solution, down to weak offsuit hands that a postflop solver would fold,
  * value high cards over suited connectors and small pairs (playability
    after the flop is not rewarded),
  * open somewhat tighter from late position than full solutions, because
    the blinds defend so much.

Other simplifications: one open size per position (UTG..CO 2.5, BTN 2.3,
SB 3.0), one 3-bet size (in position 3x, out of position 4x the open at
100 BB; 3.25x/4.5x at 200 BB, 3.5x/5x at 300 BB), one 4-bet size (2.3x,
2.4x, 2.5x), 5-bets are all-in, no limping, no cold calls of 3-bets or
4-bets, and at 9-max at most one player may cold call an open (the big
blind may always call). The 5-bet being all-in makes deep 4-bet pots
somewhat jam-or-fold.

With six or nine players CFR has no Nash guarantee. The "exploitability_pct"
in each header is the engine's sampled estimate of the mean gain one player
could make by deviating alone, in % of the starting pot; read it as a
convergence indicator, not as a distance to an equilibrium.


File format (version 1)
-----------------------

Plain text, UTF-8. Lines starting with '#' are comments; blank lines
separate blocks. Keys are "key: value". Readers ignore keys they do not
know. The C++ reader/writer is gto::mc::read_preflop_library /
write_preflop_library (engine/src/mc/preflop.cpp).

First line:   k15-preflop-library 1

Header block:
  format: 6max                     table format
  players: 6
  stack_bb: 100                    every stack before posting, big blinds
  blinds: 0.5 1                    small blind, big blind
  ante: 0
  settings: ...                    free text: the sizes the tree used
  iterations: 1234567890           Monte Carlo iterations
  exploitability_pct: 0.31         estimate, see above
  open: UTG 0.142                  one line per position: share of all
                                   hands that raise first in

Scenario blocks (one per heads-up flop line; up to 4-bet pots):
  scenario: BTN:raise2.3/BB:call   positional path (see below)
  name: BTN open 2.3, BB call
  kind: srp                        limped | srp | 3bet | 4bet | 5bet+
  oop: BB                          out of position postflop
  ip: BTN
  aggressor: BTN                   last raiser (empty when limped)
  pot: 5.1                         at the flop, big blinds (dead blinds
                                   and antes included)
  effective_stack: 97.7            smaller stack behind at the flop
  frequency: 0.0612                share of deals that follow this line
  oop_range: AA:0.12,...           range text, PioSOLVER style weights
  ip_range: ...

Ranges are each player's reach on the line: the solver's average strategy
of the hand's class multiplied along the player's own actions (weights below
0.005 dropped, the rest rounded to 3 decimals). They are the ranges to put
into a postflop spot with the given pot and stack.

Positional paths: '/'-separated "POS:action" segments. A segment folds
everybody whose turn comes before POS, then applies the action. Actions:
fold, check, call, raiseX (raise to X big blinds, blinds included), allin.
Folds after the last other action are kept ("HJ:raise2.5/SB:call/BB:fold").
Positions: UTG, UTG1, UTG2, LJ, HJ, CO, BTN, SB, BB.
