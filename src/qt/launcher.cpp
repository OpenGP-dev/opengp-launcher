// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt/launcher.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QQmlEngine>

#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
#endif

#include "core/game_process.h"
#include "core/source_build.h"
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

// The CMake to build a source tree's games with: the one the launcher was
// built with, else one on PATH.
QString cmake_program() {
  const QString built_with = QStringLiteral(LAUNCHER_CMAKE);
  return QFileInfo::exists(built_with) ? built_with : QStringLiteral("cmake");
}

// The Ninja the launcher was built with, for a new build directory (the
// presets use Ninja), or "" to let CMake look for it.
std::string ninja_program() {
  const QFileInfo built_with(QStringLiteral(LAUNCHER_MAKE_PROGRAM));
  return built_with.exists() && built_with.baseName() == QLatin1String("ninja") ? built_with.filePath().toStdString()
                                                                                : std::string();
}

// A check's verdict as launcher.toml keeps it, and as QML sees it.
const char* verdict_name(DumpVerdict v) {
  switch (v) {
    case DumpVerdict::Good: return "good";
    case DumpVerdict::KnownBad: return "bad";
    default: return "unknown";
  }
}
int verdict_value(const std::string& name) {
  if (name == "good") return Launcher::DumpGood;
  if (name == "bad") return Launcher::DumpKnownBad;
  return Launcher::DumpUnknown;
}

