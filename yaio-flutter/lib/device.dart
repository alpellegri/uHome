import 'dart:async';
import 'package:flutter/material.dart';
import 'package:firebase_messaging/firebase_messaging.dart';
import 'package:firebase_database/firebase_database.dart';
import 'drawer.dart';
import 'node_setup.dart';
import 'firebase_utils.dart';
import 'const.dart';
import 'entries.dart';

class Device extends StatefulWidget {
  Device({Key key, this.title}) : super(key: key);

  static const String routeName = '/setup';

  final String title;

  @override
  _DeviceState createState() => new _DeviceState();
}

class _DeviceState extends State<Device> {
  final FirebaseMessaging _fbMessaging = new FirebaseMessaging();
  bool _connected = false;

  @override
  void initState() {
    super.initState();
    print('_MyHomePageState');
    _connected = false;
    signInWithGoogle().then((onValue) {
      _fbMessaging.configure(
        onMessage: (Map<String, dynamic> message) {
          print("onMessage: $message");
          // _showItemDialog(message);
        },
        onLaunch: (Map<String, dynamic> message) {
          print("onLaunch: $message");
          // _navigateToItemDetail(message);
        },
        onResume: (Map<String, dynamic> message) {
          print("onResume: $message");
          // _navigateToItemDetail(message);
        },
      );

      _fbMessaging.requestNotificationPermissions(
          const IosNotificationSettings(sound: true, badge: true, alert: true));
      _fbMessaging.onIosSettingsRegistered
          .listen((IosNotificationSettings settings) {
        print('Settings registered: $settings');
      });
      _fbMessaging.getToken().then((String token) {
        assert(token != null);
        setFbToken(token);
        setState(() {
          _connected = true;
        });
      });
    });
  }

  @override
  Widget build(BuildContext context) {
    if (_connected == false) {
      return new Scaffold(
          drawer: drawer,
          appBar: new AppBar(
            title: new Text(widget.title),
          ),
          body: new LinearProgressIndicator(value: null));
    } else {
      return new Scaffold(
        drawer: drawer,
        appBar: new AppBar(
          title: new Text(widget.title),
        ),
        body: new ExpasionPanelsDemo(),
      );
    }
  }
}

class ListItem extends StatelessWidget {
  final String value;
  final FormFieldState<String> field;

  ListItem(this.value, this.field);

  @override
  Widget build(BuildContext context) {
    return new Row(mainAxisSize: MainAxisSize.min, children: <Widget>[
      new Radio<String>(
        value: value,
        groupValue: field.value,
        onChanged: field.didChange,
      ),
      new Text(value),
    ]);
  }
}

typedef Widget DemoItemBodyBuilder<T>(DemoItem<T> item);
typedef String ValueToString<T>(T value);

class DualHeaderWithHint extends StatelessWidget {
  const DualHeaderWithHint({this.name, this.value, this.hint, this.showHint});

  final String name;
  final String value;
  final String hint;
  final bool showHint;

  Widget _crossFade(Widget first, Widget second, bool isExpanded) {
    return new AnimatedCrossFade(
      firstChild: first,
      secondChild: second,
      firstCurve: const Interval(0.0, 0.75, curve: Curves.fastOutSlowIn),
      secondCurve: const Interval(0.25, 1.0, curve: Curves.fastOutSlowIn),
      sizeCurve: Curves.fastOutSlowIn,
      crossFadeState:
          isExpanded ? CrossFadeState.showSecond : CrossFadeState.showFirst,
      duration: const Duration(milliseconds: 500),
    );
  }

