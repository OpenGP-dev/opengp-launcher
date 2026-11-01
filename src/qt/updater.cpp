// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt/updater.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QProcess>
#include <QQmlEngine>
#include <QSaveFile>
#include <QTimer>

#include "core/news.h"
#include "core/update.h"
#include "installer/installer.h"
#include "qt/launcher.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace launcher {

namespace {

constexpr qint64 kMaxRelease = 1024 * 1024;  // GitHub's answer: the release with its notes and files
constexpr int kVersionTimeoutMs = 30000;     // the new launcher's --version

Launcher& app() { return *Launcher::instance(); }

QString qs(const std::string& s) { return QString::fromStdString(s); }
QString qs(const fs::path& p) { return QString::fromStdString(utf8(p)); }
fs::path path_of(const QString& s) { return fs::path(QDir::toNativeSeparators(s).toStdU16String()); }

fs::path dir() { return update_dir(app().home().dir); }

}  // namespace

Updater::Updater(QObject* parent) : QObject(parent) {
  instance_ = this;
  network_.setTransferTimeout(30000);
  program_ = QCoreApplication::applicationFilePath();
  const Home& home = app().home();
  std::error_code ec;
  self_in_home_ = home.package && fs::equivalent(launcher_program(home.dir), path_of(program_), ec);
  if (!home.package || !app().homeError().isEmpty()) return;

  // Just updated: say so, once the window is up.
  QFile done(qs(update_done_file(home.dir)));
  if (done.open(QIODevice::ReadOnly)) {
    const QStringList from = QString::fromUtf8(done.readAll()).split(QLatin1Char('\n'));
    const std::string old_opengp = from.value(1).trimmed().toStdString();
    const QString text =
        newer_version(home.pkg.version, old_opengp)
            ? QStringLiteral("OpenGP is updated to ") + qs(home.pkg.version) +
                  QStringLiteral(". Choose Update on each game you installed, to build it for this version.")
            : QStringLiteral("The launcher is updated to " LAUNCHER_VERSION ".");
    QTimer::singleShot(0, this, [text] { emit app().message(text, false); });
    // The old files, once the old launcher has had time to end (Windows keeps
    // its program until then). Only after an update: another launcher may be
    // downloading one (and leftovers from one that stopped go when the next
    // one starts).
    QTimer::singleShot(5000, this, [this] {
      if (state_ == Idle) clean_up();
    });
  }
}

Updater::~Updater() {
  stop();
  instance_ = nullptr;
}

Updater* Updater::create(QQmlEngine*, QJSEngine*) {
  QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
  return instance_;
}

QString Updater::name() const {
  // With the news off, no update is looked for, nor offered (but one being
  // installed is still shown).
  if (state_ == Idle && !app().prefs().news) return {};
  if (!is_update(launcher_version_.toStdString(), opengp_version_.toStdString(), LAUNCHER_VERSION,
                 app().home().pkg.version))
    return {};
  return rebuilds() ? QStringLiteral("OpenGP ") + opengp_version_ : QStringLiteral("OpenGP Launcher ") + launcher_version_;
}

bool Updater::rebuilds() const {
  return !opengp_version_.isEmpty() && newer_version(opengp_version_.toStdString(), app().home().pkg.version);
}

bool Updater::canInstall() const { return self_in_home_ && !asset_url_.isEmpty() && !name().isEmpty(); }

// ---- Looking for one ----