// Stops a CMake run and everything it started (Ninja, the compilers).
void stop_tree(QProcess* process) {
  if (process->state() == QProcess::NotRunning) return;
#ifdef _WIN32
  QProcess::execute(QStringLiteral("taskkill"),
                    {QStringLiteral("/T"), QStringLiteral("/F"), QStringLiteral("/PID"), QString::number(process->processId())});
#else
  ::kill(-static_cast<pid_t>(process->processId()), SIGTERM);  // its process group (see next_cmake_step)
#endif
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
  stop_verify_ = true;
  if (cmake_) {
    cmake_->disconnect(this);
    stop_tree(cmake_);
    cmake_->waitForFinished(5000);
  }
  delete cmake_log_;
  for (std::thread* t : {&check_thread_, &build_thread_, &art_thread_, &verify_thread_})
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
  if (generation == generation_) {
    for (int i = 0; i < games_.rowCount() && i < static_cast<int>(statuses.size()); ++i) {
      GameList::Entry e = games_.at(i);
      e.state = static_cast<int>(statuses[i].state);
      e.image = qs(statuses[i].image);
      const GameArt art = existing_art(qs(home_.user_dir / "art" / e.id.toStdString()), e.image);
      e.logo = art.logo, e.tile = art.tile, e.heroes = art.heroes;
      const ImageCheck* check = current_check(e.id, e.image);
      e.dump_check = check ? verdict_value(check->verdict) : NotChecked;
      e.dump_name = check ? qs(check->dump) : QString();
      e.dump_note = check ? qs(check->note) : QString();
      games_.update(i, e);
    }
    emit currentChanged();  // the current game's details changed
    verify_next();
  }
  // After the games' states, so that whoever waited for the check sees them.
  emit checkingChanged();
  if (generation == generation_) make_missing_art();
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

void Launcher::play(int index, bool testMenu, bool gameLog) {
  if (index < 0 || index >= games_.rowCount() || !running_id_.isEmpty() || !building_id_.isEmpty()) return;
  const GameList::Entry& e = games_.at(index);
  const Game* game = home_.game(e.id.toStdString());
  if (!game) return;
  std::vector<std::string> args = {"--user-dir", utf8(home_.user_dir)};
  if (testMenu) args.push_back("--test-menu");
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
  if (index < 0 || index >= games_.rowCount() || !building_id_.isEmpty()) return;
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
  if (!home_.package) {
    build_from_source(*game, e.image);
    return;
  }

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
  cmake_steps_.clear();
  delete cmake_log_;
  cmake_log_ = nullptr;
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

void Launcher::cancelBuild() {
  cancel_ = true;
  if (cmake_) stop_tree(cmake_);
}

void Launcher::build_from_source(const Game& game, const QString& image) {
  const SourceBuild b = source_build(home_, game, cmake_program().toStdString(), ninja_program());
  cmake_steps_ = b.commands;
  cmake_partial_.clear();
  cmake_error_.clear();
  QDir().mkpath(qs(b.build_dir));
  cmake_log_ = new QFile(qs(b.build_dir / "launcher-build.log"));
  if (!cmake_log_->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    delete cmake_log_;
    cmake_log_ = nullptr;
  }
  build_line(QStringLiteral("Reading ") + image);

  if (build_thread_.joinable()) build_thread_.join();
  build_thread_ = std::thread([this, home = home_, game, image = image.toStdString()] {
    std::string error;
    const bool ok = extract_dol(home, game, image, &error);
    QMetaObject::invokeMethod(this, [this, ok, error = qs(error)] {
      if (!ok) finish_build(false, error);
      else if (cancel_) finish_build(false, QStringLiteral("cancelled"));
      else next_cmake_step();
    }, Qt::QueuedConnection);
  });
}

void Launcher::next_cmake_step() {
  if (cmake_steps_.empty()) {
    finish_build(true, {});
    return;
  }
  const std::vector<std::string> command = std::move(cmake_steps_.front());
  cmake_steps_.erase(cmake_steps_.begin());
  QStringList args;
  for (std::size_t i = 1; i < command.size(); ++i) args << qs(command[i]);
  const bool configure = args.value(0) != QLatin1String("--build");
  progress_ = -1;
  build_line(configure ? QStringLiteral("Configuring with CMake") : QStringLiteral("Building with CMake"));

  cmake_ = new QProcess(this);
  cmake_->setWorkingDirectory(qs(home_.dir));
  cmake_->setProcessChannelMode(QProcess::MergedChannels);
  // The tools beside CMake (Homebrew's, say) on PATH, for when the launcher
  // was not started from a shell.
  const QString program = qs(command[0]);
  if (QFileInfo(program).isAbsolute()) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PATH"), QDir::toNativeSeparators(QFileInfo(program).absolutePath()) +
                                           QDir::listSeparator() + env.value(QStringLiteral("PATH")));
    cmake_->setProcessEnvironment(env);
  }
#ifndef _WIN32
  // In a process group of its own, so that Stop reaches the compilers too.
  cmake_->setChildProcessModifier([] { ::setpgid(0, 0); });
#endif
  connect(cmake_, &QProcess::readyRead, this, &Launcher::cmake_output);
  connect(cmake_, &QProcess::finished, this, [this](int exit_code, QProcess::ExitStatus status) {
    cmake_output();
    if (!cmake_partial_.isEmpty()) build_line(std::exchange(cmake_partial_, QString()));
    cmake_->deleteLater();
    cmake_ = nullptr;
    if (cancel_) {
      finish_build(false, QStringLiteral("cancelled"));
    } else if (status != QProcess::NormalExit || exit_code != 0) {
      const QString log = cmake_log_ ? QDir::toNativeSeparators(cmake_log_->fileName()) : QString();
      finish_build(false, (cmake_error_.isEmpty() ? QStringLiteral("CMake stopped with an error.") : cmake_error_) +
                              (log.isEmpty() ? QString() : QStringLiteral(" (All of it is in ") + log + QStringLiteral(")")));
    } else {
      next_cmake_step();
    }
  });
  connect(cmake_, &QProcess::errorOccurred, this, [this, program](QProcess::ProcessError error) {
    if (error != QProcess::FailedToStart) return;  // the others end in finished()
    const QString why = cmake_->errorString();
    cmake_->deleteLater();
    cmake_ = nullptr;
    finish_build(false, QStringLiteral("Could not run CMake (") + program + QStringLiteral("): ") + why);
  });
  cmake_->start(program, args);
}

void Launcher::cmake_output() {
  if (!cmake_) return;
  cmake_partial_ += QString::fromUtf8(cmake_->readAll());
  qsizetype end;
  while ((end = cmake_partial_.indexOf(QLatin1Char('\n'))) >= 0) {
    QString line = cmake_partial_.left(end);
    cmake_partial_.remove(0, end + 1);
    if (line.endsWith(QLatin1Char('\r'))) line.chop(1);
    if (!line.isEmpty()) build_line(line);
  }
}

