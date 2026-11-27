import 'package:fixnum/fixnum.dart';
import 'package:grpc/grpc.dart';
import 'package:k15_gto/src/api/gto_api.dart';

/// An in-memory server for widget tests. Records what the screens ask for.
class FakeApi implements GtoApi {
  FakeApi({this.label = 'solver.example:50051', this.isLocal = false});

  @override
  final String label;

  @override
  final bool isLocal;

  @override
  final GetInfoResponse info = GetInfoResponse(
    serverVersion: '0.1.0',
    engineVersion: '0.1.0',
    apiMajor: 1,
    apiMinor: 4,
    os: 'linux',
    logicalCores: 16,
    totalMemoryBytes: Int64(32) << 30,
    availableMemoryBytes: Int64(24) << 30,
  );

  final List<Job> jobs = [];
  final Map<String, GetNodeResponse> nodes = {};

  /// Nodes of jobs that have a parent, where they differ from [nodes].
  final Map<String, GetNodeResponse> childNodes = {};
  final List<String> nodeRequestsByJob = [];
  final List<String> calls = [];
  final List<Spot> estimated = [];
  final List<({Spot spot, SolveSettings settings, String name})> submitted = [];
  final List<String> nodeRequests = [];
  final List<({String parentId, List<NodeLock> locks, SolveSettings settings})>
  resolved = [];
  Object? resolveError;
  final Map<String, List<Runout>> runouts = {};

  @override
  Future<String>? lost;

  EstimateTreeResponse estimateResult = EstimateTreeResponse(
    nodes: Int64(15774),
    decisionNodes: Int64(5675),
    oopHands: 233,
    ipHands: 234,
    strategyBytes: Int64(27 << 20),
    treeBytes: Int64(1 << 20),
    canSolve: true,
    memoryBudgetBytes: Int64(24) << 30,
  );

  /// If set, the next matching call fails with it.
  Object? estimateError;
  Object? submitError;

  /// Set: submit answers with this job, marked as reused, as the server does
  /// for a spot it already solves.
  Job? reuseJob;
  Object? listError;
  bool closed = false;

  Job _job(String id) => jobs.firstWhere(
    (job) => job.id == id,
    orElse: () => throw const GrpcError.notFound('there is no such job'),
  );

  @override
  Future<EstimateTreeResponse> estimate(Spot spot) async {
    estimated.add(spot);
    if (estimateError != null) throw estimateError!;
    return estimateResult;
  }

  @override
  Future<Job> submit(Spot spot, SolveSettings settings, String name) async {
    if (submitError != null) throw submitError!;
    submitted.add((spot: spot, settings: settings, name: name));
    if (reuseJob != null) return reuseJob!.deepCopy()..reused = true;
    final job = Job(
      id: 'job${jobs.length + 1}',
      name: name,
      state: JobState.JOB_STATE_QUEUED,
      spot: spot,
      settings: settings,
    );
    jobs.insert(0, job);
    return job;
  }

  @override
  Future<List<Job>> listJobs() async {
    if (listError != null) throw listError!;
    return [for (final job in jobs) job.deepCopy()];
  }

  @override
  Future<Job> getJob(String id) async => _job(id).deepCopy();

  @override
  Future<Job> pause(String id) async {
    calls.add('pause $id');
    return _job(id)..state = JobState.JOB_STATE_PAUSED;
  }

  @override
  Future<Job> resume(String id) async {
    calls.add('resume $id');
    return _job(id)..state = JobState.JOB_STATE_QUEUED;
  }

  @override
  Future<Job> cancel(String id) async {
    calls.add('cancel $id');
    return _job(id)..state = JobState.JOB_STATE_CANCELLED;
  }

  @override
  Future<void> delete(String id) async {
    calls.add('delete $id');
    jobs.removeWhere((job) => job.id == id);
  }

  @override
  Stream<Job> watch(String id) => Stream.fromIterable([_job(id)]);

  /// max_hands and detail of every node request, in order.
  final List<({int maxHands, HandDetail detail})> nodeOptions = [];

  /// Preflop scenarios by job id, and ranges by "jobId:path".
  final Map<String, List<PreflopScenario>> preflopScenarios = {};
  final Map<String, GetPreflopRangesResponse> preflopRanges = {};

  @override
  Future<GetNodeResponse> getNode(
    String jobId,
    String path, {
    int maxHands = 0,
    HandDetail detail = HandDetail.HAND_DETAIL_HANDS,
  }) async {
    nodeOptions.add((maxHands: maxHands, detail: detail));
    nodeRequests.add(path);
    nodeRequestsByJob.add('$jobId:$path');
    final isChild = jobs.any(
      (job) => job.id == jobId && job.parentId.isNotEmpty,
    );
    final node = (isChild ? childNodes[path] : null) ?? nodes[path];
    if (node == null) {
      throw GrpcError.notFound("path '$path': no such line");
    }
    return node;
  }

  @override
  Future<List<PreflopScenario>> listPreflopScenarios(
    String jobId, {
    int maxRaises = 0,
  }) async {
    calls.add('scenarios $jobId');
    final result = preflopScenarios[jobId];
    if (result == null) {
      throw const GrpcError.failedPrecondition('not a preflop solve');
    }
    return result;
  }

