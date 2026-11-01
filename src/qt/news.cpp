// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt/news.h"

#include <QFile>
#include <QNetworkReply>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QSaveFile>

#include "core/news.h"
#include "qt/launcher.h"
#include "qt/updater.h"

namespace launcher {

namespace {

constexpr qint64 kMaxFeed = 256 * 1024;
constexpr int kRefreshMs = 30 * 60 * 1000;

Launcher& app() { return *Launcher::instance(); }

}  // namespace

News::News(QObject* parent) : QObject(parent) {
  instance_ = this;
  network_.setTransferTimeout(10000);
  timer_.setInterval(kRefreshMs);
  connect(&timer_, &QTimer::timeout, this, &News::refresh);
  read_changelog();
  // The copy from last time first, then the feed.
  QFile cache(cache_path());
  if (cache.open(QIODevice::ReadOnly)) show(cache.read(kMaxFeed));
  refresh();
  timer_.start();
}

News* News::create(QQmlEngine*, QJSEngine*) {
  QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
  return instance_;
}

QString News::url() const {
  const std::string& override_url = app().prefs().news_url;
  return override_url.empty() ? QStringLiteral(LAUNCHER_NEWS_URL) : QString::fromStdString(override_url);
}

QString News::cache_path() const {
  // No OpenGP (no games): no user folder to keep it in.
  return app().home().games.empty() ? QString() : QString::fromStdString(utf8(app().home().user_dir / "news.toml"));
}

bool News::enabled() const { return app().prefs().news; }

void News::setEnabled(bool enabled) {
  if (enabled == app().prefs().news) return;
  app().prefs().news = enabled;
  app().save_prefs();
  emit changed();
  if (Updater* updater = Updater::instance()) emit updater->changed();  // offered only with the news on
  if (enabled) refresh();
}

void News::refresh() {
  if (!enabled()) return;
  if (Updater* updater = Updater::instance()) updater->check();
  const QUrl feed(url());
  if (feed.isEmpty()) return;
  if (feed.isLocalFile()) {  // for trying a feed out (launcher.toml [news] url)
    QFile f(feed.toLocalFile());
    if (f.open(QIODevice::ReadOnly) && show(f.read(kMaxFeed))) offline_ = false;
    emit changed();
    return;
  }
  if (feed.scheme() != QLatin1String("https")) return;
  QNetworkRequest request(feed);
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("OpenGP-Launcher/" LAUNCHER_VERSION));
  if (!etag_.isEmpty()) request.setRawHeader("If-None-Match", etag_);
  QNetworkReply* reply = network_.get(request);
  connect(reply, &QNetworkReply::downloadProgress, reply, [reply](qint64 received, qint64) {
    if (received > kMaxFeed) reply->abort();
  });
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::NoError && status == 304) {
      offline_ = false;
    } else if (reply->error() == QNetworkReply::NoError) {
      const QByteArray text = reply->readAll();
      if (show(text)) {
        offline_ = false;
        etag_ = reply->rawHeader("ETag");
        QSaveFile cache(cache_path());
        if (cache.open(QIODevice::WriteOnly)) {
          cache.write(text);
          cache.commit();
        }
      }
    } else {
      offline_ = true;
    }
    emit changed();
  });
}

bool News::show(const QByteArray& text) {
  std::vector<NewsItem> items;
  int skipped = 0;
  std::string error;
  if (!parse_news(text.toStdString(), &items, &skipped, &error)) {
    qWarning("news: %s", error.c_str());
    return false;
  }
  if (skipped) qWarning("news: %d items left out (no id, date or title, or a link that is not https)", skipped);
  items_ = std::move(items);
  emit changed();
  return true;
}

QVariantList News::items() const {
  QVariantList out;
  const auto& seen = app().prefs().news_seen;
  for (const NewsItem& i : items_)
    out << QVariantMap{{QStringLiteral("id"), QString::fromStdString(i.id)},
                       {QStringLiteral("date"), QString::fromStdString(i.date)},
                       {QStringLiteral("title"), QString::fromStdString(i.title)},
                       {QStringLiteral("tag"), QString::fromStdString(i.tag)},
                       {QStringLiteral("link"), QString::fromStdString(i.link)},
                       {QStringLiteral("body"), QString::fromStdString(i.body)},
                       {QStringLiteral("pinned"), i.pinned},
                       {QStringLiteral("unread"), !seen.count(i.id)}};
  return out;
}

int News::unread() const { return unread_news(items_, app().prefs().news_seen); }

void News::markAllSeen() {
  if (!unread()) return;
  // Only the feed's items: old ids drop out of the file.
  std::set<std::string> seen;
  for (const NewsItem& i : items_) seen.insert(i.id);
  app().prefs().news_seen = std::move(seen);
  app().save_prefs();
  emit changed();
}

void News::read_changelog() {
  // The newest released section: "## 0.3.0 (2026-10-01)" (or "## [0.3.0] ...") up to the next.
  QFile f(QString::fromStdString(utf8(app().home().dir / "CHANGELOG.md")));
  if (!f.open(QIODevice::ReadOnly)) return;
  const QString text = QString::fromUtf8(f.readAll());
  static const QRegularExpression heading(QStringLiteral("^## +(.+)$"), QRegularExpression::MultilineOption);
  auto it = heading.globalMatch(text);
  while (it.hasNext()) {
    const QRegularExpressionMatch m = it.next();
    if (m.captured(1).trimmed().startsWith(QLatin1String("Unreleased"), Qt::CaseInsensitive)) continue;
    const qsizetype start = m.capturedEnd();
    const qsizetype end = it.hasNext() ? it.peekNext().capturedStart() : text.size();
    whats_new_title_ = QStringLiteral("What's new in OpenGP ") + m.captured(1).trimmed();
    // As plain reading: no code spans, and links only to the web.
    QString section = text.mid(start, end - start).trimmed();
    static const QRegularExpression local_link(QStringLiteral(R"(\[([^\]]+)\]\((?!https://)[^)]*\))"));
    section.replace(local_link, QStringLiteral("\\1"));
    section.remove(QLatin1Char('`'));
    whats_new_ = section;
    return;
  }
}

}  // namespace launcher
