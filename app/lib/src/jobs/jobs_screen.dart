import 'dart:async';

import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';

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
    'Iteration ${job.iteration} of ${job.settings.maxIterations}',
    if (job.measured)
      '${job.exploitabilityPct.toStringAsFixed(2)}% of pot exploitable',
    if (job.solveSeconds >= 0.05) '${job.solveSeconds.toStringAsFixed(1)} s',
  ];
  return parts.join(' · ');
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
      return const Center(
        child: Text('No solves yet. Set one up under "New solve".'),
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
            padding: const EdgeInsets.all(16),
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
    final state = job.state;
    final running = state == JobState.JOB_STATE_RUNNING;
    final waiting = state == JobState.JOB_STATE_QUEUED;
    final stopped =
        state == JobState.JOB_STATE_PAUSED ||
        state == JobState.JOB_STATE_CANCELLED;
    final failed = state == JobState.JOB_STATE_FAILED;

    final Color chipColor = switch (state) {
      JobState.JOB_STATE_RUNNING => theme.colorScheme.primaryContainer,
      JobState.JOB_STATE_DONE => theme.colorScheme.secondaryContainer,
      JobState.JOB_STATE_FAILED => theme.colorScheme.errorContainer,
      _ => theme.colorScheme.surfaceContainerHighest,
    };

    return Card(
      key: Key('job-${job.id}'),
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.fromLTRB(16, 12, 8, 12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Expanded(
                  child: Text(
                    job.name.isEmpty ? job.spot.board : job.name,
                    style: theme.textTheme.titleMedium,
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
                Container(
                  padding: const EdgeInsets.symmetric(
                    horizontal: 10,
                    vertical: 4,
                  ),
                  decoration: BoxDecoration(
                    color: chipColor,
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: Text(
                    jobStateLabel(state),
                    style: theme.textTheme.labelMedium,
                  ),
                ),
                const SizedBox(width: 4),
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
            ),
            Text(
              failed ? job.error : jobProgress(job),
              style: theme.textTheme.bodySmall?.copyWith(
                color: failed
                    ? theme.colorScheme.error
                    : theme.colorScheme.onSurfaceVariant,
              ),
            ),
            if (running) ...[
              const SizedBox(height: 8),
              Padding(
                padding: const EdgeInsets.only(right: 8),
                child: LinearProgressIndicator(
                  value: job.settings.maxIterations == 0
                      ? null
                      : job.iteration / job.settings.maxIterations,
                ),
              ),
            ],
            if (job.measured) ...[
              const SizedBox(height: 4),
              Text(
                'EV: OOP ${job.oopEv.toStringAsFixed(2)}, '
                'IP ${job.ipEv.toStringAsFixed(2)} · '
                '${formatBytes(job.memoryBytes.toInt())}',
                style: theme.textTheme.bodySmall,
              ),
            ],
            if (job.hasSolution)
              Align(
                alignment: Alignment.centerLeft,
                child: TextButton.icon(
                  key: Key('open-${job.id}'),
                  onPressed: onOpen,
                  icon: const Icon(Icons.grid_on, size: 18),
                  label: Text(running ? 'Browse so far' : 'Browse solution'),
                ),
              ),
          ],
        ),
      ),
    );
  }
}
