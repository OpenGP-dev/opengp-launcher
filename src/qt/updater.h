// SPDX-License-Identifier: GPL-3.0-or-later
//
// Updates for an installed package, for QML. With the news (qt/news.h), the
// launcher looks at the latest release (LAUNCHER_RELEASES_URL: the
// launcher's, whose tag is the launcher's version and whose files are
// OpenGP's packages; see core/update.h). When its launcher or its OpenGP is
// newer, Home offers it. Installing it downloads this computer's package,
// checks it against the SHA-256 GitHub gives for it, unpacks it into
// <package>/update/ with the system's tar, makes sure its launcher runs, puts
// its files in place of the old ones and opens the new launcher. Without a
// package for this computer, or outside an installed package, Home offers
// only the release's page.
#pragma once

#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QtQmlIntegration>

class QFile;
class QJSEngine;
class QNetworkReply;
class QProcess;
class QQmlEngine;

namespace launcher {

class Updater : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(QString name READ name NOTIFY changed)  // the update: "OpenGP 1.0.2", "OpenGP Launcher 1.0.2", or "" for none
  Q_PROPERTY(QString page READ page NOTIFY changed)  // its release page
  Q_PROPERTY(bool canInstall READ canInstall NOTIFY changed)  // else Home offers only its page
  Q_PROPERTY(bool rebuilds READ rebuilds NOTIFY changed)      // a newer OpenGP: the games are built again afterwards
  Q_PROPERTY(int sizeMB READ sizeMB NOTIFY changed)           // the download
  Q_PROPERTY(int state READ state NOTIFY changed)
  Q_PROPERTY(double progress READ progress NOTIFY changed)    // of the download, 0 to 1

 public:
  // Waiting: unpacked and checked, until the build or the game ends.
  enum State { Idle, Downloading, Unpacking, Waiting, Installing };
  Q_ENUM(State)

  // No default parent, as News (QML would make a second Updater).
  explicit Updater(QObject* parent);
  ~Updater() override;
  static Updater* create(QQmlEngine*, QJSEngine*);
  static Updater* instance() { return instance_; }

  QString name() const;
  QString page() const { return name().isEmpty() ? QString() : page_; }
  bool canInstall() const;
  bool rebuilds() const;
  int sizeMB() const { return static_cast<int>((size_ + 999999) / 1000000); }
  int state() const { return state_; }
  double progress() const { return progress_; }

  // Looks at the latest release again (News does, with the news).
  void check();
  Q_INVOKABLE void install();
  Q_INVOKABLE void cancel();

  // Once the window has closed after an update: opens the new launcher.
  void restart_if_updated();

 signals:
  void changed();

 private:
  void finish_download();
  bool take(const QByteArray& data);  // a part of the download: false if it can't be
  void unpack();
  void check_unpacked();
  void apply();
  void fail(const QString& why);
  void stop();
  void clean_up();

  static inline Updater* instance_ = nullptr;
  QNetworkAccessManager network_;
  QString program_;            // the launcher's, as it was when it started
  bool self_in_home_ = false;  // the launcher is the package's own (not --home elsewhere)
  // The latest release: its versions, page and package for this computer.
  QString launcher_version_, opengp_version_, page_, asset_name_, asset_url_, sha256_;
  qint64 size_ = 0;
  State state_ = Idle;
  double progress_ = 0;
  QNetworkReply* reply_ = nullptr;
  QFile* archive_ = nullptr;
  QCryptographicHash hash_{QCryptographicHash::Sha256};
  qint64 received_ = 0;
  QProcess* process_ = nullptr;  // tar, then the new launcher's --version
  bool stopping_ = false;
  bool restart_ = false;
};

}  // namespace launcher