  @override
  Future<GetPreflopRangesResponse> getPreflopRanges(
    String jobId,
    String path,
  ) async {
    calls.add('ranges $jobId:$path');
    final result = preflopRanges['$jobId:$path'];
    if (result == null) {
      throw GrpcError.invalidArgument("path '$path': no such line");
    }
    return result;
  }

  @override
  Future<List<Runout>> getRunouts(String jobId, String path) async {
    calls.add('runouts $path');
    final result = runouts[path];
    if (result == null) {
      throw GrpcError.invalidArgument("no card is dealt at '$path'");
    }
    return result;
  }

  @override
  Future<Job> resolve(
    String parentId,
    List<NodeLock> locks,
    SolveSettings settings,
  ) async {
    if (resolveError != null) throw resolveError!;
    resolved.add((parentId: parentId, locks: locks, settings: settings));
    final parent = _job(parentId);
    final job = Job(
      id: 'job${jobs.length + 1}',
      name: '${parent.name} (locked)',
      state: JobState.JOB_STATE_QUEUED,
      spot: parent.spot,
      settings: settings,
      parentId: parentId,
      locks: locks,
    );
    jobs.insert(0, job);
    return job;
  }

  @override
  Future<void> close() async {
    closed = true;
  }
}

/// The exact hands of a class such as "AA", "AKs" or "AKo", higher card first.
List<String> combosOf(String handClass) {
  const suits = 'shdc';
  final high = handClass[0];
  final low = handClass[1];
  final result = <String>[];
  for (var a = 0; a < 4; ++a) {
    for (var b = 0; b < 4; ++b) {
      if (high == low) {
        if (a < b) result.add('$high${suits[a]}$low${suits[b]}');
      } else if (handClass.endsWith('s')) {
        if (a == b) result.add('$high${suits[a]}$low${suits[b]}');
      } else if (a != b) {
        result.add('$high${suits[a]}$low${suits[b]}');
      }
    }
  }
  return result;
}

/// A range in which every hand of the given classes is fully present.
/// [ev] gives each class its value.
PlayerRange rangeOf(Map<String, double> classEv, {double reach = 1}) {
  final range = PlayerRange();
  var value = 0.0, weight = 0.0;
  classEv.forEach((handClass, ev) {
    for (final hand in combosOf(handClass)) {
      range.hands.add(hand);
      range.reach.add(reach);
      range.ev.add(ev);
      // Made up, but in step with the value: 0 to 1.
      range.equity.add((ev / 20).clamp(0.0, 1.0));
      value += ev * reach;
      weight += reach;
    }
  });
  range.rangeEv = weight > 0 ? value / weight : 0;
  range.rangeEquity = (range.rangeEv / 20).clamp(0.0, 1.0);
  return range;
}

/// An action node at which the acting player's classes follow [classStrategy]
/// (one frequency per action for each class).
GetNodeResponse actionNode({
  required String path,
  required Player player,
  required List<NodeAction> actions,
  required Map<String, List<double>> classStrategy,
  required Map<String, double> oop,
  required Map<String, double> ip,
  double pot = 10,
}) {
  final node = GetNodeResponse(
    type: NodeType.NODE_TYPE_ACTION,
    player: player,
    path: path,
    pot: pot,
    oop: rangeOf(oop),
    ip: rangeOf(ip),
    iteration: 180,
  );
  final actor = player == Player.PLAYER_OOP ? node.oop : node.ip;
  final hands = actor.hands.length;
  final strategy = List<double>.filled(actions.length * hands, 0);
  final totals = List<double>.filled(actions.length, 0);
  var h = 0;
  for (final handClass in (player == Player.PLAYER_OOP ? oop : ip).keys) {
    final frequencies = classStrategy[handClass]!;
    for (var i = 0; i < combosOf(handClass).length; ++i, ++h) {
      for (var a = 0; a < actions.length; ++a) {
        strategy[a * hands + h] = frequencies[a];
        totals[a] += frequencies[a];
      }
    }
  }
  node.strategy.addAll(strategy);
  for (var a = 0; a < actions.length; ++a) {
    node.actions.add(actions[a].deepCopy()..frequency = totals[a] / hands);
  }
  return node;
}

NodeAction check() =>
    NodeAction(type: ActionType.ACTION_TYPE_CHECK, label: 'check');
NodeAction fold() =>
    NodeAction(type: ActionType.ACTION_TYPE_FOLD, label: 'fold');
NodeAction call(double amount) => NodeAction(
  type: ActionType.ACTION_TYPE_CALL,
  amount: amount,
  label: 'call',
);
NodeAction bet(double amount, String label) =>
    NodeAction(type: ActionType.ACTION_TYPE_BET, amount: amount, label: label);
NodeAction raise(double amount, String label) => NodeAction(
  type: ActionType.ACTION_TYPE_RAISE,
  amount: amount,
  label: label,
);
NodeAction allin(double amount) => NodeAction(
  type: ActionType.ACTION_TYPE_ALLIN,
  amount: amount,
  label: 'allin',
);

