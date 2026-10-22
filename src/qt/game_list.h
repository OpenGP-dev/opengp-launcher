// SPDX-License-Identifier: GPL-3.0-or-later
//
// The games, for the launcher's rail and main pane.
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QtQmlIntegration>

namespace launcher {

class GameList : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Launcher.games")
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

 public:
  enum Role {
    IdRole = Qt::UserRole + 1,
    TitleRole,
    ShortTitleRole,
    StateRole,  // Launcher.State
    ImageRole,
    ExeRole,
    LogoRole,   // file URLs, "" if none
    TileRole,
    HeroesRole,
    DumpCheckRole,  // the image against the known good dumps: Launcher.DumpState
    DumpNameRole,   // the known good dump it is, or should be
    DumpNoteRole,   // what is wrong with it, or "" if nothing
    ImageErrorRole, // why the last disc image chosen for it could not be added, or ""
  };

  struct Entry {
    QString id, title, shortTitle, exe, image;
    int state = 0;
    QString logo, tile;
    QStringList heroes;
    int dump_check = -1;  // Launcher::DumpState; -1 (NotChecked) until it is
    QString dump_name, dump_note;
    QString image_error;
  };

  using QAbstractListModel::QAbstractListModel;

  int rowCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  // For QML: one game's entry as an object ({id, title, state, ...}).
  Q_INVOKABLE QVariantMap get(int row) const;

  const Entry& at(int row) const { return entries_[row]; }
  void reset(QList<Entry> entries);
  void update(int row, const Entry& entry);

 signals:
  void countChanged();

 private:
  QList<Entry> entries_;
};

}  // namespace launcher
