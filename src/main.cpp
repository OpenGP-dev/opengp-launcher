// SPDX-License-Identifier: GPL-3.0-or-later
//
// The OpenGP launcher: play Mario Kart Arcade GP 2 and GP 1 on OpenGP, change
// their settings and controls, add the disc images and build the games, and
// look after the saves.
//
//   OpenGP [--home DIR] [--user-dir DIR] [--game ID] [--play]
//     --play starts the game (the last one chosen, or --game) once it is found ready
//   OpenGP --version
//   OpenGP --smoke-test                 load the interface and quit (0 if it loaded without warnings)
//   OpenGP --screenshot-ui FILE [--view VIEW]   save a picture of the window and quit
//     VIEW: home, play, saves, settings/<page>, news (on Home)

#include <QCommandLineParser>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <cstdio>
#include <cstring>

#include "core/home.h"
#include "qt/controls.h"
#include "qt/game_settings.h"
#include "qt/launcher.h"
#include "qt/news.h"
#include "qt/user_data_model.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace launcher;

namespace {

// The launcher is a windowed program; on Windows, print to the console it
// was started from, if any (for --version and the tests).
void attach_console() {
#ifdef _WIN32
  const bool redirected = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) != FILE_TYPE_UNKNOWN;
  if (!redirected && AttachConsole(ATTACH_PARENT_PROCESS)) {
    std::freopen("CONOUT$", "w", stdout);
    std::freopen("CONOUT$", "w", stderr);
  }
#endif
}

