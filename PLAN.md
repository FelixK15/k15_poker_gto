# K15 GTO — Solver Plan

A GTO solver for No-Limit Hold'em in the style of PioSOLVER, GTO+ and GTO Wizard:
a headless C++ solver server (Windows and Linux) and a Flutter client (Windows and Android).

## 1. Decisions taken

| Topic | Decision |
|---|---|
| Solver scope v1 | Heads-up **postflop** solver (flop, turn, river) |
| 6-max / 9-max, 100 / 200 / 300BB | Covered by an editable **preflop range library** that pre-fills ranges, pot and stacks for the postflop solve |
| Node locking | **In v1**: fix a player's strategy at any node and re-solve around it |
| Engine language | **C++20** |
| Client | **Flutter** (one codebase, Windows + Android) |
| Local solving | **Windows only**. Android is always a remote client |
| Later | Preflop solving, multiway spots, PLO4 |

## 2. Architecture

```
┌──────────────────────────┐        gRPC (protobuf, HTTP/2)        ┌───────────────────────────┐
│ Flutter client           │  ───────────────────────────────────▶ │ gto-server (C++)          │
│  Windows / Android       │  ◀─── streamed progress + node data ─ │  Windows / Linux          │
│                          │                                        │                           │
│  startup: Local | Remote │                                        │  API layer (gRPC)         │
└──────────┬───────────────┘                                        │  Job manager + queue      │
           │ Local mode (Windows): client starts the                │  Solution store (disk)    │
           │ bundled gto-server.exe as a child process              │  libsolver (engine)       │
           └──────────────────────────────────────────────────────▶ └───────────────────────────┘
```

**One code path for local and remote.** In local mode the Windows client launches the bundled
`gto-server.exe` on `127.0.0.1` with a random port and a one-time token, then talks to it exactly
as it would to a remote server. No FFI, no second integration to maintain, and a crash in the
solver cannot take the GUI down.

**The server owns the solve.** Solves are long-running jobs with an id. The client can disconnect
(phone goes to sleep, laptop closes) and reattach later. Solutions stay on the server and are
browsed **node by node**; the client never downloads a multi-gigabyte tree.

### Components

| Component | Contents |
|---|---|
| `libsolver` (static lib) | Cards, hand evaluator, ranges, tree builder, isomorphism, CFR, best response, save/load. No networking, fully unit-testable |
| `gto-server` (exe) | gRPC API, job queue, resource limits, solution store, auth, logging |
| `gto-cli` (exe) | Runs a solve from a JSON config. Used for benchmarks, regression tests and scripting |
| `proto/` | The API contract. Generates C++ and Dart code |
| `app/` (Flutter) | The GUI |

## 3. Solver engine

### 3.1 Algorithm
- **Discounted CFR** (Brown & Sandholm 2019, α=1.5, β=0, γ=2) with alternating updates, computed
  as vectors over all hands in a range. This is the standard for this class of solver.
- **Showdown evaluation in O(n)** per node: hands sorted by strength, prefix sums, card-removal
  correction. This is the single most important optimisation; the naive version is O(n²).
- **Suit isomorphism** on turn and river cards: equivalent runouts are solved once and mapped back
  (about a quarter less work and memory on two-tone flops and more than half on monotone ones; nothing on rainbow flops, where no two suits are alike).
- **Exploitability** from a best-response pass, reported in % of pot. Solves stop at a target
  exploitability, an iteration limit, or on user request.

### 3.2 Game tree
Input for a solve:
- Board (3, 4 or 5 cards), OOP and IP ranges (1326 weights each), starting pot, effective stack.
- Bet sizes per street and per player: % of pot, raise multipliers, all-in, geometric sizing.
- Donk bets on/off, raise cap, all-in threshold, "force all-in when remaining stack is below x%".
- Rake (% and cap).

The tree builder reports **node count and estimated memory before the solve starts**, so the
server can reject or queue a job that does not fit.