void Launcher::build_line(const QString& line) {
  if (cmake_log_) {
    cmake_log_->write(line.toUtf8() + '\n');
    cmake_log_->flush();
  }
  // The first error is the one to show (the compiler's, or Ninja's).
  if (cmake_error_.isEmpty() && line.contains(QLatin1String("error:"))) cmake_error_ = line.trimmed();
  const double fraction = ninja_progress(line.toStdString());
  if (fraction >= 0) progress_ = fraction;
  progress_text_ = line;
  build_log_ << line;
  emit progressChanged();
}

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

// ---- Checking disc images ----

const ImageCheck* Launcher::current_check(const QString& id, const QString& image) const {
  const auto it = prefs_.image_checks.find(id.toStdString());
  if (image.isEmpty() || it == prefs_.image_checks.end() || it->second.image != image.toStdString()) return nullptr;
  std::int64_t size = 0, modified = 0;
  if (!image_stamp(it->second.image, &size, &modified) || size != it->second.size || modified != it->second.modified)
    return nullptr;
  return &it->second;
}

void Launcher::verify_next() {
  if (!verifying_id_.isEmpty()) return;
  for (int i = 0; i < games_.rowCount(); ++i) {
    const GameList::Entry& e = games_.at(i);
    const std::string image = e.image.toStdString();
    if (image.empty() || e.dump_check != NotChecked || verify_failed_.count(image)) continue;
    if (verify_thread_.joinable()) verify_thread_.join();
    verifying_id_ = e.id;
    verify_progress_ = 0;
    emit verifyChanged();
    verify_thread_ = std::thread([this, id = e.id, image] {
      std::int64_t size = 0, modified = 0;
      image_stamp(image, &size, &modified);  // before reading it: a change while it is read makes the check stale
      int shown = 0;
      DumpCheck check;
      std::string error;
      const bool ok = check_dump(image, id.toStdString(), [this, &shown](double fraction) {
        const int percent = static_cast<int>(fraction * 100);
        if (percent == shown) return;
        shown = percent;
        QMetaObject::invokeMethod(this, [this, fraction] {
          verify_progress_ = fraction;
          emit verifyChanged();
        }, Qt::QueuedConnection);
      }, &stop_verify_, &check, &error);
      QMetaObject::invokeMethod(this, [=, this] { finish_verify(id, image, ok, check, error, size, modified); },
                                Qt::QueuedConnection);
    });
    return;
  }
}

void Launcher::finish_verify(const QString& id, const std::string& image, bool ok, const DumpCheck& check,
                             const std::string& error, std::int64_t size, std::int64_t modified) {
  verifying_id_.clear();
  emit verifyChanged();
  const int row = row_of(id);
  if (!ok) {
    verify_failed_.insert(image);
    qWarning("checking %s: %s", image.c_str(), error.c_str());
  } else {
    prefs_.image_checks[id.toStdString()] = {image, size, modified, verdict_name(check.verdict), check.dump, check.note};
    save_prefs();
    if (row >= 0 && games_.at(row).image.toStdString() == image) {
      GameList::Entry e = games_.at(row);
      e.dump_check = verdict_value(verdict_name(check.verdict));
      e.dump_name = qs(check.dump);
      e.dump_note = qs(check.note);
      games_.update(row, e);
      emit currentChanged();
      if (check.verdict != DumpVerdict::Good)
        emit message(e.title + QStringLiteral(": the disc image is not a known good dump. ") + qs(check.note), true);
    }
  }
  verify_next();
}

void Launcher::checkImage(int index) {
  if (index < 0 || index >= games_.rowCount() || !verifying_id_.isEmpty()) return;
  GameList::Entry e = games_.at(index);
  if (e.image.isEmpty()) return;
  prefs_.image_checks.erase(e.id.toStdString());
  verify_failed_.erase(e.image.toStdString());
  e.dump_check = NotChecked;
  e.dump_name.clear();
  e.dump_note.clear();
  games_.update(index, e);
  emit currentChanged();
  verify_next();
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
