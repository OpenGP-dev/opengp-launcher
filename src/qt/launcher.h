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
#include <set>
#include <thread>
#include <vector>

#include "core/dump_check.h"
#include "core/home.h"
#include "core/library.h"
#include "core/prefs.h"
#include "qt/game_list.h"

class QFile;
class QJSEngine;
class QProcess;
class QQmlEngine;

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
  // The heading for homeError ("" for the usual "OpenGP isn't in this folder").
  Q_PROPERTY(QString homeErrorTitle READ homeErrorTitle CONSTANT)
  // Files in the ISO folder that are not usable images, and why.
  Q_PROPERTY(QStringList skippedImages READ skippedImages NOTIFY checkingChanged)
  Q_PROPERTY(QString launcherVersion READ launcherVersion CONSTANT)
  Q_PROPERTY(QString openGPVersion READ openGPVersion CONSTANT)
  Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)
  Q_PROPERTY(QString buildingId READ buildingId NOTIFY buildingChanged)
  Q_PROPERTY(QString progressText READ progressText NOTIFY progressChanged)
  Q_PROPERTY(bool stopping READ stopping NOTIFY progressChanged)  // Stop was pressed: the build is ending
  Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(QStringList buildLog READ buildLog NOTIFY progressChanged)
  Q_PROPERTY(QString runningId READ runningId NOTIFY runningChanged)
  Q_PROPERTY(QString verifyingId READ verifyingId NOTIFY verifyChanged)
  Q_PROPERTY(double verifyProgress READ verifyProgress NOTIFY verifyChanged)
  Q_PROPERTY(int onLaunch READ onLaunch WRITE setOnLaunch NOTIFY prefsChanged)

 public:
  enum State { NoImage, NotBuilt, OutOfDate, Ready, Checking };
  Q_ENUM(State)

  // A disc image against the known good dumps (core/dump_check.h).
  enum DumpState { NotChecked = -1, DumpGood, DumpKnownBad, DumpUnknown };
  Q_ENUM(DumpState)

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
  QString homeErrorTitle() const { return home_error_title_; }
  void setHomeErrorTitle(const QString& title) { home_error_title_ = title; }
  QStringList skippedImages() const { return skipped_images_; }
  QString launcherVersion() const;
  QString openGPVersion() const;
  bool checking() const { return checking_; }
  QString buildingId() const { return building_id_; }
  QString progressText() const { return progress_text_; }
  bool stopping() const { return cancel_ && !building_id_.isEmpty(); }
  double progress() const { return progress_; }
  QStringList buildLog() const { return build_log_; }
  QString runningId() const { return running_id_; }
  QString verifyingId() const { return verifying_id_; }
  double verifyProgress() const { return verify_progress_; }
  int onLaunch() const { return static_cast<int>(prefs_.on_launch); }
  void setOnLaunch(int value);

  // Looks at the games again: their images, builds and pictures.
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void play(int index, bool testMenu = false, bool gameLog = false);
  Q_INVOKABLE void addImage(const QUrl& file);
  // Clears the message about a disc image that could not be added.
  Q_INVOKABLE void dismissImageError(int index);
  Q_INVOKABLE void build(int index);
  Q_INVOKABLE void cancelBuild();
  Q_INVOKABLE void removeBuild(int index);
  // Checks the game's disc image against the known good dumps again.
  Q_INVOKABLE void checkImage(int index);
  // "home", "user", "screenshots", "logs", "cards", "backups" or "image" (shows the current game's disc image)
  Q_INVOKABLE void openFolder(const QString& which);
  Q_INVOKABLE void openLink(const QString& url);

 signals:
  void currentChanged();
  void checkingChanged();
  void buildingChanged();
  void progressChanged();
  void runningChanged();
  void verifyChanged();
  void prefsChanged();
  void message(const QString& text, bool error);
  void gameStarting(const QString& id);  // before it starts: let go of the controllers
  void gameEnded(const QString& id);     // settings may have changed
  void minimizeRequested();
  void raiseRequested();  // something went wrong in a game: show the window
  void quitRequested();

 private:
  void apply_statuses(int generation, const std::vector<GameStatus>& statuses, const QStringList& skipped);
  void make_missing_art();
  void set_image_error(int row, const QString& error);
  void game_ended(const QString& id, int exit_code, bool crashed);
  // A game started elsewhere (its shortcut, an earlier launcher) counts as
  // running too, while it holds its lock: the launcher's changes to its
  // settings and cards would be undone when it saves.
  void check_running_elsewhere();
  void finish_build(bool ok, const QString& error);
  // A source tree's games, built with CMake (see core/source_build.h).
  void build_from_source(const Game& game, const QString& image);
  void next_cmake_step();
  void cmake_output();
  void build_line(const QString& line);
  // Disc images are checked one at a time, in the background, when there is
  // no check of them yet (prefs_.image_checks).
  void verify_next();
  void finish_verify(const QString& id, const std::string& image, bool ok, const DumpCheck& check,
                     const std::string& error, std::int64_t size, std::int64_t modified);
  const ImageCheck* current_check(const QString& id, const QString& image) const;
  int row_of(const QString& id) const;

  static inline Launcher* instance_ = nullptr;
  Home home_;
  QString home_error_, prefs_path_;
  QString home_error_title_;
  QStringList skipped_images_;
  fs::file_time_type started_{};  // when the running game was started
  Prefs prefs_;
  GameList games_;
  int current_ = 0;
  bool checking_ = false;
  int generation_ = 0;
  std::thread check_thread_, build_thread_, art_thread_, verify_thread_;
  std::atomic<bool> cancel_{false}, stop_verify_{false};
  bool art_running_ = false, check_again_ = false;
  QString building_id_, progress_text_, running_id_, verifying_id_;
  bool running_elsewhere_ = false;  // running_id_ is a game the launcher did not start
  bool lock_was_free_ = false;      // and its lock was free at the last look
  double verify_progress_ = 0;
  std::set<std::string> verify_failed_;  // images that could not be read: not again until asked
  std::set<std::string> art_failed_;     // images no pictures could be made from: not again until one is added
  double progress_ = -1;
  QStringList build_log_;
  std::vector<std::vector<std::string>> cmake_steps_;  // the CMake commands still to run
  QProcess* cmake_ = nullptr;                          // the one running
  QFile* cmake_log_ = nullptr;                         // what they print: build/<preset>/launcher-build.log
  QString cmake_partial_, cmake_error_;                // a line not finished yet; the first error
};

}  // namespace launcher
