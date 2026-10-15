// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt/launcher.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QProcess>
#include <QQmlEngine>

#include "core/game_process.h"
#include "core/user_data.h"
#include "qt/art_maker.h"

namespace launcher {

namespace {

QString qs(const std::string& s) { return QString::fromStdString(s); }
QString qs(const fs::path& p) { return QString::fromStdString(utf8(p)); }

// Runs `fn` on the UI thread later, if `target` still exists then. For
// threads that may outlive the launcher's objects (a game's).
template <typename Fn>
void post(QObject* target, Fn fn) {
  QPointer<QObject> guard(target);
  if (QCoreApplication* app = QCoreApplication::instance())
    QMetaObject::invokeMethod(app, [guard, fn = std::move(fn)] {
      if (guard) fn();
    }, Qt::QueuedConnection);
}

// The last line a game printed, for when it stops with an error.
QString last_line(const fs::path& log) {
  QFile f(qs(log));
  if (!f.open(QIODevice::ReadOnly)) return {};
  const QStringList lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
  for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
    const QString line = it->trimmed();
    if (!line.isEmpty() && !line.startsWith(QLatin1String("[opengp] status"))) return line;
  }
  return {};
}

}  // namespace

Launcher::Launcher(Home home, QString home_error, QString prefs_path, QObject* parent)
    : QObject(parent), home_(std::move(home)), home_error_(std::move(home_error)), prefs_path_(std::move(prefs_path)) {
  instance_ = this;
  std::string error;
  if (!load_prefs(prefs_path_.toStdString(), &prefs_, &error)) qWarning("%s", error.c_str());

  QList<GameList::Entry> entries;
  for (const Game& g : home_.games) {
    GameList::Entry e;
    e.id = qs(g.info.id);
    e.title = qs(g.info.title);
    e.shortTitle = qs(g.short_title);
    e.exe = qs(g.exe);
    e.state = Checking;
    entries << e;
  }
  games_.reset(std::move(entries));
  current_ = std::max(0, row_of(qs(prefs_.game)));
  refresh();
}

Launcher::~Launcher() {
  cancel_ = true;
  for (std::thread* t : {&check_thread_, &build_thread_, &art_thread_})
    if (t->joinable()) t->join();
  instance_ = nullptr;
}

Launcher* Launcher::create(QQmlEngine*, QJSEngine*) {
  QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
  return instance_;
}

void Launcher::save_prefs() {
  std::string error;
  if (!launcher::save_prefs(prefs_path_.toStdString(), prefs_, &error)) emit message(qs(error), true);
}

int Launcher::row_of(const QString& id) const {
  for (int i = 0; i < games_.rowCount(); ++i)
    if (games_.at(i).id == id) return i;
  return -1;
}

void Launcher::setCurrentIndex(int index) {
  if (index == current_ || index < 0 || index >= games_.rowCount()) return;
  current_ = index;
  prefs_.game = games_.at(index).id.toStdString();
  save_prefs();
  emit currentChanged();
}

QString Launcher::currentId() const { return games_.rowCount() ? games_.at(current_).id : QString(); }
QString Launcher::homePath() const { return QDir::toNativeSeparators(qs(home_.dir)); }
QString Launcher::userPath() const { return QDir::toNativeSeparators(qs(home_.user_dir)); }
QString Launcher::launcherVersion() const { return QStringLiteral(LAUNCHER_VERSION); }
QString Launcher::openGPVersion() const {
  return home_.package && !home_.pkg.version.empty() ? qs(home_.pkg.version) : QStringLiteral(OPENGP_VERSION);
}

void Launcher::setOnLaunch(int value) {
  const auto on_launch = static_cast<OnLaunch>(std::clamp(value, 0, 2));
  if (on_launch == prefs_.on_launch) return;
  prefs_.on_launch = on_launch;
  save_prefs();
  emit prefsChanged();
}

// ---- Looking at the games ----

void Launcher::refresh() {
  if (checking_) {
    check_again_ = true;
    return;
  }
  if (check_thread_.joinable()) check_thread_.join();
  checking_ = true;
  emit checkingChanged();
  const int generation = ++generation_;
  check_thread_ = std::thread([this, generation, home = home_] {
    auto statuses = check_games(home);
    QMetaObject::invokeMethod(this, [this, generation, statuses = std::move(statuses)] {
      apply_statuses(generation, statuses);
    }, Qt::QueuedConnection);
  });
}