  @override
  Widget build(BuildContext context) {
    final ThemeData theme = Theme.of(context);
    final TextTheme textTheme = theme.textTheme;

    return new Row(children: <Widget>[
      new Expanded(
        child: new Container(
          margin: const EdgeInsets.only(left: 24.0),
          child: new FittedBox(
            fit: BoxFit.scaleDown,
            alignment: Alignment.centerLeft,
            child: new Text(
              name,
              // style: textTheme.body1.copyWith(fontSize: 15.0),
            ),
          ),
        ),
      ),
      new Container(
          margin: const EdgeInsets.only(left: 24.0),
          child: _crossFade(
              new Text(
                value,
                // style: textTheme.caption.copyWith(fontSize: 15.0),
              ),
              new Text(
                hint,
                // style: textTheme.caption.copyWith(fontSize: 15.0),
              ),
              showHint))
    ]);
  }
}

class CollapsibleBody extends StatelessWidget {
  const CollapsibleBody({
    this.margin: EdgeInsets.zero,
    this.child,
    this.isEditMode,
    this.onSelect,
    this.onCancel,
    this.onAdd,
    this.onRemove,
  });

  final EdgeInsets margin;
  final Widget child;
  final bool isEditMode;
  final VoidCallback onSelect;
  final VoidCallback onCancel;
  final VoidCallback onAdd;
  final VoidCallback onRemove;

  @override
  Widget build(BuildContext context) {
    final ThemeData theme = Theme.of(context);
    final TextTheme textTheme = theme.textTheme;

    var widget;
    if (isEditMode == false) {
      widget = new ButtonTheme.bar(
          child: new ButtonBar(children: <Widget>[
        new FlatButton(
            onPressed: onRemove,
            child: const Text(
              'REMOVE',
            )),
        new FlatButton(
            onPressed: onAdd,
            child: const Text(
              'ADD',
            )),
        new FlatButton(
            onPressed: onCancel,
            child: const Text(
              'CANCEL',
            )),
        new FlatButton(
            onPressed: onSelect,
            // textTheme: ButtonTextTheme.accent,
            child: const Text('SELECT')),
      ]));
    } else {
      widget = new ButtonTheme.bar(
          child: new ButtonBar(children: <Widget>[
        new FlatButton(
            onPressed: onCancel,
            child: const Text(
              'CANCEL',
            )),
        new FlatButton(
            onPressed: onSelect,
            // textTheme: ButtonTextTheme.accent,
            child: const Text('SAVE')),
      ]));
    }

    return new Column(children: <Widget>[
      new Container(
          margin: const EdgeInsets.only(left: 8.0, right: 8.0, bottom: 0.0) -
              margin,
          child: child),
      widget,
    ]);
  }
}

class DemoItem<T> {
  DemoItem({
    this.name,
    this.value,
    this.hint,
    this.builder,
    this.valueToString,
  }) : textController = new TextEditingController(text: valueToString(value));

  List query = new List();
  final String name;
  final String hint;
  final TextEditingController textController;
  final DemoItemBodyBuilder<T> builder;
  final ValueToString<T> valueToString;
  T value;
  bool isExpanded = false;
  bool isEditMode = false;

  ExpansionPanelHeaderBuilder get headerBuilder {
    return (BuildContext context, bool isExpanded) {
      return new DualHeaderWithHint(
          name: name,
          value: valueToString(value),
          hint: hint,
          showHint: isExpanded);
    };
  }

  Widget build() => builder(this);
}

class ExpasionPanelsDemo extends StatefulWidget {
  @override
  _ExpansionPanelsDemoState createState() => new _ExpansionPanelsDemoState();
}

class _ExpansionPanelsDemoState extends State<ExpasionPanelsDemo> {
  DatabaseReference _fcmRef;
  bool _nodeNeedUpdate = false;

  DatabaseReference _rootRef;
  StreamSubscription<Event> _onAddSubscription;
  StreamSubscription<Event> _onEditedSubscription;
  StreamSubscription<Event> _onRemoveSubscription;
  List<DemoItem<dynamic>> _demoItems;
  Map<String, dynamic> entryMap = new Map<String, dynamic>();
  bool _isPreferencesReady = false;
  String _ctrlDomainName = '';
  String _ctrlNodeName = '';
  bool _isNeedCreate = true;