void Updater::check() {
  // Only an installed package has an OpenGP version of its own to compare,
  // and nothing changes while an update is being installed.
  const QUrl releases(QStringLiteral(LAUNCHER_RELEASES_URL));
  const Home& home = app().home();
  if (state_ != Idle || !home.package || home.pkg.version.empty() || !app().homeError().isEmpty() ||
      releases.isEmpty())
    return;
  // https, or a local file to try it out.
  if (releases.scheme() != QLatin1String("https") && !releases.isLocalFile()) return;
  QNetworkRequest request(releases);
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("OpenGP-Launcher/" LAUNCHER_VERSION));
  request.setRawHeader("Accept", "application/vnd.github+json");
  request.setTransferTimeout(10000);
  QNetworkReply* reply = network_.get(request);
  connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received, qint64) {
    if (received > kMaxRelease) reply->abort();
  });
  connect(reply, &QNetworkReply::finished, this, [this, reply, local = releases.isLocalFile()] {
    reply->deleteLater();
    // Anything wrong (offline, no release yet, GitHub's limit of 60 an hour) offers nothing new.
    if (reply->error() != QNetworkReply::NoError) {
      qWarning("update check: %s", qPrintable(reply->errorString()));
      return;
    }
    if (state_ != Idle) return;
    QJsonParseError error;
    const QJsonObject release = QJsonDocument::fromJson(reply->readAll(), &error).object();
    if (error.error != QJsonParseError::NoError) {
      qWarning("update check: %s", qPrintable(error.errorString()));
      return;
    }
    const std::string version = release_version(release.value(QLatin1String("tag_name")).toString().toStdString());
    const QString page = release.value(QLatin1String("html_url")).toString();
    if (version.empty() || !page.startsWith(QLatin1String("https://github.com/")) ||
        release.value(QLatin1String("draft")).toBool() || release.value(QLatin1String("prerelease")).toBool()) {
      qWarning("update check: not a published release with an x.y.z tag_name and an https://github.com/ html_url");
      return;
    }
    launcher_version_ = qs(version);
    page_ = page;
    opengp_version_.clear();
    asset_name_.clear();
    asset_url_.clear();
    sha256_.clear();
    size_ = 0;
    // This computer's package: its name says OpenGP's version. It can be
    // installed only with GitHub's SHA-256 of it to check it against.
    for (const QJsonValue& value : release.value(QLatin1String("assets")).toArray()) {
      const QJsonObject asset = value.toObject();
      const QString name = asset.value(QLatin1String("name")).toString();
      const std::string opengp = package_version(name.toStdString());
      if (opengp.empty()) continue;
      opengp_version_ = qs(opengp);
      const QString url = asset.value(QLatin1String("browser_download_url")).toString();
      const QString sha256 = qs(sha256_digest(asset.value(QLatin1String("digest")).toString().toStdString()));
      const qint64 size = asset.value(QLatin1String("size")).toInteger();
      const bool url_ok = url.startsWith(QLatin1String("https://github.com/")) || (local && QUrl(url).isLocalFile());
      if (url_ok && !sha256.isEmpty() && size > 0) {
        asset_name_ = name;
        asset_url_ = url;
        sha256_ = sha256;
        size_ = size;
      } else {
        qWarning("update check: %s has no https://github.com/ browser_download_url, sha256 digest or size: offering "
                 "only the release's page", qPrintable(name));
      }
      break;
    }
    emit changed();
  });
}

// ---- Installing it ----

void Updater::install() {
  if (state_ != Idle || !canInstall()) return;
  clean_up();
  std::error_code ec;
  if (fs::exists(dir(), ec)) {
    fail(QStringLiteral("an earlier update that didn't finish left OpenGP's old files in ") +
         QDir::toNativeSeparators(qs(dir() / "old")));
    return;
  }
  if (!fs::create_directories(dir() / "new", ec)) {
    fail(QStringLiteral("could not make ") + QDir::toNativeSeparators(qs(dir())) + QStringLiteral(": ") + qs(ec.message()));
    return;
  }
  archive_ = new QFile(qs(dir() / asset_name_.toStdString()), this);
  if (!archive_->open(QIODevice::WriteOnly)) {
    fail(QStringLiteral("could not write ") + QDir::toNativeSeparators(archive_->fileName()));
    return;
  }
  hash_.reset();
  received_ = 0;
  progress_ = 0;
  stopping_ = false;
  QNetworkRequest request{QUrl(asset_url_)};
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("OpenGP-Launcher/" LAUNCHER_VERSION));
  request.setRawHeader("Accept", "application/octet-stream");
  reply_ = network_.get(request);
  connect(reply_, &QNetworkReply::readyRead, this, [this] {
    if (!take(reply_->readAll())) reply_->abort();  // ends in finish_download()
  });
  connect(reply_, &QNetworkReply::finished, this, &Updater::finish_download);
  state_ = Downloading;
  emit changed();
}