void Launcher::apply_statuses(int generation, const std::vector<GameStatus>& statuses) {
  checking_ = false;
  emit checkingChanged();
  if (generation == generation_) {
    for (int i = 0; i < games_.rowCount() && i < static_cast<int>(statuses.size()); ++i) {
      GameList::Entry e = games_.at(i);
      e.state = static_cast<int>(statuses[i].state);
      e.image = qs(statuses[i].image);
      const GameArt art = existing_art(qs(home_.user_dir / "art" / e.id.toStdString()), e.image);
      e.logo = art.logo, e.tile = art.tile, e.heroes = art.heroes;
      games_.update(i, e);
    }
    emit currentChanged();  // the current game's details changed
    make_missing_art();
  }
  if (check_again_) {
    check_again_ = false;
    refresh();
  }
}

void Launcher::make_missing_art() {
  if (art_running_) return;
  struct Job {
    QString id, image, dir;
  };
  std::vector<Job> jobs;
  for (int i = 0; i < games_.rowCount(); ++i) {
    const GameList::Entry& e = games_.at(i);
    const QString dir = qs(home_.user_dir / "art" / e.id.toStdString());
    if (!e.image.isEmpty() && !existing_art(dir, e.image).current) jobs.push_back({e.id, e.image, dir});
  }
  if (jobs.empty()) return;
  if (art_thread_.joinable()) art_thread_.join();
  art_running_ = true;
  art_thread_ = std::thread([this, jobs = std::move(jobs)] {
    for (const Job& job : jobs) {
      if (cancel_) break;
      QString error;
      if (!make_art(job.image, job.id, job.dir, &error)) qWarning("pictures for %s: %s", qPrintable(job.id), qPrintable(error));
    }
    QMetaObject::invokeMethod(this, [this] {
      art_running_ = false;
      refresh();
    }, Qt::QueuedConnection);
  });
}

// ---- Playing ----

void Launcher::play(int index, bool testMenu, bool freePlay, bool gameLog) {
  if (index < 0 || index >= games_.rowCount() || !running_id_.isEmpty() || !building_id_.isEmpty()) return;
  const GameList::Entry& e = games_.at(index);
  const Game* game = home_.game(e.id.toStdString());
  if (!game) return;
  std::vector<std::string> args = {"--user-dir", utf8(home_.user_dir)};
  if (testMenu) args.push_back("--test-menu");
  if (freePlay) args.push_back("--free-play");
  if (gameLog) args.push_back("--game-log");

  emit gameStarting(e.id);
  const fs::path log = next_log(home_.user_dir, e.id.toStdString());
  auto process = std::make_shared<GameProcess>();
  std::string error;
  if (!process->start(utf8(game->exe), args, utf8(log), &error)) {
    emit message(qs(error), true);
    emit gameEnded(e.id);
    return;
  }
  running_id_ = e.id;
  emit runningChanged();
  std::thread([this, process, id = e.id] {
    const GameProcess::Result r = process->wait();
    post(this, [this, id, r] { game_ended(id, r.exit_code, r.crashed); });
  }).detach();

  if (prefs_.on_launch == OnLaunch::Minimize) emit minimizeRequested();
  else if (prefs_.on_launch == OnLaunch::Close) emit quitRequested();
}

void Launcher::game_ended(const QString& id, int exit_code, bool crashed) {
  running_id_.clear();
  emit runningChanged();
  emit gameEnded(id);
  const int row = row_of(id);
  const QString title = row >= 0 ? games_.at(row).title : id;
  if (crashed || exit_code != 0) {
    const QString why = last_line(log_path(home_.user_dir, id.toStdString()));
    emit message(title + (crashed ? QStringLiteral(" stopped unexpectedly.") : QStringLiteral(" could not run.")) +
                     (why.isEmpty() ? QString() : QStringLiteral(" ") + why),
                 true);
  }
}

// ---- Disc images and builds ----

void Launcher::addImage(const QUrl& file) {
  const QString path = file.isLocalFile() ? file.toLocalFile() : file.toString();
  std::string id, error;
  if (!add_image(home_, path.toStdString(), &id, &error)) {
    emit message(qs(error), true);
    return;
  }
  const int row = row_of(qs(id));
  if (row >= 0) {
    setCurrentIndex(row);
    // New pictures for the new image.
    QDir(qs(home_.user_dir / "art" / id)).removeRecursively();
    emit message(games_.at(row).title + QStringLiteral(": disc image added."), false);
  }
  refresh();
}

