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

const $core.bool _omitEnumNames =
    $core.bool.fromEnvironment('protobuf.omit_enum_names');