void Updater::finish_download() {
  QNetworkReply* reply = std::exchange(reply_, nullptr);
  reply->deleteLater();
  if (stopping_) return;
  const bool written = take(reply->readAll()) && archive_->flush();
  archive_->close();
  if (received_ > size_) {
    fail(QStringLiteral("the download is bigger than GitHub says it is"));
    return;
  }
  if (!written) {
    fail(QStringLiteral("could not write ") + QDir::toNativeSeparators(archive_->fileName()));
    return;
  }
  if (reply->error() != QNetworkReply::NoError) {
    fail(reply->errorString());
    return;
  }
  if (received_ != size_) {
    fail(QStringLiteral("the download ended early. Try again."));
    return;
  }
  if (QString::fromLatin1(hash_.result().toHex()) != sha256_) {
    fail(QStringLiteral("the download isn't the package GitHub has (its SHA-256 is different). Try again."));
    return;
  }
  unpack();
}

bool Updater::take(const QByteArray& data) {
  received_ += data.size();
  if (received_ > size_ || archive_->write(data) != data.size()) return false;
  hash_.addData(data);
  const double progress = static_cast<double>(received_) / static_cast<double>(size_);
  const bool shown = static_cast<int>(progress * 100) != static_cast<int>(progress_ * 100);  // the notice says the %
  progress_ = progress;
  if (shown) emit changed();
  return true;
}

void Updater::unpack() {
  state_ = Unpacking;
  emit changed();
  // The system's tar (Windows' is bsdtar, which reads zip files), with
  // relative paths: Windows' reads its arguments in the system's code page,
  // which may not have every character of the package's path.
#ifdef _WIN32
  const QString tar = qEnvironmentVariable("SystemRoot", QStringLiteral("C:\\Windows")) + QStringLiteral("\\System32\\tar.exe");
#else
  const QString tar = QStringLiteral("tar");
#endif
  process_ = new QProcess(this);
  process_->setWorkingDirectory(qs(dir()));
  process_->setProcessChannelMode(QProcess::MergedChannels);
  connect(process_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
    QProcess* process = std::exchange(process_, nullptr);
    process->deleteLater();
    if (stopping_) return;
    if (status != QProcess::NormalExit || code != 0) {
      const QString output = QString::fromLocal8Bit(process->readAll()).trimmed();
      fail(QStringLiteral("could not unpack the package") + (output.isEmpty() ? QString() : QStringLiteral(": ") + output));
      return;
    }
    check_unpacked();
  });
  connect(process_, &QProcess::errorOccurred, this, [this, tar](QProcess::ProcessError error) {
    if (error != QProcess::FailedToStart || stopping_) return;  // the others end in finished()
    process_->deleteLater();
    process_ = nullptr;
    fail(QStringLiteral("could not run ") + QDir::toNativeSeparators(tar) + QStringLiteral(" to unpack it"));
  });
  process_->start(tar, {QStringLiteral("-xf"), asset_name_, QStringLiteral("-C"), QStringLiteral("new")});
}