void Launcher::build(int index) {
  if (!home_.package || index < 0 || index >= games_.rowCount() || !building_id_.isEmpty()) return;
  const GameList::Entry& e = games_.at(index);
  if (e.image.isEmpty() || running_id_ == e.id) return;
  const Game* game = home_.game(e.id.toStdString());
  if (!game) return;
  if (build_thread_.joinable()) build_thread_.join();
  cancel_ = false;
  building_id_ = e.id;
  progress_ = -1;
  progress_text_ = QStringLiteral("Getting ready");
  build_log_.clear();
  emit buildingChanged();
  emit progressChanged();

  build_thread_ = std::thread([this, pkg = home_.pkg, info = game->info, image = e.image.toStdString()] {
    installer::Reporter report;
    report.line = [this](const std::string& line) {
      QMetaObject::invokeMethod(this, [this, line = qs(line)] {
        progress_text_ = line;
        build_log_ << line;
        emit progressChanged();
      }, Qt::QueuedConnection);
    };
    report.progress = [this](double fraction) {
      QMetaObject::invokeMethod(this, [this, fraction] {
        progress_ = fraction;
        emit progressChanged();
      }, Qt::QueuedConnection);
    };
    std::string cxx, error;
    installer::BuildOptions options;
    options.cancel = &cancel_;
    const bool ok = installer::find_compiler(pkg, "", report, &cxx, &error) &&
                    installer::install_game(pkg, info, image, cxx, options, report, &error);
    QMetaObject::invokeMethod(this, [this, ok, error = qs(error)] { finish_build(ok, error); }, Qt::QueuedConnection);
  });
}

void Launcher::finish_build(bool ok, const QString& error) {
  const int row = row_of(building_id_);
  const QString title = row >= 0 ? games_.at(row).title : building_id_;
  building_id_.clear();
  progress_ = -1;
  progress_text_.clear();
  emit buildingChanged();
  emit progressChanged();
  if (ok) emit message(title + QStringLiteral(" is ready to play."), false);
  else if (error == QLatin1String("cancelled")) emit message(QStringLiteral("Stopped building ") + title + QStringLiteral("."), false);
  else emit message(QStringLiteral("Could not build ") + title + QStringLiteral(": ") + error, true);
  refresh();
}

void Launcher::cancelBuild() { cancel_ = true; }

void Launcher::removeBuild(int index) {
  if (!home_.package || index < 0 || index >= games_.rowCount()) return;
  const GameList::Entry& e = games_.at(index);
  if (running_id_ == e.id || building_id_ == e.id) return;
  if (!QFile::remove(e.exe)) {
    emit message(QStringLiteral("Could not remove ") + QDir::toNativeSeparators(e.exe), true);
    return;
  }
  emit message(e.title + QStringLiteral(" removed. Build it again from its disc image to play."), false);
  refresh();
}

// ---- Folders and links ----

void Launcher::openFolder(const QString& which) {
  if (which == QLatin1String("image")) {
    const QString image = games_.rowCount() ? games_.at(current_).image : QString();
    if (image.isEmpty()) return;
#ifdef _WIN32
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            {QStringLiteral("/select,") + QDir::toNativeSeparators(image)});
#elif defined(__APPLE__)
    QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-R"), image});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(image).absolutePath()));
#endif
    return;
  }
  fs::path dir = home_.user_dir;
  if (which == QLatin1String("home")) dir = home_.dir;
  else if (which == QLatin1String("screenshots")) dir /= "screenshots";
  else if (which == QLatin1String("logs")) dir /= "logs";
  else if (which == QLatin1String("backups")) dir /= "backups";
  else if (which == QLatin1String("cards") && games_.rowCount()) dir = dir / "cards" / currentId().toStdString();
  std::error_code ec;
  fs::create_directories(dir, ec);
  QDesktopServices::openUrl(QUrl::fromLocalFile(qs(dir)));
}

void Launcher::openLink(const QString& url) {
  const QUrl u(url);
  if (u.scheme() == QLatin1String("https")) QDesktopServices::openUrl(u);
}

}  // namespace launcher
