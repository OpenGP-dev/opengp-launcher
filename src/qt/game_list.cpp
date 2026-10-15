// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt/game_list.h"

#include <QUrl>

namespace launcher {

namespace {

QString file_url(const QString& path) { return path.isEmpty() ? QString() : QUrl::fromLocalFile(path).toString(); }

}  // namespace

int GameList::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : static_cast<int>(entries_.size()); }

QVariant GameList::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() >= entries_.size()) return {};
  const Entry& e = entries_[index.row()];
  switch (role) {
    case IdRole: return e.id;
    case TitleRole: return e.title;
    case ShortTitleRole: return e.shortTitle;
    case StateRole: return e.state;
    case ImageRole: return e.image;
    case ExeRole: return e.exe;
    case LogoRole: return file_url(e.logo);
    case TileRole: return file_url(e.tile);
    case HeroesRole: {
      QStringList urls;
      for (const QString& h : e.heroes) urls << file_url(h);
      return urls;
    }
    default: return {};
  }
}

QHash<int, QByteArray> GameList::roleNames() const {
  return {{IdRole, "gameId"},   {TitleRole, "title"},  {ShortTitleRole, "shortTitle"},
          {StateRole, "gameState"}, {ImageRole, "image"},  {ExeRole, "exe"},
          {LogoRole, "logo"},   {TileRole, "tile"},    {HeroesRole, "heroes"}};
}

QVariantMap GameList::get(int row) const {
  QVariantMap map;
  if (row < 0 || row >= entries_.size()) return map;
  const auto roles = roleNames();
  for (auto it = roles.begin(); it != roles.end(); ++it)
    map.insert(QString::fromUtf8(it.value()), data(index(row), it.key()));
  return map;
}

void GameList::reset(QList<Entry> entries) {
  beginResetModel();
  entries_ = std::move(entries);
  endResetModel();
  emit countChanged();
}

void GameList::update(int row, const Entry& entry) {
  if (row < 0 || row >= entries_.size()) return;
  entries_[row] = entry;
  emit dataChanged(index(row), index(row));
}

}  // namespace launcher
