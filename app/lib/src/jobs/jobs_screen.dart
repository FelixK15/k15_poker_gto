import 'dart:async';

import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../common/playing_card.dart';
import '../common/solve_labels.dart';

/// "Running", "Paused", … for a job state.
String jobStateLabel(JobState state) {
  switch (state) {
    case JobState.JOB_STATE_QUEUED:
      return 'Queued';
    case JobState.JOB_STATE_RUNNING:
      return 'Solving';
    case JobState.JOB_STATE_PAUSED:
      return 'Paused';
    case JobState.JOB_STATE_DONE:
      return 'Done';
    case JobState.JOB_STATE_CANCELLED:
      return 'Cancelled';
    case JobState.JOB_STATE_FAILED:
      return 'Failed';
    default:
      return 'Unknown';
  }
}

/// One line on where a job stands.
String jobProgress(Job job) {
  final parts = <String>[
    if (isMonteCarlo(job))
      '${formatSamples(job.mcIterations.toInt())} of '
          '${formatSamples(jobMcBudget(job))} samples'
    else
      'Iteration ${job.iteration} of ${job.settings.maxIterations}',
    if (job.measured)
      exploitabilityLabel(job.exploitabilityPct, job.exploitabilityKind),
    if (job.solveSeconds >= 0.05) '${job.solveSeconds.toStringAsFixed(1)} s',
  ];
  return parts.join(' · ');
}

/// The Monte Carlo iterations a job runs to.
int jobMcBudget(Job job) => job.settings.mcIterations > 0
    ? job.settings.mcIterations.toInt()
    : defaultMcIterations;

/// How far a running job is, 0 to 1, or null if that cannot be told.
double? jobFraction(Job job) {
  if (isMonteCarlo(job)) {
    return (job.mcIterations.toInt() / jobMcBudget(job)).clamp(0.0, 1.0);
  }
  return job.settings.maxIterations == 0
      ? null
      : job.iteration / job.settings.maxIterations;
}

/// All jobs on the server, kept up to date, with their controls.
class JobsScreen extends StatefulWidget {
  const JobsScreen({
    super.key,
    required this.api,
    required this.onOpen,
    this.active = true,
    this.refreshInterval = const Duration(seconds: 1),
  });

  final GtoApi api;

  /// Called when the user opens a job's solution.
  final ValueChanged<Job> onOpen;

  /// Whether the screen is visible; it only asks the server while it is.
  final bool active;
  final Duration refreshInterval;

  @override
  State<JobsScreen> createState() => _JobsScreenState();
}

class _JobsScreenState extends State<JobsScreen> {
  Timer? _timer;
  List<Job>? _jobs;
  String? _error;
  bool _loading = false;

  @override
  void initState() {
    super.initState();
    _updateTimer();
  }

  @override
  void didUpdateWidget(JobsScreen old) {
    super.didUpdateWidget(old);
    if (old.active != widget.active) _updateTimer();
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  void _updateTimer() {
    _timer?.cancel();
    _timer = null;
    if (!widget.active) return;
    _refresh();
    _timer = Timer.periodic(widget.refreshInterval, (_) => _refresh());
  }

  Future<void> _refresh() async {
    if (_loading) return;
    _loading = true;
    try {
      final jobs = await widget.api.listJobs();
      if (!mounted) return;
      setState(() {
        _jobs = jobs;
        _error = null;
      });
    } catch (error) {
      if (!mounted) return;
      setState(() => _error = describeError(error));
    } finally {
      _loading = false;
    }
  }

  Future<void> _act(Future<void> Function() action) async {
    try {
      await action();
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context)
          .showSnackBar(SnackBar(content: Text(describeError(error))));
    }
    await _refresh();
  }

