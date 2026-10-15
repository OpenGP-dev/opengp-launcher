// SPDX-License-Identifier: GPL-3.0-or-later
//
// The games' controls (user/controls.toml), edited with OpenGP's own code
// (input::HostInput): the keys, buttons and axes bound to each of the
// cabinet's controls, for the keyboard, gamepads and other controllers
// (wheels). Controllers are read through SDL (without a window); keys come
// from the launcher's window and are turned into the SDL scancodes the games
// name keys by.
#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQmlIntegration>
#include <memory>

#include "input/host.h"

class QQmlEngine;
class QJSEngine;

namespace launcher {

class Controls : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(int device READ device WRITE setDevice NOTIFY changed)  // 0 keyboard, 1 gamepad, 2 other controller
  Q_PROPERTY(QVariantList rows READ rows NOTIFY changed)             // [{name, label, axis, bindings: [..]}]
  Q_PROPERTY(int capturing READ capturing NOTIFY changed)            // the control being bound, or -1
  Q_PROPERTY(double forceFeedback READ forceFeedback WRITE setForceFeedback NOTIFY changed)
  Q_PROPERTY(double steerSpeed READ steerSpeed WRITE setSteerSpeed NOTIFY changed)
  Q_PROPERTY(double deadzone READ deadzone WRITE setDeadzone NOTIFY changed)
  Q_PROPERTY(bool backgroundInput READ backgroundInput WRITE setBackgroundInput NOTIFY changed)
  Q_PROPERTY(QStringList connected READ connected NOTIFY devicesChanged)
  Q_PROPERTY(bool locked READ locked NOTIFY changed)
  Q_PROPERTY(QString error READ error NOTIFY changed)

 public:
  explicit Controls(QString path, QObject* parent = nullptr);
  ~Controls() override;
  static Controls* create(QQmlEngine*, QJSEngine*);

  int device() const { return device_; }
  void setDevice(int device);
  QVariantList rows() const;
  int capturing() const;
  double forceFeedback() const;
  void setForceFeedback(double v);
  double steerSpeed() const;
  void setSteerSpeed(double v);
  double deadzone() const;
  void setDeadzone(double v);
  bool backgroundInput() const;
  void setBackgroundInput(bool v);
  QStringList connected() const { return connected_; }
  bool locked() const { return !input_; }
  QString error() const { return error_; }

  Q_INVOKABLE void startCapture(int control);
  Q_INVOKABLE void cancelCapture();
  Q_INVOKABLE void removeBinding(int control, int index);
  Q_INVOKABLE void restoreDefaults();

  // While a game runs it has the controllers (and rewrites the file): let go
  // of them, and read the file again afterwards.
  void suspend();
  void resume();

 signals:
  void changed();
  void devicesChanged();

 protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void start();
  void pump();
  void save();
  void update_connected();
  input::HostInput::DeviceKind kind() const;

  static inline Controls* instance_ = nullptr;
  QString path_, error_;
  std::unique_ptr<input::HostInput> input_;
  QTimer timer_;
  int device_ = 0;
  int capture_control_ = -1;
  QStringList connected_;
};

}  // namespace launcher
