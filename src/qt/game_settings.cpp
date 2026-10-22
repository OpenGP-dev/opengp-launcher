// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt/game_settings.h"

#include <filesystem>

#include "os/atomic_file.h"

#include <QQmlEngine>
#include <algorithm>

#include "camera/webcam.h"

namespace launcher {

namespace {
constexpr app::AspectRatio kAspects[] = {app::AspectRatio::Auto, app::AspectRatio::Original, app::AspectRatio::Wide16x9,
                                         app::AspectRatio::Wide16x10, app::AspectRatio::Wide21x9};
}

GameSettings::GameSettings(QString path, QObject* parent) : QObject(parent), path_(std::move(path)) {
  instance_ = this;
  reload();
}

GameSettings::~GameSettings() { instance_ = nullptr; }

GameSettings* GameSettings::create(QQmlEngine*, QJSEngine*) {
  QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
  return instance_;
}

void GameSettings::set_locked(bool locked) {
  if (locked == locked_) return;
  locked_ = locked;
  if (!locked) reload();  // the game may have saved its own changes
  emit lockedChanged();
}

void GameSettings::reload() {
  app::Settings s;
  std::string error;
  error_.clear();
  warning_.clear();
  if (!app::load_settings(path_.toStdString(), &s, &error)) {
    // The games run on their defaults then; say so, and leave the file be.
    error_ = QString::fromStdString(error);
    s = app::Settings{};
  } else if (!error.empty()) {
    warning_ = QString::fromStdString(error);
  }
  s_ = s;
  emit changed();
}

void GameSettings::resetToDefaults() {
  if (locked_) return;
  // An unreadable file is kept as settings.toml.bad, as the games keep it,
  // so the player can still look at what was in it.
  QString kept;
  if (!error_.isEmpty()) {
    std::error_code ec;
    const std::string path = path_.toStdString();
    std::filesystem::rename(os::utf8_path(path), os::utf8_path(path + ".bad"), ec);
    if (!ec) kept = QStringLiteral("The old file was kept as settings.toml.bad.");
  }
  s_ = app::Settings{};
  std::string error;
  error_ = app::save_settings(path_.toStdString(), s_, &error) ? QString() : QString::fromStdString(error);
  warning_ = error_.isEmpty() ? kept : QString();
  emit changed();
}

template <typename Fn>
void GameSettings::edit(Fn change) {
  if (locked_ || !error_.isEmpty()) {
    emit changed();  // put the controls back
    return;
  }
  app::Settings s = s_;
  change(s);
  if (s == s_) return;
  std::string error;
  if (!app::save_settings(path_.toStdString(), s, &error)) {
    error_ = QString::fromStdString(error);
  } else {
    s_ = s;
  }
  emit changed();
}

QStringList GameSettings::aspectNames() const {
  QStringList names;
  for (app::AspectRatio a : kAspects) names << QString::fromLatin1(app::aspect_name(a));
  return names;
}

QStringList GameSettings::webcams() const {
  QStringList out;
  for (const std::string& name : camera::Webcam::names()) out << QString::fromStdString(name);
  return out;
}

void GameSettings::setFullscreen(bool v) { edit([&](app::Settings& s) { s.fullscreen = v; }); }
void GameSettings::setWindowWidth(int v) { edit([&](app::Settings& s) { s.window_width = std::clamp(v, 320, 16384); }); }
void GameSettings::setWindowHeight(int v) { edit([&](app::Settings& s) { s.window_height = std::clamp(v, 240, 16384); }); }
void GameSettings::setAspect(int v) {
  edit([&](app::Settings& s) { s.aspect = kAspects[std::clamp(v, 0, static_cast<int>(std::size(kAspects)) - 1)]; });
}
void GameSettings::setInternalResolution(int v) {
  edit([&](app::Settings& s) { s.internal_resolution = static_cast<gekko::u32>(std::clamp(v, 0, 8)); });
}
void GameSettings::setVsync(bool v) { edit([&](app::Settings& s) { s.vsync = v; }); }
void GameSettings::setEvenPacing(bool v) { edit([&](app::Settings& s) { s.even_pacing = v; }); }
void GameSettings::setVolume(int v) { edit([&](app::Settings& s) { s.volume = std::clamp(v, 0, 100); }); }
void GameSettings::setFreePlay(bool v) { edit([&](app::Settings& s) { s.free_play = v; }); }
void GameSettings::setPauseInMenu(bool v) { edit([&](app::Settings& s) { s.pause_in_menu = v; }); }
void GameSettings::setPauseInBackground(bool v) { edit([&](app::Settings& s) { s.pause_in_background = v; }); }
void GameSettings::setCard(const QString& v) { edit([&](app::Settings& s) { s.card = v.toStdString(); }); }
void GameSettings::setCamera(int v) {
  edit([&](app::Settings& s) { s.camera = static_cast<app::CameraSource>(std::clamp(v, 0, 2)); });
}
void GameSettings::setWebcam(const QString& v) { edit([&](app::Settings& s) { s.webcam = v.toStdString(); }); }
void GameSettings::setRichPresence(bool v) { edit([&](app::Settings& s) { s.rich_presence = v; }); }
void GameSettings::setCabinet(int v) { edit([&](app::Settings& s) { s.cabinet = std::clamp(v, 0, 4); }); }
void GameSettings::setLan(bool v) { edit([&](app::Settings& s) { s.lan = v; }); }
void GameSettings::setLinkPort(int v) { edit([&](app::Settings& s) { s.link_port = std::clamp(v, 1024, 65535 - 40); }); }

}  // namespace launcher