  Future<void> _confirmDelete(Job job) async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Delete this solve?'),
        content: Text(
          '"${job.name}" and its solution are removed from the server. '
          'This cannot be undone.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context, false),
            child: const Text('Keep'),
          ),
          FilledButton(
            key: const Key('confirm-delete'),
            onPressed: () => Navigator.pop(context, true),
            child: const Text('Delete'),
          ),
        ],
      ),
    );
    if (confirmed == true) await _act(() => widget.api.delete(job.id));
  }

  @override
  Widget build(BuildContext context) {
    final jobs = _jobs;
    if (jobs == null) {
      // Nothing to animate while the screen is hidden behind another tab.
      if (!widget.active) return const SizedBox.shrink();
      return Center(
        child: _error == null
            ? const CircularProgressIndicator()
            : Text(_error!, key: const Key('jobs-error')),
      );
    }
    if (jobs.isEmpty) {
      final theme = Theme.of(context);
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(
                Icons.grid_view_rounded,
                size: 48,
                color: theme.colorScheme.onSurfaceVariant,
              ),
              const SizedBox(height: 12),
              Text(
                'No solves yet. Set one up under "New solve".',
                textAlign: TextAlign.center,
                style: theme.textTheme.bodyMedium?.copyWith(
                  color: theme.colorScheme.onSurfaceVariant,
                ),
              ),
            ],
          ),
        ),
      );
    }
    return Column(
      children: [
        if (_error != null)
          MaterialBanner(
            content: Text(_error!, key: const Key('jobs-error')),
            actions: [
              TextButton(onPressed: _refresh, child: const Text('Retry')),
            ],
          ),
        Expanded(
          child: ListView.builder(
            padding: const EdgeInsets.fromLTRB(12, 12, 12, 24),
            itemCount: jobs.length,
            itemBuilder: (context, index) => Center(
              child: ConstrainedBox(
                constraints: const BoxConstraints(maxWidth: 900),
                child: _JobCard(
                  job: jobs[index],
                  onOpen: () => widget.onOpen(jobs[index]),
                  onPause: () => _act(() => widget.api.pause(jobs[index].id)),
                  onResume: () => _act(() => widget.api.resume(jobs[index].id)),
                  onCancel: () => _act(() => widget.api.cancel(jobs[index].id)),
                  onDelete: () => _confirmDelete(jobs[index]),
                ),
              ),
            ),
          ),
        ),
      ],
    );
  }
}

class _JobCard extends StatelessWidget {
  const _JobCard({
    required this.job,
    required this.onOpen,
    required this.onPause,
    required this.onResume,
    required this.onCancel,
    required this.onDelete,
  });

  final Job job;
  final VoidCallback onOpen;
  final VoidCallback onPause;
  final VoidCallback onResume;
  final VoidCallback onCancel;
  final VoidCallback onDelete;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final scheme = theme.colorScheme;
    final state = job.state;
    final running = state == JobState.JOB_STATE_RUNNING;
    final waiting = state == JobState.JOB_STATE_QUEUED;
    final stopped =
        state == JobState.JOB_STATE_PAUSED ||
        state == JobState.JOB_STATE_CANCELLED;
    final failed = state == JobState.JOB_STATE_FAILED;
    final muted = theme.textTheme.bodySmall?.copyWith(
      color: scheme.onSurfaceVariant,
    );

    final Color stateColor = switch (state) {
      JobState.JOB_STATE_RUNNING => scheme.primary,
      JobState.JOB_STATE_DONE => const Color(0xFF45A36B),
      JobState.JOB_STATE_FAILED => scheme.error,
      JobState.JOB_STATE_QUEUED => const Color(0xFFE0B94F),
      _ => scheme.onSurfaceVariant,
    };
    final narrow = MediaQuery.sizeOf(context).width < 560;

