// SPDX-License-Identifier: GPL-3.0-or-later
//
// The current game's saves and data for QML: its Mario Cards, its own saved
// data, backups of user/, screenshots and its last log.
#pragma once

#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QtQmlIntegration>

class QQmlEngine;
class QJSEngine;

namespace launcher {

class UserData : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(QStringList cards READ cards NOTIFY changed)
  Q_PROPERTY(QVariantList backups READ backups NOTIFY changed)  // [{name, size}]
  Q_PROPERTY(QStringList screenshots READ screenshots NOTIFY changed)  // file URLs, newest first
  Q_PROPERTY(bool hasGameData READ hasGameData NOTIFY changed)
  Q_PROPERTY(QString log READ log NOTIFY changed)
  Q_PROPERTY(bool locked READ locked NOTIFY changed)  // a game is running

 public:
  explicit UserData(QObject* parent = nullptr);
  static UserData* create(QQmlEngine*, QJSEngine*);

  QStringList cards() const { return cards_; }
  QVariantList backups() const { return backups_; }
  QStringList screenshots() const { return screenshots_; }
  bool hasGameData() const { return has_game_data_; }
  QString log() const { return log_; }
  bool locked() const;

  Q_INVOKABLE void refresh();
  // A new card: the next free name becomes the card the player holds; the game
  // saves it when it hands the card out.
  Q_INVOKABLE void newCard();
  // False if it could not (the reason is shown).
  Q_INVOKABLE bool renameCard(const QString& from, const QString& to);
  Q_INVOKABLE void duplicateCard(const QString& name);
  Q_INVOKABLE void deleteCard(const QString& name);
  Q_INVOKABLE void importCard(const QUrl& file);
  Q_INVOKABLE void exportCard(const QString& name, const QUrl& file);
  Q_INVOKABLE void backUp();
  Q_INVOKABLE void restoreBackup(const QString& name);
  Q_INVOKABLE void deleteBackup(const QString& name);
  Q_INVOKABLE void resetGameData();

 signals:
  void changed();

 private:
  bool report(bool ok, const std::string& error, const QString& done);

  static inline UserData* instance_ = nullptr;
  QStringList cards_, screenshots_;
  QVariantList backups_;
  bool has_game_data_ = false;
  QString log_;
};

}  // namespace launcher
