import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../connection/server_connection.dart';
import '../jobs/jobs_screen.dart';
import '../range/range_store.dart';
import '../solution/solution_screen.dart';
import '../spot/spot_setup_screen.dart';

/// The app once connected: set up a solve, or look at the jobs.
class HomeScreen extends ConsumerStatefulWidget {
  const HomeScreen({super.key, required this.api});

  final GtoApi api;

  @override
  ConsumerState<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends ConsumerState<HomeScreen> {
  static const _destinations = [
    (icon: Icons.add_circle_outline, label: 'New solve'),
    (icon: Icons.list_alt, label: 'Jobs'),
  ];

  int _index = 0;

  void _openSolution(Job job) {
    Navigator.of(context).push(
      MaterialPageRoute<void>(
        builder: (_) => SolutionScreen(api: widget.api, job: job),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final api = widget.api;
    final info = api.info;
    final wide = MediaQuery.sizeOf(context).width >= 720;

    final page = IndexedStack(
      index: _index,
      children: [
        SpotSetupScreen(
          api: api,
          store: ref.watch(rangeStoreProvider),
          // Show the new job where its progress is visible.
          onSubmitted: (_) => setState(() => _index = 1),
        ),
        JobsScreen(api: api, onOpen: _openSolution, active: _index == 1),
      ],
    );

    return Scaffold(
      appBar: AppBar(
        title: const Text('K15 GTO'),
        actions: [
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8),
            child: Center(
              child: Text(
                '${api.label} · ${info.logicalCores} threads · '
                '${formatBytes(info.totalMemoryBytes.toInt())}',
                key: const Key('server-label'),
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ),
          ),
          IconButton(
            key: const Key('disconnect-button'),
            tooltip: api.isLocal ? 'Stop the solver' : 'Disconnect',
            icon: const Icon(Icons.logout),
            onPressed: () => ref.read(connectionProvider.notifier).disconnect(),
          ),
        ],
      ),
      body: wide
          ? Row(
              children: [
                NavigationRail(
                  selectedIndex: _index,
                  onDestinationSelected: (i) => setState(() => _index = i),
                  labelType: NavigationRailLabelType.all,
                  destinations: [
                    for (final d in _destinations)
                      NavigationRailDestination(
                        icon: Icon(d.icon),
                        label: Text(d.label),
                      ),
                  ],
                ),
                const VerticalDivider(width: 1),
                Expanded(child: page),
              ],
            )
          : page,
      bottomNavigationBar: wide
          ? null
          : NavigationBar(
              selectedIndex: _index,
              onDestinationSelected: (i) => setState(() => _index = i),
              destinations: [
                for (final d in _destinations)
                  NavigationDestination(icon: Icon(d.icon), label: d.label),
              ],
            ),
    );
  }
}