    final controls = Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        if (running || waiting)
          IconButton(
            key: Key('pause-${job.id}'),
            tooltip: 'Pause',
            icon: const Icon(Icons.pause),
            onPressed: onPause,
          ),
        if (stopped)
          IconButton(
            key: Key('resume-${job.id}'),
            tooltip: 'Continue solving',
            icon: const Icon(Icons.play_arrow),
            onPressed: onResume,
          ),
        if (running || waiting)
          IconButton(
            key: Key('cancel-${job.id}'),
            tooltip: 'Stop for good',
            icon: const Icon(Icons.stop),
            onPressed: onCancel,
          ),
        IconButton(
          key: Key('delete-${job.id}'),
          tooltip: 'Delete',
          icon: const Icon(Icons.delete_outline),
          onPressed: onDelete,
        ),
      ],
    );

    final open = job.hasSolution
        ? FilledButton.tonalIcon(
            key: Key('open-${job.id}'),
            onPressed: onOpen,
            icon: const Icon(Icons.grid_on, size: 18),
            label: Text(running ? 'Browse so far' : 'Browse solution'),
          )
        : null;

    final board = splitCards(job.spot.board);
    return Card(
      key: Key('job-${job.id}'),
      margin: const EdgeInsets.only(bottom: 8),
      clipBehavior: Clip.antiAlias,
      child: InkWell(
        onTap: job.hasSolution ? onOpen : null,
        child: Padding(
          padding: const EdgeInsets.fromLTRB(14, 12, 8, 12),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  if (board.isNotEmpty) ...[
                    CardRow(board, width: narrow ? 22 : 26),
                    const SizedBox(width: 12),
                  ],
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          jobTitle(job),
                          style: theme.textTheme.titleSmall,
                          overflow: TextOverflow.ellipsis,
                        ),
                        const SizedBox(height: 2),
                        Row(
                          children: [
                            Container(
                              width: 7,
                              height: 7,
                              decoration: BoxDecoration(
                                color: stateColor,
                                shape: BoxShape.circle,
                              ),
                            ),
                            const SizedBox(width: 6),
                            Text(
                              jobStateLabel(state),
                              style: theme.textTheme.labelMedium?.copyWith(
                                color: stateColor,
                                fontWeight: FontWeight.w600,
                              ),
                            ),
                            const SizedBox(width: 8),
                            Flexible(
                              child: Text(
                                jobKind(job),
                                key: Key('kind-${job.id}'),
                                overflow: TextOverflow.ellipsis,
                                style: muted,
                              ),
                            ),
                          ],
                        ),
                      ],
                    ),
                  ),
                  if (!narrow && open != null) ...[
                    open,
                    const SizedBox(width: 4),
                  ],
                  controls,
                ],
              ),
              const SizedBox(height: 8),
              Tooltip(
                message: failed || !job.measured
                    ? ''
                    : exploitabilityMeaning(job.exploitabilityKind),
                child: Text(
                  failed ? job.error : jobProgress(job),
                  key: Key('progress-${job.id}'),
                  style: failed
                      ? theme.textTheme.bodySmall?.copyWith(color: scheme.error)
                      : muted,
                ),
              ),
              if (job.locks.isNotEmpty)
                Text(
                  'Re-solve with ${job.locks.length} locked '
                  '${job.locks.length == 1 ? 'decision' : 'decisions'}',
                  key: Key('locks-${job.id}'),
                  style: muted,
                ),
              if (job.measured)
                Text(
                  '${evLine(job)} · ${formatBytes(job.memoryBytes.toInt())}',
                  key: Key('ev-${job.id}'),
                  style: muted,
                ),
              if (running) ...[
                const SizedBox(height: 8),
                Padding(
                  padding: const EdgeInsets.only(right: 8),
                  child: ClipRRect(
                    borderRadius: BorderRadius.circular(3),
                    child: LinearProgressIndicator(
                      minHeight: 5,
                      value: jobFraction(job),
                    ),
                  ),
                ),
              ],
              if (narrow && open != null) ...[
                const SizedBox(height: 8),
                SizedBox(width: double.infinity, child: open),
              ],
            ],
          ),
        ),
      ),
    );
  }
}