void Updater::check_unpacked() {
  // The package the release says, with a launcher that runs: one that
  // didn't (a library missing, a broken signature on macOS) would leave the
  // player with no launcher at all.
  const fs::path package = find_unpacked_package(dir() / "new");
  installer::Package pkg;
  std::string error;
  if (package.empty() || !installer::load_package(package, &pkg, &error)) {
    fail(QStringLiteral("the download isn't an OpenGP package") + (error.empty() ? QString() : QStringLiteral(" (") + qs(error) + QLatin1Char(')')));
    return;
  }
  if (qs(pkg.version) != opengp_version_) {
    fail(QStringLiteral("the package is OpenGP ") + qs(pkg.version) + QStringLiteral(", not ") + opengp_version_);
    return;
  }
  const QString program = qs(launcher_program(package));
  process_ = new QProcess(this);
  process_->setWorkingDirectory(qs(package));
  connect(process_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
    QProcess* process = std::exchange(process_, nullptr);
    process->deleteLater();
    if (stopping_) return;
    // "OpenGP Launcher 1.0.2 (OpenGP 1.0.2)"
    const QString said = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
    if (status != QProcess::NormalExit || code != 0 ||
        !said.startsWith(QStringLiteral("OpenGP Launcher ") + launcher_version_ + QLatin1Char(' '))) {
      qWarning("update: the new launcher said \"%s\" (exit code %d)", qPrintable(said), code);
      fail(QStringLiteral("the new launcher doesn't start on this computer"));
      return;
    }
    state_ = Waiting;
    emit changed();
    apply();
  });
  connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error != QProcess::FailedToStart || stopping_) return;  // the others end in finished()
    process_->deleteLater();
    process_ = nullptr;
    fail(QStringLiteral("the new launcher doesn't start on this computer"));
  });
#ifdef _WIN32
  // A launcher that can't load (a DLL missing) ends with an error instead of
  // Windows' message box about it, which would hold it up: the error mode is
  // the new process's too.
  const UINT error_mode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
  process_->start(program, {QStringLiteral("--version")});
  SetErrorMode(error_mode);
#else
  process_->start(program, {QStringLiteral("--version")});
#endif
  QTimer::singleShot(kVersionTimeoutMs, process_, [process = process_] { process->kill(); });  // ends in finished()
}

void Updater::apply() {
  if (state_ != Waiting) return;
  // Not while a game is built from the package's files, or played: once it ends.
  Launcher& launcher = app();
  if (!launcher.buildingId().isEmpty() || !launcher.runningId().isEmpty()) {
    connect(&launcher, &Launcher::buildingChanged, this, &Updater::apply, Qt::UniqueConnection);
    connect(&launcher, &Launcher::runningChanged, this, &Updater::apply, Qt::UniqueConnection);
    return;
  }
  disconnect(&launcher, nullptr, this, nullptr);
  state_ = Installing;
  emit changed();
  const Home& home = launcher.home();
  const fs::path package = find_unpacked_package(dir() / "new");
  std::string error;
  if (!apply_update(home.dir, package, dir() / "old", &error)) {
    fail(qs(error));
    return;
  }
  // The new launcher says what changed, and takes out the old files.
  QSaveFile done(qs(update_done_file(home.dir)));
  if (done.open(QIODevice::WriteOnly)) {
    done.write(QStringLiteral(LAUNCHER_VERSION "\n%1\n").arg(qs(home.pkg.version)).toUtf8());
    done.commit();
  }
  restart_ = true;
  // As closing the window does: the launcher saves its settings and ends,
  // then main() opens the new one (restart_if_updated).
  emit launcher.quitRequested();
}

void Updater::restart_if_updated() {
  if (!restart_) return;
  if (!QProcess::startDetached(program_, {}, qs(app().home().dir)))
    qWarning("update: could not open the new launcher, %s", qPrintable(program_));
}

void Updater::cancel() {
  if (state_ == Idle || state_ == Installing) return;
  stop();
  disconnect(&app(), nullptr, this, nullptr);
  state_ = Idle;
  emit changed();
  clean_up();
}

void Updater::fail(const QString& why) {
  const QString update = name();
  qWarning("update: %s", qPrintable(why));
  stop();
  state_ = Idle;
  emit changed();
  clean_up();
  emit app().message(QStringLiteral("Could not update to ") + update + QStringLiteral(": ") + why, true);
}

void Updater::stop() {
  stopping_ = true;
  if (reply_) {
    reply_->abort();  // finish_download() sees stopping_
  }
  if (process_) {
    process_->disconnect(this);
    process_->kill();
    process_->waitForFinished(5000);
    delete process_;
    process_ = nullptr;
  }
  if (archive_) {
    archive_->close();
    delete archive_;
    archive_ = nullptr;
  }
  stopping_ = false;
}

void Updater::clean_up() { clean_update(app().home().dir); }

}  // namespace launcher
