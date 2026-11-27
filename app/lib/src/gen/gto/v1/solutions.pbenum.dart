// This is a generated file - do not edit.
//
// Generated from gto/v1/solutions.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports

import 'dart:core' as $core;

import 'package:protobuf/protobuf.dart' as $pb;

class HandDetail extends $pb.ProtobufEnum {
  /// PlayerRange.hands and the arrays parallel to it, as always.
  static const HandDetail HAND_DETAIL_HANDS =
      HandDetail._(0, _omitEnumNames ? '' : 'HAND_DETAIL_HANDS');

  /// Hands and PlayerRange.classes (Hold'em only).
  static const HandDetail HAND_DETAIL_HANDS_AND_CLASSES =
      HandDetail._(1, _omitEnumNames ? '' : 'HAND_DETAIL_HANDS_AND_CLASSES');

  /// Only PlayerRange.classes: hands, reach, ev, equity, strategy and locked
  /// stay empty. The compact choice for preflop nodes. Hold'em only:
  /// INVALID_ARGUMENT for PLO4.
  static const HandDetail HAND_DETAIL_CLASSES =
      HandDetail._(2, _omitEnumNames ? '' : 'HAND_DETAIL_CLASSES');

  static const $core.List<HandDetail> values = <HandDetail>[
    HAND_DETAIL_HANDS,
    HAND_DETAIL_HANDS_AND_CLASSES,
    HAND_DETAIL_CLASSES,
  ];

  static final $core.List<HandDetail?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 2);
  static HandDetail? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const HandDetail._(super.value, super.name);
}

class NodeType extends $pb.ProtobufEnum {
  static const NodeType NODE_TYPE_UNSPECIFIED =
      NodeType._(0, _omitEnumNames ? '' : 'NODE_TYPE_UNSPECIFIED');

  /// A player has to act.
  static const NodeType NODE_TYPE_ACTION =
      NodeType._(1, _omitEnumNames ? '' : 'NODE_TYPE_ACTION');

  /// The next card is dealt.
  static const NodeType NODE_TYPE_CHANCE =
      NodeType._(2, _omitEnumNames ? '' : 'NODE_TYPE_CHANCE');

  /// The hand ended with a fold.
  static const NodeType NODE_TYPE_FOLD =
      NodeType._(3, _omitEnumNames ? '' : 'NODE_TYPE_FOLD');

  /// The hand ended at showdown.
  static const NodeType NODE_TYPE_SHOWDOWN =
      NodeType._(4, _omitEnumNames ? '' : 'NODE_TYPE_SHOWDOWN');

  static const $core.List<NodeType> values = <NodeType>[
    NODE_TYPE_UNSPECIFIED,
    NODE_TYPE_ACTION,
    NODE_TYPE_CHANCE,
    NODE_TYPE_FOLD,
    NODE_TYPE_SHOWDOWN,
  ];

  static final $core.List<NodeType?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 4);
  static NodeType? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const NodeType._(super.value, super.name);
}

class Player extends $pb.ProtobufEnum {
  static const Player PLAYER_OOP =
      Player._(0, _omitEnumNames ? '' : 'PLAYER_OOP');
  static const Player PLAYER_IP =
      Player._(1, _omitEnumNames ? '' : 'PLAYER_IP');

  static const $core.List<Player> values = <Player>[
    PLAYER_OOP,
    PLAYER_IP,
  ];

  static final $core.List<Player?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 1);
  static Player? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const Player._(super.value, super.name);
}

class ActionType extends $pb.ProtobufEnum {
  static const ActionType ACTION_TYPE_UNSPECIFIED =
      ActionType._(0, _omitEnumNames ? '' : 'ACTION_TYPE_UNSPECIFIED');
  static const ActionType ACTION_TYPE_FOLD =
      ActionType._(1, _omitEnumNames ? '' : 'ACTION_TYPE_FOLD');
  static const ActionType ACTION_TYPE_CHECK =
      ActionType._(2, _omitEnumNames ? '' : 'ACTION_TYPE_CHECK');
  static const ActionType ACTION_TYPE_CALL =
      ActionType._(3, _omitEnumNames ? '' : 'ACTION_TYPE_CALL');
  static const ActionType ACTION_TYPE_BET =
      ActionType._(4, _omitEnumNames ? '' : 'ACTION_TYPE_BET');
  static const ActionType ACTION_TYPE_RAISE =
      ActionType._(5, _omitEnumNames ? '' : 'ACTION_TYPE_RAISE');
  static const ActionType ACTION_TYPE_ALLIN =
      ActionType._(6, _omitEnumNames ? '' : 'ACTION_TYPE_ALLIN');

  static const $core.List<ActionType> values = <ActionType>[
    ACTION_TYPE_UNSPECIFIED,
    ACTION_TYPE_FOLD,
    ACTION_TYPE_CHECK,
    ACTION_TYPE_CALL,
    ACTION_TYPE_BET,
    ACTION_TYPE_RAISE,
    ACTION_TYPE_ALLIN,
  ];

  static final $core.List<ActionType?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 6);
  static ActionType? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const ActionType._(super.value, super.name);
}

class PreflopScenarioKind extends $pb.ProtobufEnum {
  static const PreflopScenarioKind PREFLOP_SCENARIO_KIND_UNSPECIFIED =
      PreflopScenarioKind._(
          0, _omitEnumNames ? '' : 'PREFLOP_SCENARIO_KIND_UNSPECIFIED');

  /// Nobody raised: limps, or the small blind completed.
  static const PreflopScenarioKind PREFLOP_SCENARIO_KIND_LIMPED =
      PreflopScenarioKind._(
          1, _omitEnumNames ? '' : 'PREFLOP_SCENARIO_KIND_LIMPED');

  /// One raise, called.
  static const PreflopScenarioKind PREFLOP_SCENARIO_KIND_SINGLE_RAISED =
      PreflopScenarioKind._(
          2, _omitEnumNames ? '' : 'PREFLOP_SCENARIO_KIND_SINGLE_RAISED');
  static const PreflopScenarioKind PREFLOP_SCENARIO_KIND_THREE_BET =
      PreflopScenarioKind._(
          3, _omitEnumNames ? '' : 'PREFLOP_SCENARIO_KIND_THREE_BET');
  static const PreflopScenarioKind PREFLOP_SCENARIO_KIND_FOUR_BET =
      PreflopScenarioKind._(
          4, _omitEnumNames ? '' : 'PREFLOP_SCENARIO_KIND_FOUR_BET');

  /// Five raises or more.
  static const PreflopScenarioKind PREFLOP_SCENARIO_KIND_FIVE_BET_PLUS =
      PreflopScenarioKind._(
          5, _omitEnumNames ? '' : 'PREFLOP_SCENARIO_KIND_FIVE_BET_PLUS');

  static const $core.List<PreflopScenarioKind> values = <PreflopScenarioKind>[
    PREFLOP_SCENARIO_KIND_UNSPECIFIED,
    PREFLOP_SCENARIO_KIND_LIMPED,
    PREFLOP_SCENARIO_KIND_SINGLE_RAISED,
    PREFLOP_SCENARIO_KIND_THREE_BET,
    PREFLOP_SCENARIO_KIND_FOUR_BET,
    PREFLOP_SCENARIO_KIND_FIVE_BET_PLUS,
  ];

  static final $core.List<PreflopScenarioKind?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 5);
  static PreflopScenarioKind? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const PreflopScenarioKind._(super.value, super.name);
}

const $core.bool _omitEnumNames =
    $core.bool.fromEnvironment('protobuf.omit_enum_names');
