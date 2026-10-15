// SPDX-License-Identifier: GPL-3.0-or-later
//
// The launcher's state for QML: the games and what can be done with them
// (play, build, add a disc image), and the game that is running.
#pragma once

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQmlIntegration>
#include <atomic>
#include <memory>
#include <thread>

#include "core/home.h"
#include "core/library.h"
#include "core/prefs.h"
#include "qt/game_list.h"

class QQmlEngine;
class QJSEngine;

namespace launcher {

class Launcher : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(GameList* games READ games CONSTANT)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
  Q_PROPERTY(QString currentId READ currentId NOTIFY currentChanged)
  Q_PROPERTY(bool isPackage READ isPackage CONSTANT)
  Q_PROPERTY(QString homePath READ homePath CONSTANT)
  Q_PROPERTY(QString userPath READ userPath CONSTANT)
  Q_PROPERTY(QString homeError READ homeError CONSTANT)
  Q_PROPERTY(QString launcherVersion READ launcherVersion CONSTANT)
  Q_PROPERTY(QString openGPVersion READ openGPVersion CONSTANT)
  Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)
  Q_PROPERTY(QString buildingId READ buildingId NOTIFY buildingChanged)
  Q_PROPERTY(QString progressText READ progressText NOTIFY progressChanged)
  Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(QStringList buildLog READ buildLog NOTIFY progressChanged)
  Q_PROPERTY(QString runningId READ runningId NOTIFY runningChanged)
  Q_PROPERTY(int onLaunch READ onLaunch WRITE setOnLaunch NOTIFY prefsChanged)

 public:
  enum State { NoImage, NotBuilt, OutOfDate, Ready, Checking };
  Q_ENUM(State)

  // OnLaunch values, for QML.
  enum LaunchAction { KeepOpen, Minimize, Close };
  Q_ENUM(LaunchAction)

  Launcher(Home home, QString home_error, QString prefs_path, QObject* parent = nullptr);
  ~Launcher() override;

  static Launcher* create(QQmlEngine*, QJSEngine*);
  static Launcher* instance() { return instance_; }

  const Home& home() const { return home_; }
  Prefs& prefs() { return prefs_; }
  void save_prefs();

  GameList* games() { return &games_; }
  int currentIndex() const { return current_; }
  void setCurrentIndex(int index);
  QString currentId() const;
  bool isPackage() const { return home_.package; }
  QString homePath() const;
  QString userPath() const;
  QString homeError() const { return home_error_; }
  QString launcherVersion() const;
  QString openGPVersion() const;
  bool checking() const { return checking_; }
  QString buildingId() const { return building_id_; }
  QString progressText() const { return progress_text_; }
  double progress() const { return progress_; }
  QStringList buildLog() const { return build_log_; }
  QString runningId() const { return running_id_; }
  int onLaunch() const { return static_cast<int>(prefs_.on_launch); }
  void setOnLaunch(int value);

  // Looks at the games again: their images, builds and pictures.
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void play(int index, bool testMenu = false, bool freePlay = false, bool gameLog = false);
  Q_INVOKABLE void addImage(const QUrl& file);
  Q_INVOKABLE void build(int index);
  Q_INVOKABLE void cancelBuild();
  Q_INVOKABLE void removeBuild(int index);
  // "home", "user", "screenshots", "logs", "cards", "backups" or "image" (shows the current game's disc image)
  Q_INVOKABLE void openFolder(const QString& which);
  Q_INVOKABLE void openLink(const QString& url);

 signals:
  void currentChanged();
  void checkingChanged();
  void buildingChanged();
  void progressChanged();
  void runningChanged();
  void prefsChanged();
  void message(const QString& text, bool error);
  void gameStarting(const QString& id);  // before it starts: let go of the controllers
  void gameEnded(const QString& id);     // settings may have changed
  void minimizeRequested();
  void quitRequested();

 private:
  void apply_statuses(int generation, const std::vector<GameStatus>& statuses);
  void make_missing_art();
  void game_ended(const QString& id, int exit_code, bool crashed);
  void finish_build(bool ok, const QString& error);
  int row_of(const QString& id) const;

  static inline Launcher* instance_ = nullptr;
  Home home_;
  QString home_error_, prefs_path_;
  Prefs prefs_;
  GameList games_;
  int current_ = 0;
  bool checking_ = false;
  int generation_ = 0;
  std::thread check_thread_, build_thread_, art_thread_;
  std::atomic<bool> cancel_{false};
  bool art_running_ = false, check_again_ = false;
  QString building_id_, progress_text_, running_id_;
  double progress_ = -1;
  QStringList build_log_;
};

}  // namespace launcher