fs::path path_of(const QString& s) { return fs::path(s.toStdU16String()); }

}  // namespace

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], "--version") == 0) {
      attach_console();
      std::printf("OpenGP Launcher %s (OpenGP %s)\n", LAUNCHER_VERSION, OPENGP_VERSION);
      return 0;
    }

  QGuiApplication app(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("OpenGP"));
  QGuiApplication::setApplicationVersion(QStringLiteral(LAUNCHER_VERSION));
  QQuickStyle::setStyle(QStringLiteral("Basic"));
  // The launcher's typeface (M PLUS Rounded 1c, OFL; Latin only: other
  // scripts come from the system's fonts).
  for (const char* weight : {"Regular", "ExtraBold"})
    QFontDatabase::addApplicationFont(QStringLiteral(":/qt/qml/OpenGP/Launcher/fonts/MPLUSRounded1c-%1.ttf").arg(QLatin1String(weight)));
  QFont font(QStringLiteral("Rounded Mplus 1c"));
  font.setWeight(QFont::Normal);
  QGuiApplication::setFont(font);
  QPalette palette = QGuiApplication::palette();
  palette.setColor(QPalette::Link, QColor(0x3B, 0x9B, 0xEF));  // Theme.blue, for links in Markdown
  QGuiApplication::setPalette(palette);

  QCommandLineParser args;
  args.setApplicationDescription(QStringLiteral("The OpenGP launcher"));
  args.addHelpOption();
  const QCommandLineOption home_opt(QStringLiteral("home"), QStringLiteral("The OpenGP folder to look after."),
                                    QStringLiteral("dir"));
  const QCommandLineOption user_opt(QStringLiteral("user-dir"),
                                    QStringLiteral("Settings and saves (default: <home>/user)."), QStringLiteral("dir"));
  const QCommandLineOption smoke_opt(QStringLiteral("smoke-test"), QStringLiteral("Load the interface and quit."));
  const QCommandLineOption shot_opt(QStringLiteral("screenshot-ui"), QStringLiteral("Save a picture of the window and quit."),
                                    QStringLiteral("file"));
  const QCommandLineOption game_opt(QStringLiteral("game"), QStringLiteral("The game to show first (its ID)."),
                                    QStringLiteral("id"));
  const QCommandLineOption view_opt(QStringLiteral("view"), QStringLiteral("What to show first (home, play, saves, news, settings/<page>)."),
                                    QStringLiteral("view"));
  const QCommandLineOption play_opt(QStringLiteral("play"), QStringLiteral("Start the game at once (for shortcuts)."));
  args.addOptions({home_opt, user_opt, smoke_opt, shot_opt, game_opt, view_opt, play_opt});
  args.process(app);
  if (args.isSet(smoke_opt) || args.isSet(shot_opt)) attach_console();

  // The OpenGP to look after.
  const fs::path exe_dir = path_of(QCoreApplication::applicationDirPath());
  const fs::path dir = args.isSet(home_opt) ? path_of(QDir(args.value(home_opt)).absolutePath())
                                            : default_home(exe_dir, path_of(QStringLiteral(LAUNCHER_DEV_HOME)));
  const fs::path user_dir = args.isSet(user_opt) ? path_of(QDir(args.value(user_opt)).absolutePath()) : fs::path();
  Home home;
  std::string error;
  QString home_error;
  if (!open_home(dir, user_dir, &home, &error)) {
    home_error = QString::fromStdString(error);
    home.dir = dir;
    home.user_dir = user_dir.empty() ? dir / "user" : user_dir;
  }
  std::error_code ec;
  fs::create_directories(home.user_dir, ec);

  const QString prefs_path = QString::fromStdString(utf8(home.user_dir / "launcher.toml"));
  Launcher launcher(std::move(home), home_error, prefs_path);
  GameSettings settings(QString::fromStdString(utf8(launcher.home().user_dir / "settings.toml")));
  Controls controls(QString::fromStdString(utf8(launcher.home().user_dir / "controls.toml")));
  if (args.isSet(game_opt))
    for (int i = 0; i < launcher.games()->rowCount(); ++i)
      if (launcher.games()->at(i).id == args.value(game_opt)) launcher.setCurrentIndex(i);
  UserData user_data;
  News news;

  // --play: once the first look at the games is done.
  if (args.isSet(play_opt)) {
    auto* once = new QObject(&launcher);
    QObject::connect(&launcher, &Launcher::checkingChanged, once, [&launcher, once] {
      if (launcher.checking()) return;
      const int row = launcher.currentIndex();
      if (launcher.games()->at(row).state == Launcher::Ready) launcher.play(row);
      else emit launcher.message(launcher.games()->at(row).title + QStringLiteral(" isn't ready to play yet."), true);
      once->deleteLater();
    });
  }

  // A running game has the controllers and the settings files.
  QObject::connect(&launcher, &Launcher::gameStarting, [&] {
    controls.suspend();
    settings.set_locked(true);
  });
  QObject::connect(&launcher, &Launcher::gameEnded, [&] {
    controls.resume();
    settings.set_locked(false);
    user_data.refresh();
  });
  QObject::connect(&launcher, &Launcher::quitRequested, &app, &QCoreApplication::quit, Qt::QueuedConnection);

  // The window's icon: the main game's picture, from the player's disc.
  auto set_icon = [&] {
    if (launcher.games()->rowCount())
      if (const QString tile = launcher.games()->at(0).tile; !tile.isEmpty()) QGuiApplication::setWindowIcon(QIcon(tile));
  };
  QObject::connect(launcher.games(), &GameList::dataChanged, set_icon);
  set_icon();

  QQmlApplicationEngine engine;
  QStringList warnings;
  QObject::connect(&engine, &QQmlEngine::warnings, [&](const QList<QQmlError>& list) {
    for (const QQmlError& w : list) {
      warnings << w.toString();
      std::fprintf(stderr, "%s\n", qPrintable(w.toString()));
    }
  });
  engine.setInitialProperties({{QStringLiteral("startView"), args.value(view_opt)},
                               {QStringLiteral("width"), launcher.prefs().window_width},
                               {QStringLiteral("height"), launcher.prefs().window_height}});
  engine.loadFromModule("OpenGP.Launcher", "Main");
  if (engine.rootObjects().isEmpty()) return 1;
  auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());

  if (args.isSet(smoke_opt)) {
    QTimer::singleShot(1000, &app, [&] { app.exit(warnings.isEmpty() ? 0 : 1); });
  } else if (args.isSet(shot_opt)) {
    // Give the games' check and the pictures a moment.
    QTimer::singleShot(2500, &app, [&] {
      const bool saved = window && window->grabWindow().save(args.value(shot_opt));
      std::printf("%s %s\n", saved ? "saved" : "could not save", qPrintable(args.value(shot_opt)));
      app.exit(saved ? 0 : 1);
    });
  }

  const int code = app.exec();
  if (window && !args.isSet(shot_opt) && !args.isSet(smoke_opt) && window->visibility() == QWindow::Windowed) {
    launcher.prefs().window_width = window->width();
    launcher.prefs().window_height = window->height();
    launcher.save_prefs();
  }
  return code;
}
