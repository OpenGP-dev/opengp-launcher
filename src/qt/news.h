// SPDX-License-Identifier: GPL-3.0-or-later
//
// The news feed for QML: downloaded (news.toml, see docs/news.md and
// core/news.h) when the launcher starts and every half hour, kept in
// user/news.toml for when the feed cannot be reached. With no news at all, the
// newest section of OpenGP's CHANGELOG.md stands in. At the same times, in an
// installed package, OpenGP's latest release (LAUNCHER_RELEASES_URL) for the
// notice on Home that a newer OpenGP is out. Turning the news off turns both off.
#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQmlIntegration>
#include <vector>

#include "core/news.h"

class QQmlEngine;
class QJSEngine;

namespace launcher {

class News : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(QVariantList items READ items NOTIFY changed)  // [{id, date, title, tag, link, body, pinned, unread}]
  Q_PROPERTY(int unread READ unread NOTIFY changed)
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
  Q_PROPERTY(bool offline READ offline NOTIFY changed)       // showing the copy kept from before
  Q_PROPERTY(QString whatsNew READ whatsNew NOTIFY changed)  // the changelog's newest section (Markdown), when there is no news
  Q_PROPERTY(QString whatsNewTitle READ whatsNewTitle NOTIFY changed)
  Q_PROPERTY(QString newVersion READ newVersion NOTIFY changed)  // a newer OpenGP than the package's ("1.0.2"), or ""
  Q_PROPERTY(QString newVersionPage READ newVersionPage NOTIFY changed)  // its release page

 public:
  // No default parent: with a default constructor QML would make a second
  // News of its own (fetching everything twice) instead of calling create().
  explicit News(QObject* parent);
  static News* create(QQmlEngine*, QJSEngine*);

  QVariantList items() const;
  int unread() const;
  bool enabled() const;
  void setEnabled(bool enabled);
  bool offline() const { return offline_; }
  QString whatsNew() const { return whats_new_; }
  QString whatsNewTitle() const { return whats_new_title_; }
  QString newVersion() const { return enabled() ? new_version_ : QString(); }
  QString newVersionPage() const { return enabled() ? new_version_page_ : QString(); }

  Q_INVOKABLE void refresh();
  Q_INVOKABLE void markAllSeen();

 signals:
  void changed();

 private:
  QString url() const;
  QString cache_path() const;
  bool show(const QByteArray& text);
  void read_changelog();
  void check_release();

  static inline News* instance_ = nullptr;
  QNetworkAccessManager network_;
  QTimer timer_;
  std::vector<NewsItem> items_;
  bool offline_ = false;
  QByteArray etag_;
  QString whats_new_, whats_new_title_;
  QString new_version_, new_version_page_;
};

}  // namespace launcher
