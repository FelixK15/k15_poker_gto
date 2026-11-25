import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../connection/server_connection.dart';
import '../jobs/jobs_screen.dart';
import '../range/range_store.dart';
import '../settings/theme_picker.dart';
import '../solution/solution_screen.dart';
import '../spot/spot_setup_screen.dart';

/// The app once connected: set up a solve, or look at the solves.
class HomeScreen extends ConsumerStatefulWidget {
  const HomeScreen({super.key, required this.api});

  final GtoApi api;

  @override
  ConsumerState<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends ConsumerState<HomeScreen> {
  static const _destinations = [
    (
      icon: Icons.add_box_outlined,
      selectedIcon: Icons.add_box,
      label: 'New solve',
    ),
    (
      icon: Icons.grid_view_outlined,
      selectedIcon: Icons.grid_view_rounded,
      label: 'Solves',
    ),
  ];

  int _index = 0;

  void _openSolution(Job job) {
    Navigator.of(context).push(
      MaterialPageRoute<void>(
        builder: (_) => SolutionScreen(api: widget.api, job: job),
      ),
    );
  }

  void _select(int index) => setState(() => _index = index);

  @override
  Widget build(BuildContext context) {
    final api = widget.api;
    final wide = MediaQuery.sizeOf(context).width >= 720;
    final theme = Theme.of(context);

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

    final actions = [
      _ServerStatus(api: api, showLabel: wide),
      const ThemeButton(),
      IconButton(
        key: const Key('disconnect-button'),
        tooltip: api.isLocal ? 'Stop the solver' : 'Disconnect',
        icon: const Icon(Icons.logout),
        onPressed: () => ref.read(connectionProvider.notifier).disconnect(),
      ),
      const SizedBox(width: 4),
    ];
    final appBar = AppBar(
      title: Text(_destinations[_index].label),
      actions: actions,
    );

    if (!wide) {
      return Scaffold(
        appBar: AppBar(
          title: Row(
            children: [
              const _Brand(size: 26),
              const SizedBox(width: 10),
              Text(_destinations[_index].label),
            ],
          ),
          actions: actions,
        ),
        body: page,
        bottomNavigationBar: NavigationBar(
          selectedIndex: _index,
          onDestinationSelected: _select,
          destinations: [
            for (final d in _destinations)
              NavigationDestination(
                icon: Icon(d.icon),
                selectedIcon: Icon(d.selectedIcon),
                label: d.label,
              ),
          ],
        ),
      );
    }

    return Scaffold(
      body: Row(
        children: [
          NavigationRail(
            selectedIndex: _index,
            onDestinationSelected: _select,
            labelType: NavigationRailLabelType.all,
            minWidth: 76,
            leading: const Padding(
              padding: EdgeInsets.only(top: 6, bottom: 18),
              child: _Brand(size: 34),
            ),
            destinations: [
              for (final d in _destinations)
                NavigationRailDestination(
                  icon: Icon(d.icon),
                  selectedIcon: Icon(d.selectedIcon),
                  label: Text(d.label),
                ),
            ],
          ),
          VerticalDivider(width: 1, color: theme.colorScheme.outlineVariant),
          Expanded(
            child: Scaffold(appBar: appBar, body: page),
          ),
        ],
      ),
    );
  }
}

/// The app's mark.
class _Brand extends StatelessWidget {
  const _Brand({required this.size});

  final double size;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Container(
      width: size,
      height: size,
      alignment: Alignment.center,
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [scheme.primary, scheme.tertiary],
        ),
        borderRadius: BorderRadius.circular(size * 0.25),
      ),
      child: Text(
        'K15',
        style: TextStyle(
          color: scheme.onPrimary,
          fontWeight: FontWeight.w800,
          fontSize: size * 0.34,
          letterSpacing: -0.5,
        ),
      ),
    );
  }
}

/// Which server the app talks to, with its size. Only a dot on phones; the
/// details are in its tooltip.
class _ServerStatus extends StatelessWidget {
  const _ServerStatus({required this.api, required this.showLabel});

  final GtoApi api;
  final bool showLabel;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final info = api.info;
    final text =
        '${api.label} · ${info.logicalCores} threads · '
        '${formatBytes(info.totalMemoryBytes.toInt())}';
    const online = Color(0xFF45A36B);
    return Tooltip(
      message: text,
      child: Container(
        margin: const EdgeInsets.symmetric(horizontal: 4),
        padding: EdgeInsets.symmetric(
          horizontal: showLabel ? 10 : 8,
          vertical: 5,
        ),
        decoration: BoxDecoration(
          color: theme.colorScheme.surfaceContainerHigh,
          borderRadius: BorderRadius.circular(20),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(
              api.isLocal ? Icons.computer : Icons.dns_outlined,
              size: 14,
              color: online,
            ),
            if (showLabel) ...[
              const SizedBox(width: 6),
              Text(
                text,
                key: const Key('server-label'),
                style: theme.textTheme.labelSmall?.copyWith(
                  color: theme.colorScheme.onSurfaceVariant,
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }
}