  static const time_limit = const Duration(seconds: 20);
  Map<dynamic, dynamic> _control;
  Map<dynamic, dynamic> _status;
  Map<dynamic, dynamic> _startup;
  DatabaseReference _controlRef;
  DatabaseReference _statusRef;
  DatabaseReference _startupRef;
  bool _connected = false;
  int _controlTimeoutCnt;
  StreamSubscription<Event> _controlSub;
  StreamSubscription<Event> _statusSub;
  StreamSubscription<Event> _startupSub;

  List<IoEntry> entryList = new List();

  @override
  void initState() {
    super.initState();
    loadPreferences().then((map) {
      setState(() {
        _isPreferencesReady = true;
      });

      print('getRootRef: ${getRootRef()}');
      _rootRef = FirebaseDatabase.instance.reference().child(getRootRef());
      _onAddSubscription = _rootRef.onChildAdded.listen(_onRootEntryAdded);
      _onEditedSubscription =
          _rootRef.onChildChanged.listen(_onRootEntryChanged);
      _onRemoveSubscription =
          _rootRef.onChildRemoved.listen(_onRootEntryRemoved);
      if (map.isNotEmpty) {
        setState(() {
          _ctrlDomainName = map['domain'];
          _ctrlNodeName = map['nodename'];
        });
      }

      _fcmRef = FirebaseDatabase.instance.reference().child(getFcmTokenRef());
      _fcmRef.once().then((DataSnapshot onValue) {
        print('once: ${onValue.value}');
        Map map = onValue.value;
        bool tokenFound = false;
        String token = getFbToken();
        if (map != null) {
          map.forEach((key, value) {
            if (value == token) {
              print('key test: $key');
              tokenFound = true;
            }
          });
        }
        if (tokenFound == false) {
          _nodeNeedUpdate = true;
          _fcmRef.push().set(token);
          print('token saved: $token');
        }

        // at the end, not before
        // FirebaseDatabase.instance.setPersistenceEnabled(true);
        // FirebaseDatabase.instance.setPersistenceCacheSizeBytes(10000000);
      });

      _loadNodeInfo();

      _demoItems = <DemoItem<dynamic>>[
        new DemoItem<String>(
            name: 'Domain',
            value: _ctrlDomainName,
            hint: 'Select domain',
            valueToString: (String location) => location,
            builder: (DemoItem<String> item) {
              void close() {
                setState(() {
                  item.isExpanded = false;
                  item.isEditMode = false;
                });
              }

              void add() {
                setState(() {
                  item.isEditMode = true;
                });
              }

              return new Form(
                  child: new Builder(builder: (BuildContext context) {
                return new CollapsibleBody(
                  onSelect: () {
                    Form.of(context).save();
                    _ctrlDomainName = item.value;
                    close();
                  },
                  onCancel: () {
                    Form.of(context).reset();
                    close();
                  },
                  onAdd: () {
                    add();
                  },
                  onRemove: () {
                    if (_isNeedCreate == false) {
                      DatabaseReference ref;
                      ref = FirebaseDatabase.instance
                          .reference()
                          .child(getRootRef());
                      ref.child(item.value).remove();
                    }
                    setState(() {
                      _ctrlDomainName = '';
                      _ctrlNodeName = '';
                    });
                    close();
                  },
                  isEditMode: item.isEditMode,
                  child: (item.isEditMode == true)
                      ? (new TextFormField(
                          controller: item.textController,
                          decoration: new InputDecoration(
                            hintText: item.hint,
                            labelText: item.name,
                          ),
                          onSaved: (String value) {
                            item.value = value;
                          },
                        ))
                      : (new FormField<String>(
                          initialValue: item.value,
                          onSaved: (String result) {
                            item.value = result;
                          },
                          builder: (FormFieldState<String> field) {
                            return new ListView.builder(
                              shrinkWrap: true,
                              reverse: true,
                              itemCount: item.query.length,
                              itemBuilder: (buildContext, index) {
                                return new InkWell(
                                  child: new ListItem(item.query[index], field),
                                );
                              },
                            );
                          })),
                );
              }));
            }),
        new DemoItem<String>(
            name: 'Device',
            value: _ctrlNodeName,
            hint: 'Select Device',
            valueToString: (String location) => location,
            builder: (DemoItem<String> item) {
              void close() {
                setState(() {
                  item.isExpanded = false;
                  item.isEditMode = false;
                });
              }

              void add() {
                setState(() {
                  item.isEditMode = true;
                });
              }

              return new Form(
                  child: new Builder(builder: (BuildContext context) {
                return new CollapsibleBody(
                  onSelect: () {
                    Form.of(context).save();
                    _ctrlNodeName = item.value;
                    _changePreferences();
                    close();
                  },
                  onCancel: () {
                    Form.of(context).reset();
                    close();
                  },
                  onAdd: () {
                    add();
                  },
                  onRemove: () {
                    if (_isNeedCreate == false) {
                      DatabaseReference ref;
                      ref = FirebaseDatabase.instance
                          .reference()
                          .child(getRootRef())
                          .child(_ctrlDomainName);
                      ref.child(item.value).remove();
                    }
                    setState(() {
                      _ctrlNodeName = '';
                    });
                    close();
                  },
                  isEditMode: item.isEditMode,
                  child: (item.isEditMode == true)
                      ? (new TextFormField(
                          controller: item.textController,
                          decoration: new InputDecoration(
                            hintText: item.hint,
                            labelText: item.name,
                          ),
                          onSaved: (String value) {
                            item.value = value;
                          },
                        ))
                      : (new FormField<String>(
                          initialValue: item.value,
                          onSaved: (String result) {
                            item.value = result;
                          },
                          builder: (FormFieldState<String> field) {
                            return new ListView.builder(
                              shrinkWrap: true,
                              reverse: true,
                              itemCount: item.query.length,
                              itemBuilder: (buildContext, index) {
                                return new InkWell(
                                  child: new ListItem(item.query[index], field),
                                );
                              },
                            );
                          })),
                );
              }));
            }),
      ];
    });
  }

