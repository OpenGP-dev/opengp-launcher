// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt/controls.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QQmlEngine>
#include <SDL3/SDL.h>
#include <algorithm>
#include <filesystem>

#include "core/scancodes.h"
#include "os/atomic_file.h"

namespace launcher {

Controls::Controls(QString path, QObject* parent) : QObject(parent), path_(std::move(path)) {
  instance_ = this;
  timer_.setInterval(16);
  connect(&timer_, &QTimer::timeout, this, &Controls::pump);
  start();
}

Controls::~Controls() {
  suspend();
  instance_ = nullptr;
}

Controls* Controls::create(QQmlEngine*, QJSEngine*) {
  QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
  return instance_;
}

void Controls::start() {
  // No SDL window here: let SDL report controllers anyway.
  SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
  // Qt, not SDL, decides when the launcher quits: SDL would turn SIGTERM and
  // SIGINT into a quit event nothing reads, and the launcher would not stop
  // when the system asks it to (at logout, for one).
  SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
  input_ = std::make_unique<input::HostInput>();
  std::string error;
  error_.clear();
  // A file OpenGP cannot read leaves the default controls in place, and load
  // sets it aside as controls.toml.bad. If it could not, the file is still
  // there: say so, and save nothing over it until the player restores the
  // defaults. No OpenGP, no file: load would write the defaults there.
  std::error_code ec;
  load_failed_ = !path_.isEmpty() && !input_->load(path_.toStdString(), &error) &&
                 std::filesystem::exists(os::utf8_path(path_.toStdString()), ec);
  if (!error.empty()) error_ = QString::fromStdString(error);
  error.clear();
  if (!input_->init(&error)) error_ = QStringLiteral("No controllers: ") + QString::fromStdString(error);
  timer_.start();
  emit changed();
}

void Controls::suspend() {
  if (!input_) return;
  cancelCapture();
  timer_.stop();
  input_.reset();
  SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
  connected_.clear();
  emit devicesChanged();
  emit changed();
}

void Controls::resume() {
  if (!input_) start();
}

void Controls::reload() {
  if (!input_) return;
  suspend();
  start();
}

input::HostInput::DeviceKind Controls::kind() const { return static_cast<input::HostInput::DeviceKind>(device_); }

void Controls::setDevice(int device) {
  device = std::clamp(device, 0, 2);
  if (device == device_) return;
  cancelCapture();
  device_ = device;
  emit changed();
}

QVariantList Controls::rows() const {
  QVariantList rows;
  for (std::size_t c = 0; c < input::HostInput::kControlCount; ++c) {
    // Steering is whole axes: the keyboard steers with its own two controls.
    if (kind() == input::HostInput::DeviceKind::Keyboard && input::HostInput::control_is_axis(c)) continue;
    QStringList bindings;
    if (input_)
      for (const std::string& b : input_->bindings(kind(), c)) bindings << QString::fromStdString(b);
    rows << QVariantMap{{QStringLiteral("control"), static_cast<int>(c)},
                        {QStringLiteral("name"), QString::fromLatin1(input::HostInput::control_name(c))},
                        {QStringLiteral("label"), QString::fromLatin1(input::HostInput::control_label(c))},
                        {QStringLiteral("axis"), input::HostInput::control_is_axis(c)},
                        {QStringLiteral("bindings"), bindings}};
  }
  return rows;
}

int Controls::capturing() const { return input_ && input_->capturing() ? capture_control_ : -1; }

bool Controls::editable() {
  // Not while a game has the controls, nor over a file that could not be
  // read: the defaults in memory would replace every binding in it.
  if (input_ && !load_failed_) return true;
  emit changed();
  return false;
}

double Controls::forceFeedback() const { return input_ ? input_->force_feedback() : 1.0; }
void Controls::setForceFeedback(double v) {
  if (!editable()) return;
  input_->set_force_feedback(static_cast<float>(v));
  save();
}
double Controls::steerSpeed() const { return input_ ? input_->steer_speed() : 4.0; }
void Controls::setSteerSpeed(double v) {
  if (!editable()) return;
  input_->set_steer_speed(static_cast<float>(v));
  save();
}
double Controls::deadzone() const { return input_ ? input_->deadzone(kind()) : 0.0; }
void Controls::setDeadzone(double v) {
  if (!editable()) return;
  input_->set_deadzone(kind(), static_cast<float>(v));
  save();
}
bool Controls::backgroundInput() const { return input_ && input_->background_input(); }
void Controls::setBackgroundInput(bool v) {
  if (!editable()) return;
  input_->set_background_input(v);
  save();
}

void Controls::startCapture(int control) {
  if (control < 0 || control >= static_cast<int>(input::HostInput::kControlCount) || !editable()) return;
  input_->start_capture(kind(), static_cast<std::size_t>(control));
  capture_control_ = control;
  // Keys come from the launcher's window, before QML sees them.
  qApp->installEventFilter(this);
  emit changed();
}

void Controls::cancelCapture() {
  if (!input_ || !input_->capturing()) return;
  input_->cancel_capture();
  qApp->removeEventFilter(this);
  emit changed();
}

void Controls::removeBinding(int control, int index) {
  if (control < 0 || index < 0 || !editable()) return;
  input_->remove_binding(kind(), static_cast<std::size_t>(control), static_cast<std::size_t>(index));
  save();
}

void Controls::restoreDefaults() {
  if (!input_) return;
  cancelCapture();
  // An unreadable file is kept as controls.toml.bad, as GameSettings keeps
  // settings.toml, so the player can still look at what was in it.
  if (load_failed_) {
    std::error_code ec;
    const std::string path = path_.toStdString();
    std::filesystem::rename(os::utf8_path(path), os::utf8_path(path + ".bad"), ec);
    load_failed_ = false;
  }
  input_->restore_defaults();
  save();
}

void Controls::save() {
  if (load_failed_) return;  // never over a file that could not be read
  std::string error;
  error_ = input_ && !path_.isEmpty() && !input_->save(&error) ? QString::fromStdString(error) : QString();
  emit changed();
}

bool Controls::eventFilter(QObject* watched, QEvent* event) {
  if (!input_ || !input_->capturing()) return QObject::eventFilter(watched, event);
  if (event->type() == QEvent::KeyRelease || event->type() == QEvent::ShortcutOverride) return true;
  if (event->type() != QEvent::KeyPress) return QObject::eventFilter(watched, event);
  const auto* key = static_cast<QKeyEvent*>(event);
  int scancode = sdl_scancode(key->nativeScanCode(), key->nativeVirtualKey());
  if (key->key() == Qt::Key_Escape) scancode = SDL_SCANCODE_ESCAPE;  // cancels, whatever its scan code
  if (scancode == SDL_SCANCODE_UNKNOWN) return true;
  SDL_Event e{};
  e.type = SDL_EVENT_KEY_DOWN;
  e.key.scancode = static_cast<SDL_Scancode>(scancode);
  e.key.repeat = key->isAutoRepeat();
  e.key.down = true;
  input_->capture_event(e);
  if (!input_->capturing()) {
    qApp->removeEventFilter(this);
    save();
  }
  return true;
}

void Controls::pump() {
  if (!input_) return;
  SDL_Event e;
  bool devices = false;
  while (SDL_PollEvent(&e)) {
    input_->handle_event(e);
    if (e.type == SDL_EVENT_JOYSTICK_ADDED || e.type == SDL_EVENT_JOYSTICK_REMOVED) devices = true;
    if (input_->capturing() && kind() != input::HostInput::DeviceKind::Keyboard) {
      input_->capture_event(e);
      if (!input_->capturing()) {
        qApp->removeEventFilter(this);
        save();
      }
    }
  }
  if (devices) update_connected();
}

void Controls::update_connected() {
  QStringList names;
  int count = 0;
  if (SDL_JoystickID* ids = SDL_GetJoysticks(&count)) {
    for (int i = 0; i < count; ++i) {
      const char* name = SDL_IsGamepad(ids[i]) ? SDL_GetGamepadNameForID(ids[i]) : SDL_GetJoystickNameForID(ids[i]);
      names << (name ? QString::fromUtf8(name) : QStringLiteral("Controller"));
    }
    SDL_free(ids);
  }
  if (names != connected_) {
    connected_ = names;
    emit devicesChanged();
  }
}

}  // namespace launcher