### 3.3 Performance and memory
- Flat arrays for regrets and strategy sums, one block per node, no per-node heap allocation.
- 32-bit floats first; **16-bit compressed storage** as an option (halves memory, standard in
  commercial solvers).
- Multithreading by subtree (flop → one task per turn card). Thread pool, no locks on the hot path.
- AVX2 vector kernels with a scalar fallback, selected at runtime.
- 300BB trees grow fast because more raises fit before all-in. The raise cap, all-in threshold
  and the memory estimate are what keep these solvable; default tree presets per stack depth will
  be tuned for this.

### 3.4 Correctness
1. CFR core verified on Kuhn and Leduc poker, where exact equilibria are known.
2. River-only and turn-only spots checked against hand-derivable results (e.g. polarised range vs
   bluff-catchers).
3. Full flop spots cross-checked against open-source solvers (TexasSolver, b-inary
   postflop-solver) on identical inputs: EVs and frequencies must agree within exploitability.
4. Every solve reports exploitability, so a broken change is visible immediately.

Note: both reference solvers are **AGPL**. They are used for comparison and ideas only; no code
is copied, so the project keeps its licensing options open.

### 3.5 Node locking
The user fixes one player's strategy at a decision node; the solver then finds the best strategies
for everything else (the opponent's exploit and the locked player's adjustment elsewhere).

- **Granularity:** per hand. Each combo at the node is either locked to given action frequencies
  or left free, so "lock only my flush draws to always bet" is possible. Whole-range locks are
  the special case where every combo is locked.
- **Engine:** a locked node skips the regret update for its locked combos and always plays the
  fixed frequencies; its values still propagate up and down the tree. Any number of nodes can be
  locked, for either player. Locks are stored sparsely (only locked nodes cost memory).
- **Isomorphism:** where interchangeable cards are stored once below a locked node, the lock must not
  tell those suits apart, or the merged runouts would no longer be the same situation. The engine
  makes such a lock symmetric: every hand in a group of mirrored hands gets the group's average.
  Locks on whole hand classes are symmetric already. (The original idea of un-merging the
  affected runouts was dropped: it would change the tree and rule out a warm start.)
