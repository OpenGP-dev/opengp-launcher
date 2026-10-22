// SPDX-License-Identifier: GPL-3.0-or-later
//
// The games' settings (user/settings.toml, shared by both games), read and
// written with OpenGP's own code (app::Settings). A running game rewrites the
// whole file when it saves, so while one runs they can only be looked at; they
// are read again when it ends.
#pragma once

#include <QObject>
#include <QStringList>
#include <QtQmlIntegration>

#include "app/settings.h"

class QQmlEngine;
class QJSEngine;

namespace launcher {

class GameSettings : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged)
  Q_PROPERTY(QString error READ error NOTIFY changed)
  // Values in settings.toml OpenGP did not understand (they keep their
  // settings), or what became of an unreadable file: shown, but no reason to stop.
  Q_PROPERTY(QString warning READ warning NOTIFY changed)
  // [video]
  Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY changed)
  Q_PROPERTY(int windowWidth READ windowWidth WRITE setWindowWidth NOTIFY changed)
  Q_PROPERTY(int windowHeight READ windowHeight WRITE setWindowHeight NOTIFY changed)
  Q_PROPERTY(int aspect READ aspect WRITE setAspect NOTIFY changed)  // index into aspectNames
  Q_PROPERTY(QStringList aspectNames READ aspectNames CONSTANT)
  Q_PROPERTY(int internalResolution READ internalResolution WRITE setInternalResolution NOTIFY changed)
  Q_PROPERTY(bool vsync READ vsync WRITE setVsync NOTIFY changed)
  Q_PROPERTY(bool evenPacing READ evenPacing WRITE setEvenPacing NOTIFY changed)
  // [audio]
  Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY changed)
  // [game]
  Q_PROPERTY(bool freePlay READ freePlay WRITE setFreePlay NOTIFY changed)
  Q_PROPERTY(bool pauseInMenu READ pauseInMenu WRITE setPauseInMenu NOTIFY changed)
  Q_PROPERTY(bool pauseInBackground READ pauseInBackground WRITE setPauseInBackground NOTIFY changed)
  Q_PROPERTY(QString card READ card WRITE setCard NOTIFY changed)
  // [camera]
  Q_PROPERTY(int camera READ camera WRITE setCamera NOTIFY changed)  // 0 off, 1 webcam, 2 test pattern
  Q_PROPERTY(QString webcam READ webcam WRITE setWebcam NOTIFY changed)
  // [discord]
  Q_PROPERTY(bool richPresence READ richPresence WRITE setRichPresence NOTIFY changed)
  // [link]
  Q_PROPERTY(int cabinet READ cabinet WRITE setCabinet NOTIFY changed)
  Q_PROPERTY(bool lan READ lan WRITE setLan NOTIFY changed)
  Q_PROPERTY(int linkPort READ linkPort WRITE setLinkPort NOTIFY changed)

 public:
  explicit GameSettings(QString path, QObject* parent = nullptr);
  ~GameSettings() override;
  static GameSettings* create(QQmlEngine*, QJSEngine*);
  static GameSettings* instance() { return instance_; }

  bool locked() const { return locked_; }
  void set_locked(bool locked);
  QString error() const { return error_; }
  QString warning() const { return warning_; }

  bool fullscreen() const { return s_.fullscreen; }
  int windowWidth() const { return s_.window_width; }
  int windowHeight() const { return s_.window_height; }
  int aspect() const { return static_cast<int>(s_.aspect); }
  QStringList aspectNames() const;
  int internalResolution() const { return static_cast<int>(s_.internal_resolution); }
  bool vsync() const { return s_.vsync; }
  bool evenPacing() const { return s_.even_pacing; }
  int volume() const { return s_.volume; }
  bool freePlay() const { return s_.free_play; }
  bool pauseInMenu() const { return s_.pause_in_menu; }
  bool pauseInBackground() const { return s_.pause_in_background; }
  QString card() const { return QString::fromStdString(s_.card); }
  int camera() const { return static_cast<int>(s_.camera); }
  QString webcam() const { return QString::fromStdString(s_.webcam); }
  bool richPresence() const { return s_.rich_presence; }
  int cabinet() const { return s_.cabinet; }
  bool lan() const { return s_.lan; }
  int linkPort() const { return s_.link_port; }

  void setFullscreen(bool v);
  void setWindowWidth(int v);
  void setWindowHeight(int v);
  void setAspect(int v);
  void setInternalResolution(int v);
  void setVsync(bool v);
  void setEvenPacing(bool v);
  void setVolume(int v);
  void setFreePlay(bool v);
  void setPauseInMenu(bool v);
  void setPauseInBackground(bool v);
  void setCard(const QString& v);
  void setCamera(int v);
  void setWebcam(const QString& v);
  void setRichPresence(bool v);
  void setCabinet(int v);
  void setLan(bool v);
  void setLinkPort(int v);

  // Reads the file again.
  Q_INVOKABLE void reload();
  // Replaces the file with the defaults (after a bad file, which the games ignore).
  Q_INVOKABLE void resetToDefaults();
  // The webcams on this computer (their names, as the [camera] webcam setting takes them).
  Q_INVOKABLE QStringList webcams() const;

 signals:
  void changed();
  void lockedChanged();

 private:
  // Changes the settings with `change` and saves them, unless a game is running.
  template <typename Fn>
  void edit(Fn change);

  static inline GameSettings* instance_ = nullptr;
  QString path_;
  app::Settings s_;
  QString error_;
  QString warning_;
  bool locked_ = false;
};

}  // namespace launcher