const sampleOop = {
  'AA': 14.2,
  'KK': 12.8,
  'AKs': 9.4,
  'AKo': 9.1,
  'QQ': 6.0,
  'JTs': 3.2,
  '76s': 1.1,
  '55': 0.6,
};
const sampleIp = {
  'KK': 13.0,
  'TT': 7.5,
  'AQs': 5.2,
  'KQs': 6.8,
  'KQo': 6.1,
  'T9s': 2.4,
  '98s': 1.3,
  '22': 0.4,
};

/// A small solved turn spot: a few lines deep, enough to click through.
void addSampleSolution(FakeApi api) {
  api.nodes[''] = actionNode(
    path: '',
    player: Player.PLAYER_OOP,
    actions: [check(), bet(5, 'bet5'), bet(10, 'bet10')],
    classStrategy: const {
      'AA': [0.2, 0.3, 0.5],
      'KK': [0.1, 0.2, 0.7],
      'AKs': [0.4, 0.6, 0.0],
      'AKo': [0.5, 0.5, 0.0],
      'QQ': [0.9, 0.1, 0.0],
      'JTs': [0.3, 0.2, 0.5],
      '76s': [0.6, 0.0, 0.4],
      '55': [1.0, 0.0, 0.0],
    },
    oop: sampleOop,
    ip: sampleIp,
  );
  api.nodes['bet5'] = actionNode(
    path: 'bet5',
    player: Player.PLAYER_IP,
    actions: [fold(), call(5), raise(17, 'raise17'), allin(95)],
    classStrategy: const {
      'KK': [0.0, 0.3, 0.5, 0.2],
      'TT': [0.0, 0.9, 0.1, 0.0],
      'AQs': [0.1, 0.9, 0.0, 0.0],
      'KQs': [0.0, 0.8, 0.2, 0.0],
      'KQo': [0.0, 1.0, 0.0, 0.0],
      'T9s': [0.3, 0.5, 0.2, 0.0],
      '98s': [0.6, 0.2, 0.2, 0.0],
      '22': [1.0, 0.0, 0.0, 0.0],
    },
    oop: sampleOop,
    ip: sampleIp,
    pot: 15,
  );
  api.nodes['check'] = actionNode(
    path: 'check',
    player: Player.PLAYER_IP,
    actions: [check(), bet(5, 'bet5')],
    classStrategy: const {
      'KK': [0.2, 0.8],
      'TT': [0.7, 0.3],
      'AQs': [0.8, 0.2],
      'KQs': [0.4, 0.6],
      'KQo': [0.5, 0.5],
      'T9s': [0.6, 0.4],
      '98s': [0.5, 0.5],
      '22': [1.0, 0.0],
    },
    oop: sampleOop,
    ip: sampleIp,
  );
  api.nodes['check/check'] = GetNodeResponse(
    type: NodeType.NODE_TYPE_CHANCE,
    path: 'check/check',
    pot: 10,
    oop: rangeOf(sampleOop),
    ip: rangeOf(sampleIp),
    nextCards: [
      for (final suit in ['c', 'd', 'h', 's'])
        for (final rank in '23456789TJQA'.split('')) '$rank$suit',
    ],
  );
  api.nodes['check/check/2c'] = actionNode(
    path: 'check/check/2c',
    player: Player.PLAYER_OOP,
    actions: [check(), bet(7.5, 'bet7.5'), allin(95)],
    classStrategy: const {
      'AA': [0.3, 0.6, 0.1],
      'KK': [0.1, 0.6, 0.3],
      'AKs': [0.6, 0.4, 0.0],
      'AKo': [0.7, 0.3, 0.0],
      'QQ': [1.0, 0.0, 0.0],
      'JTs': [0.5, 0.3, 0.2],
      '76s': [0.7, 0.2, 0.1],
      '55': [1.0, 0.0, 0.0],
    },
    oop: sampleOop,
    ip: sampleIp,
  );
  api.nodes['bet5/fold'] = GetNodeResponse(
    type: NodeType.NODE_TYPE_FOLD,
    player: Player.PLAYER_IP,
    path: 'bet5/fold',
    pot: 15,
    oop: rangeOf(sampleOop),
    ip: rangeOf(sampleIp),
  );
}

Job sampleJob({
  String id = 'job1',
  JobState state = JobState.JOB_STATE_DONE,
  int iteration = 180,
  bool hasSolution = true,
}) => Job(
  id: id,
  name: 'Ks Td 7h 4c · pot 10',
  state: state,
  spot: Spot(board: 'Ks Td 7h 4c', pot: 10, effectiveStack: 95),
  settings: SolveSettings(maxIterations: 500, targetExploitabilityPct: 0.5),
  iteration: iteration,
  measured: iteration > 0,
  exploitabilityPct: 0.42,
  oopEv: 4.21,
  ipEv: 5.79,
  solveSeconds: 0.8,
  memoryBytes: Int64(28 << 20),
  hasSolution: hasSolution,
);