- **Re-solve:** a new job that copies the parent's state, keeps its regrets and iteration count, and
  rebuilds the average strategy from there (the parent's average belongs to another game). The
  parent is kept, so the two can be compared.
- **Exploitability with locks:** the best response for the locked player is computed with the
  locked nodes held fixed. The reported number therefore means "distance from equilibrium of the
  locked game", and the UI labels it that way.
- **Tests:** locking a node to its already-solved strategy must leave the solution unchanged;
  locking a river bluff-catcher to always fold must drive the opponent to bluff 100% there.

### 3.6 Built for the future features
These cost little now and are expensive to retrofit:
- Hand representation is generic over hole-card count (2 for NLH, 4 for PLO4). Ranges are
  "vector of weights over a combo index", not hard-coded to 1326.
- The tree and the CFR traversal take the player count as a parameter (2 today).
- Terminal evaluation sits behind an interface, so multiway showdowns and side pots can be added.
- Storage is addressed through an interface, so abstraction (bucketed hands) can be added for
  preflop and PLO.

## 4. Server

### 4.1 API (gRPC)
| Service | Calls |
|---|---|
| `Server` | `GetInfo` (version, cores, RAM, limits), `Ping` |
| `Trees` | `EstimateTree` (config → nodes, memory) |
| `Jobs` | `Submit`, `List`, `Get`, `Pause`, `Resume`, `Cancel`, `Watch` (stream: iteration, exploitability, ETA) |
| `Solutions` | `List`, `Delete`, `GetNode` (path → actions, per-hand strategy, EV, equity for both players), `GetRunoutReport`, `Export` |
| `Locks` | `Resolve` (parent job, locks, settings → new job). A solution's locks never change; they are listed on its `Job` |
| `Ranges` | Not built: the range library and saved ranges live in the client instead (see section 6), so they work the same against any server |

`GetNode` is addressed by the action path from the root (e.g. `bet33 / call / Ts / check`), which
keeps the client stateless and makes deep links and "copy line" trivial.

### 4.2 Operation
- **Auth:** bearer token. Remote servers use TLS; local mode binds to loopback only.
- **Resources:** configurable RAM and thread budget. One solve at a time by default, others queue.
- **Persistence:** solutions saved in a versioned binary format (zstd-compressed). The job list
  survives a restart.
- **Deployment:** single executable + config file; Windows zip/service and a Linux Docker image.

## 5. Client (Flutter)

### 5.1 Screens
1. **Connect** (startup): *Solve locally* (Windows only) or *Connect to server* (host, port,
   token; saved server list; shows server cores/RAM once connected).
2. **Spot setup:**
   - Game format: 6-max / 9-max, stack 100 / 200 / 300BB.
   - Preflop line picker (e.g. "BTN open, BB call" or "CO open, BTN 3-bet, CO call") → fills both
     ranges, pot and effective stack from the range library.
   - Board picker (with random and "common flops" shortcuts).
   - Tree settings with presets (simple / standard / detailed) and a live size estimate.
3. **Range editor:** 13×13 matrix, paint with weights, suit-specific combos, import/export in the
   PIO / GTO+ text format, save to library.
4. **Jobs:** queue, progress, exploitability curve, pause / resume / stop.
5. **Solution browser** (the core of the app):
   - Action line as a breadcrumb bar, plus a collapsible tree view.
   - 13×13 strategy matrix, each cell split by action frequency (the familiar look from the
     existing tools). Toggle: strategy / EV / equity / equity realisation.
   - Hand detail panel: per-combo frequencies and EV.
   - Range summary: overall action frequencies, range EV and equity for both players, hand-class
     breakdown (sets, top pair, draws …).
   - Turn/river card picker with a runout report (EV and bet frequency per card).
   - **Node locking:** "Lock this node" opens the strategy matrix in edit mode. Set frequencies
     for the whole range (sliders per action), for selected hands (paint like the range editor),
     or start from the solved strategy and adjust. Locked nodes show a lock icon in the tree and
     breadcrumb; a locks panel lists and clears them. "Re-solve" submits the job, and a compare
     view shows strategy and EV differences against the parent solution.
6. **Settings:** theme, action colours, default tree presets.

### 5.2 Layout and tech
- Desktop: multi-pane (tree | matrix | details). Phone: one pane at a time with tabs, same widgets.
- State: Riverpod. Networking: `grpc` Dart package with generated stubs. Matrix and charts drawn
  with `CustomPainter` for speed.
- Windows package bundles `gto-server.exe`; the Android package does not contain the engine.

## 6. Preflop range library

Structure: `format (6-max | 9-max) × stack (100 | 200 | 300BB) × scenario` where a scenario is a
preflop line between two players (single-raised, 3-bet, 4-bet pots per position pair). Each
scenario stores both ranges plus the resulting pot and effective stack. Stored as plain text files
so users can edit and share them.

**Where the default ranges come from.** Ranges from commercial products cannot be redistributed,
so the library generates its own in the client (`app/lib/src/range/preflop_library.dart`): hands
are ranked by the Chen formula, each seat opens the top slice of that ranking, and calling,
3-betting and 4-betting ranges are cut from it by rules of thumb, with the very top of the 4-bet
ranges spelled out per stack depth. They are **rough starting points, not solver output**, and the
app says so. Known weak spots: no mixed frequencies, opening ranges do not change with stack depth,
and the big blind's calling range is too thin in strong pairs. They are to be replaced by ranges
from our own preflop solver once it exists (section 8).

Saved ranges are kept by the client in a JSON file in its support directory, not on the server.

## 7. Repository and tooling

```
K15_GTO/
  proto/            API contract (.proto)
  engine/           libsolver (C++20) + unit tests + benchmarks
  server/           gto-server
  cli/              gto-cli
  app/              Flutter client
  ranges/           preflop range library (text)
  tests/reference/  spots with known results for regression
  docker/           Linux server image
  CMakeLists.txt, vcpkg.json
```

- Build: CMake + vcpkg (gRPC, protobuf, spdlog, zstd, GoogleTest, Google Benchmark).
- Compilers: MSVC on Windows, GCC/Clang on Linux.
- CI: build and test on both platforms, Flutter analyze/test, a benchmark job that tracks solve
  speed on fixed spots so performance regressions are caught.

## 8. Milestones

Durations are rough guesses for one focused developer, not commitments.

| # | Milestone | Done when | Rough size |
|---|---|---|---|
| M0 | Skeleton | **Done.** Repo builds on Windows; proto compiles to C++ and Dart. Linux build and CI not yet run | 1 week |
| M1 | Engine core | **Done.** Cards, evaluator, ranges, tree builder, DCFR solver with exploitability; river spots solve via `gto-cli`; Kuhn poker and a river spot with a known answer pass | 3–4 weeks |
| M2 | Full postflop | **Done, except the cross-check.** Chance nodes, so turn and flop spots solve; suit isomorphism; multithreading; Leduc poker reaches its known value; check-down EVs match brute-force equity. Still open: comparing full spots against TexasSolver / postflop-solver | 4–6 weeks |
| M3 | Server | **Done, except TLS and compression.** Jobs with a queue, pause, resume, cancel and delete; progress stream; solutions kept on disk and browsable node by node, also while solving; size estimate; access token. Still open: TLS (planned with the client in M4) and compressing saved solutions | 3 weeks |
| M4 | Client MVP | **Done on Windows, except TLS.** Connect screen that starts the solver on this computer or connects to a server with a token; spot setup with a live size estimate; jobs list with pause, resume, cancel, delete; solution browser with the strategy matrix, breadcrumb, card picker and per-hand details. Still open: TLS on both sides, and shipping the server next to the app (M7) | 4–5 weeks |
| M5 | Range library | **Done.** Preflop line picker for 6-max and 9-max at 100/200/300BB (single-raised, 3-bet and 4-bet pots) that fills in ranges, pot and stacks; range editor on the 13×13 grid with weights, text import (PioSOLVER and GTO+ forms) and saved ranges; tree presets; board picker. The ranges are rule-based approximations, see section 6. Not done: editing single suit combinations on the grid (they can be typed) | 2–3 weeks |
| M6 | Node locking | **Done.** Per-hand locks in the engine; a re-solve is a new job that starts from the parent's solution with the locks in force; lock dialog (whole range or one hand class), lock list, and a comparison with the parent in the client. Not done: painting different frequencies per hand in one go, and a side-by-side compare view | 3–4 weeks |
| M7 | Android + polish | Phone layouts, reconnect handling, runout reports, EV/equity views, installers, Docker image | 3–4 weeks |
| M8 | Performance | AVX2 kernels, 16-bit storage, tuning against benchmarks | ongoing |

The lock hook in the CFR traversal and the un-merge support in the tree builder are designed in
during M1–M2, so M6 adds the feature without reworking the engine.

**After v1**
- Aggregated reports across flop subsets.
- Preflop solver (Monte Carlo CFR with hand abstraction) → own range library.
- Multiway postflop. Caveat: CFR has no Nash guarantee with 3+ players; results are useful in
  practice but the "exploitability" number loses its strict meaning, and memory grows sharply.
- PLO4: 270,725 starting combos instead of 1,326, so it needs hand bucketing and is a large
  project of its own.

## 9. Main risks

| Risk | Mitigation |
|---|---|
| Solve speed far behind commercial tools | O(n) showdown and isomorphism from the start; benchmark job in CI; M8 dedicated to tuning |
| 200/300BB flop trees exceed RAM | Pre-solve estimate, raise cap and all-in threshold, 16-bit storage, tuned presets per depth |
| Subtle CFR bugs give plausible but wrong strategies | Exploitability on every solve plus cross-checks against two independent solvers |
| Default preflop ranges | Self-authored baseline + import now, own preflop solver later |
| gRPC C++ build weight on Windows | vcpkg with binary caching; API layer isolated so the transport can be swapped |
