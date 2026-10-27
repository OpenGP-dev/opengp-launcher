// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt/user_data_model.h"

#include <QFile>
#include <QLocale>
#include <QQmlEngine>

#include "core/user_data.h"
#include "qt/controls.h"
#include "qt/game_settings.h"
#include "qt/launcher.h"
#include "triforce/card_folder.h"

namespace launcher
{

  namespace
  {

    QString qs(const std::string &s) { return QString::fromStdString(s); }
    fs::path path_of(const QUrl &url) { return fs::path(url.toLocalFile().toStdU16String()); }

    Launcher &app() { return *Launcher::instance(); }
    const fs::path &user_dir() { return app().home().user_dir; }
    std::string game_id() { return app().currentId().toStdString(); }

  } // namespace

  UserData::UserData(QObject *parent) : QObject(parent)
  {
    instance_ = this;
    connect(&app(), &Launcher::currentChanged, this, &UserData::refresh);
    connect(&app(), &Launcher::runningChanged, this, &UserData::refresh);
    refresh();
  }

  UserData *UserData::create(QQmlEngine *, QJSEngine *)
  {
    QJSEngine::setObjectOwnership(instance_, QJSEngine::CppOwnership);
    return instance_;
  }

  bool UserData::locked() const { return !app().runningId().isEmpty(); }

  void UserData::refresh()
  {
    const std::string id = game_id();
    cards_.clear();
    for (const std::string &c : list_cards(user_dir(), id))
      cards_ << qs(c);
    backups_.clear();
    for (const Backup &b : list_backups(user_dir()))
      backups_ << QVariantMap{{QStringLiteral("name"), qs(b.name)},
                              {QStringLiteral("size"), QLocale().formattedDataSize(static_cast<qint64>(b.bytes))}};
    screenshots_.clear();
    for (const fs::path &p : list_screenshots(user_dir()))
      screenshots_ << QUrl::fromLocalFile(QString::fromStdString(utf8(p))).toString();
    has_game_data_ = !game_data_files(user_dir(), id).empty();
    log_.clear();
    QFile f(QString::fromStdString(utf8(log_path(user_dir(), id))));
    if (f.open(QIODevice::ReadOnly))
    {
      // The end of it, if it is long.
      constexpr qint64 kMax = 64 * 1024;
      if (f.size() > kMax)
        f.seek(f.size() - kMax);
      log_ = QString::fromUtf8(f.readAll());
    }
    emit changed();
  }

  bool UserData::report(bool ok, const std::string &error, const QString &done)
  {
    if (!ok)
      emit app().message(qs(error), true);
    else if (!done.isEmpty())
      emit app().message(done, false);
    refresh();
    return ok;
  }

  void UserData::newCard()
  {
    if (locked())
      return;
    const std::string name =
        triforce::CardFolder(utf8(user_dir() / "cards" / game_id())).unused_name();
    GameSettings::instance()->setCard(qs(name));
    emit app().message(qs(name) + QStringLiteral(" will be saved when the game gives you a new card."), false);
    refresh();
  }

  bool UserData::renameCard(const QString &from, const QString &to)
  {
    if (locked())
      return false;
    std::string error;
    if (!report(rename_card(user_dir(), game_id(), from.toStdString(), to.trimmed().toStdString(), &error), error, {}))
      return false;
    if (GameSettings::instance()->card() == from)
      GameSettings::instance()->setCard(to.trimmed());
    return true;
  }

  void UserData::duplicateCard(const QString &name)
  {
    if (locked())
      return;
    std::string error;
    const std::string copy = duplicate_card(user_dir(), game_id(), name.toStdString(), &error);
    report(!copy.empty(), error, QStringLiteral("Copied to ") + qs(copy) + QStringLiteral("."));
  }

  void UserData::deleteCard(const QString &name)
  {
    if (locked())
      return;
    std::string error;
    report(delete_card(user_dir(), game_id(), name.toStdString(), &error), error, name + QStringLiteral(" deleted."));
  }

  void UserData::importCard(const QUrl &file)
  {
    if (locked())
      return;
    std::string error;
    const std::string name = import_card(user_dir(), game_id(), path_of(file), &error);
    report(!name.empty(), error, QStringLiteral("Added the card ") + qs(name) + QStringLiteral("."));
  }

  void UserData::exportCard(const QString &name, const QUrl &file)
  {
    std::string error;
    report(export_card(user_dir(), game_id(), name.toStdString(), path_of(file), &error), error,
           name + QStringLiteral(" saved."));
  }

  void UserData::backUp()
  {
    std::string error;
    const std::string name = back_up(user_dir(), "", &error);
    report(!name.empty(), error, QStringLiteral("Backed up as ") + qs(name) + QStringLiteral("."));
  }

  void UserData::restoreBackup(const QString &name)
  {
    if (locked())
      return;
    std::string error;
    if (report(restore_backup(user_dir(), name.toStdString(), &error), error,
               QStringLiteral("Restored ") + name + QStringLiteral(" (Previous files backed up automatically).")))
    {
      GameSettings::instance()->reload();
      Controls::instance()->reload();
      app().refresh();
    }
  }

  void UserData::deleteBackup(const QString &name)
  {
    std::string error;
    report(delete_backup(user_dir(), name.toStdString(), &error), error, QStringLiteral("Backup deleted."));
  }

  void UserData::resetGameData()
  {
    if (locked())
      return;
    std::string error;
    report(reset_game_data(user_dir(), game_id(), &error), error,
           QStringLiteral("Reset Game data (Backup created beforehand)."));
  }

} // namespace launcher