  @override
  void dispose() {
    super.dispose();
    _onAddSubscription.cancel();
    _onEditedSubscription.cancel();
    _onRemoveSubscription.cancel();
    _controlSub.cancel();
    _statusSub.cancel();
    _startupSub.cancel();
  }

  @override
  Widget build(BuildContext context) {
    if (_isPreferencesReady == false) {
      return new LinearProgressIndicator(value: null);
    } else {
      var update = _updateItemMenu();
      setState(() {
        _isNeedCreate = update;
      });
      print('_isNeedCreate $_isNeedCreate');
      DateTime _startupTime;
      String diffTime;
      Duration diff;
      if (_connected == true) {
        DateTime current = new DateTime.now();
        _startupTime = new DateTime.fromMillisecondsSinceEpoch(
            int.parse(_startup['time'].toString()) * 1000);
        DateTime _heartbeatTime = new DateTime.fromMillisecondsSinceEpoch(
            int.parse(_status['time'].toString()) * 1000);
        diff = current.difference(_heartbeatTime);
        if (diff.inDays > 0) {
          diffTime = '${diff.inDays} days';
        } else if (diff.inHours > 0) {
          diffTime = '${diff.inHours} hours';
        } else if (diff.inMinutes > 0) {
          diffTime = '${diff.inMinutes} minutes';
        } else if (diff.inSeconds > 0) {
          diffTime = '${diff.inSeconds} seconds';
        }
      }
      return new ListView(children: <Widget>[
        new SingleChildScrollView(
          child: new SafeArea(
            top: false,
            bottom: false,
            child: new Card(
              margin: const EdgeInsets.all(4.0),
              child: new ExpansionPanelList(
                  expansionCallback: (int index, bool isExpanded) {
                    setState(() {
                      _demoItems[index].isExpanded = !isExpanded;
                    });
                  },
                  children: _demoItems.map((DemoItem<dynamic> item) {
                    return new ExpansionPanel(
                        isExpanded: item.isExpanded,
                        headerBuilder: item.headerBuilder,
                        body: item.build());
                  }).toList()),
            ),
          ),
        ),
        new Card(
            child: new Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                mainAxisSize: MainAxisSize.min,
                children: <Widget>[
              new ListTile(
                leading: (_isNeedCreate == true)
                    ? (const Icon(Icons.link_off))
                    : (const Icon(Icons.link)),
                title: const Text('Selected Device'),
                subtitle: new Text('$_ctrlDomainName/$_ctrlNodeName'),
                trailing: new OutlineButton(
                  child: const Text('CONFIGURE'),
                  onPressed: (_isNeedCreate == true)
                      ? null
                      : () {
                          Navigator.of(context).pushNamed(NodeSetup.routeName);
                        },
                ),
              ),
            ])),
        (_connected == false)
            ? (const Text(''))
            : (new Card(
                child: new Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisSize: MainAxisSize.min,
                  children: <Widget>[
                    new ListTile(
                      leading: (diff > time_limit)
                          ? (new Icon(Icons.cloud, color: Colors.red[200]))
                          : (new Icon(Icons.cloud_done,
                              color: Colors.green[200])),
                      title: new Text('HeartBeat: $diffTime ago'),
                      subtitle: new Text('Device Memory: ${_status["heap"]}'),
                    ),
                    new ListTile(
                      leading: (_control['reboot'] == kNodeUpdate)
                          ? (new CircularProgressIndicator(
                              value: null,
                            ))
                          : (const Icon(Icons.update)),
                      title: const Text('Update Device'),
                      subtitle: new Text('Configuration'),
                      trailing: new OutlineButton(
                        child: const Text('UPDATE'),
                        onPressed: () {
                          _nodeActionRequest(kNodeUpdate);
                        },
                      ),
                    ),
                    new ListTile(
                      leading: (_control['reboot'] == kNodeReboot)
                          ? (new CircularProgressIndicator(
                              value: null,
                            ))
                          : (const Icon(Icons.power_settings_new)),
                      title: const Text('PowerUp'),
                      subtitle: new Text('${_startupTime.toString()}'),
                      trailing: new OutlineButton(
                        child: const Text('RESTART'),
                        onPressed: () {
                          _nodeActionRequest(kNodeReboot);
                        },
                      ),
                    ),
                    new ListTile(
                      leading: (_control['reboot'] == kNodeFlash)
                          ? (new CircularProgressIndicator(
                              value: null,
                            ))
                          : (const Icon(Icons.system_update_alt)),
                      title: const Text('Firmware Version'),
                      subtitle: new Text('${_startup["version"]}'),
                      trailing: new OutlineButton(
                        child: const Text('UPGRADE'),
                        onPressed: () {
                          _nodeActionRequest(kNodeFlash);
                        },
                      ),
                    ),
                    new ListTile(
                      leading: (_control['reboot'] == kNodeErase)
                          ? (new CircularProgressIndicator(
                              value: null,
                            ))
                          : (const Icon(Icons.delete_forever)),
                      title: const Text('Erase device'),
                      subtitle: new Text('${getOwner()}'),
                      trailing: new OutlineButton(
                        child: const Text('ERASE'),
                        onPressed: () {
                          _nodeActionRequest(kNodeFlash);
                        },
                      ),
                    ),
                  ],
                ),
              )),
      ]);
    }
  }

  bool _updateItemMenu() {
    bool ret = true;
    var keyList = entryMap.keys.toList();
    setState(() {
      _demoItems[0].query = keyList;
    });
    var keyValue = _demoItems[0].value;
    if (keyList.contains(keyValue)) {
      var keyList2 = entryMap[keyValue].keys.toList();
      setState(() {
        _demoItems[1].query = keyList2;
      });
      var keyValue2 = _demoItems[1].value;
      ret = !keyList2.contains(keyValue2);
    }
    return ret;
  }

  void _nodeUpdate(String source) {
    DatabaseReference dataRef;
    String root = getRootRef();
    String dataSource = '$root/$source/control';
    print(dataSource);
    dataRef = FirebaseDatabase.instance.reference().child('$dataSource/reboot');
    dataRef.set(kNodeUpdate);
    DateTime now = new DateTime.now();
    dataRef = FirebaseDatabase.instance.reference().child('$dataSource/time');
    dataRef.set(now.millisecondsSinceEpoch ~/ 1000);
  }

  void _onRootEntryAdded(Event event) {
    setState(() {
      print(event.snapshot.key);
      entryMap.putIfAbsent(event.snapshot.key, () => event.snapshot.value);
    });
    print(_nodeNeedUpdate);
    if (_nodeNeedUpdate == true) {
      var domain = event.snapshot.key;
      // value contain a map of nodes, each key is the name of the node
      var v = event.snapshot.value;
      v.forEach((node, v) {
        _nodeUpdate('$domain/$node/');
      });
    }
  }

  void _onRootEntryChanged(Event event) {
    print('_onEntryChanged');
    entryMap[event.snapshot.key] = event.snapshot.value;
  }

  void _onRootEntryRemoved(Event event) {
    setState(() {
      entryMap.remove(event.snapshot.key);
    });
  }

  void _loadNodeInfo() {
    _connected = false;
    _controlTimeoutCnt = 0;
    _controlRef = FirebaseDatabase.instance.reference().child(getControlRef());
    _statusRef = FirebaseDatabase.instance.reference().child(getStatusRef());
    _startupRef = FirebaseDatabase.instance.reference().child(getStartupRef());
    _controlSub = _controlRef.onValue.listen(_onValueControl);
    _statusSub = _statusRef.onValue.listen(_onValueStatus);
    _startupSub = _startupRef.onValue.listen(_onValueStartup);
  }

  void _changePreferences() {
    savePreferencesDN(_ctrlDomainName, _ctrlNodeName);
    if (_isNeedCreate == true) {
      DatabaseReference ref;
      ref = FirebaseDatabase.instance.reference().child(getControlRef());
      ref.set(getControlDefault());
      ref = FirebaseDatabase.instance.reference().child(getStartupRef());
      ref.set(getStartupDefault());
    }
    _loadNodeInfo();
  }

  bool checkConnected() {
    return ((_control != null) && (_status != null) && (_startup != null));
  }

  void _onValueControl(Event event) {
    print('_onValueControl');
    setState(() {
      _control = event.snapshot.value;
      _connected = checkConnected();
    });
  }

  void _onValueStatus(Event event) {
    print('_onValueStatus');
    // update control time to keep up node
    DateTime now = new DateTime.now();
    setState(() {
      if ((_control != null) && (_controlTimeoutCnt++ < 10)) {
        _control['time'] = now.millisecondsSinceEpoch ~/ 1000;
        _controlRef.set(_control);
      }
      _status = event.snapshot.value;
      _connected = checkConnected();
    });
  }

  void _onValueStartup(Event event) {
    print('_onValueStartup');
    setState(() {
      _startup = event.snapshot.value;
      _connected = checkConnected();
    });
  }

  void _nodeActionRequest(int value) {
    _controlTimeoutCnt = 0;
    _control['reboot'] = value;
    DateTime now = new DateTime.now();
    _control['time'] = now.millisecondsSinceEpoch ~/ 1000;
    _controlRef.set(_control);
  }
}