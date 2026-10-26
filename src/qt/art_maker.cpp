// SPDX-License-Identifier: GPL-3.0-or-later

#include "qt/art_maker.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QSaveFile>
#include <QtEndian>

#include "core/art.h"
#include "disc/disc.h"

namespace launcher {

namespace {

// A file from the disc, unwrapped if the game wrapped it in zlib.
bool read_disc_file(const disc::Image& disc, const std::string& name, std::vector<std::uint8_t>* out) {
  if (!disc.read_file(name, out)) return false;
  if (const std::uint32_t size = wrapped_size(*out)) {
    // qUncompress wants the size first, big-endian, then the zlib stream.
    QByteArray z(4, 0);
    qToBigEndian<quint32>(size, z.data());
    z.append(reinterpret_cast<const char*>(out->data() + 8), static_cast<qsizetype>(out->size() - 8));
    const QByteArray raw = qUncompress(z);
    if (raw.size() != static_cast<qsizetype>(size)) return false;
    out->assign(raw.begin(), raw.end());
  }
  return true;
}

bool read_tpl(const disc::Image& disc, const std::string& name, std::vector<Picture>* pictures) {
  std::vector<std::uint8_t> data;
  return read_disc_file(disc, name, &data) && decode_tpl(data, pictures, nullptr) && !pictures->empty();
}

// A texture with GP 2's separate alpha mask (<name>_a.tpl) applied.
bool read_masked(const disc::Image& disc, const std::string& stem, Picture* out) {
  std::vector<Picture> colour, mask;
  if (!read_tpl(disc, stem + ".tpl", &colour)) return false;
  if (read_tpl(disc, stem + "_a.tpl", &mask)) apply_mask(&colour[0], mask[0]);
  *out = std::move(colour[0]);
  return true;
}

bool read_screen(const disc::Image& disc, const std::string& name, Picture* out) {
  std::vector<Picture> tiles;
  if (!read_tpl(disc, name, &tiles)) return false;
  std::vector<Picture> screens = stitch_screens(tiles);
  if (screens.empty()) return false;
  *out = std::move(screens[0]);
  return true;
}

bool save(const Picture& p, const QString& path) {
  if (p.width == 0 || p.height == 0) return false;
  QImage image(reinterpret_cast<const uchar*>(p.pixels.data()), static_cast<int>(p.width),
               static_cast<int>(p.height), static_cast<qsizetype>(p.width) * 4, QImage::Format_RGBA8888);
  return image.save(path, "PNG");
}

QString stamp_text(const QString& image) {
  return QStringLiteral("version = %1\nimage = \"%2\"\n")
      .arg(kArtVersion)
      .arg(QString(image).replace(QLatin1Char('\\'), QLatin1String("\\\\")).replace(QLatin1Char('"'), QLatin1String("\\\"")));
}

}  // namespace

GameArt existing_art(const QString& art_dir, const QString& image) {
  GameArt art;
  QFile stamp(art_dir + QStringLiteral("/art.toml"));
  if (!stamp.open(QIODevice::ReadOnly) || QString::fromUtf8(stamp.readAll()) != stamp_text(image)) return art;
  art.current = true;
  const QDir dir(art_dir);
  if (dir.exists(QStringLiteral("logo.png"))) art.logo = dir.filePath(QStringLiteral("logo.png"));
  if (dir.exists(QStringLiteral("tile.png"))) art.tile = dir.filePath(QStringLiteral("tile.png"));
  for (const QString& f : dir.entryList({QStringLiteral("hero_*.png")}, QDir::Files, QDir::Name))
    art.heroes << dir.filePath(f);
  return art;
}

bool make_art(const QString& image, const QString& game_id, const QString& art_dir, QString* error) {
  disc::Image disc;
  std::string err;
  if (!disc.open(image.toStdString(), &err)) {
    if (error) *error = QString::fromStdString(err);
    return false;
  }
  QDir dir(art_dir);
  dir.removeRecursively();
  if (!QDir().mkpath(art_dir)) {
    if (error) *error = QStringLiteral("cannot create ") + art_dir;
    return false;
  }

  Picture logo, tile;
  if (game_id == QLatin1String("GGPE02")) {
    // GP 2: the logo from the attract mode's logo sheet (the one without
    // flames), and Mario and Wario racing from an ending picture (inside its
    // frame).
    Picture sheet;
    if (read_masked(disc, "tpl2_at_sp_c27_logoset", &sheet)) logo = trim(crop(sheet, 55, 206, 457, 116));
    std::vector<Picture> ending;
    if (read_tpl(disc, "tpl2_sp_ed_100cc_07.tpl", &ending)) tile = crop(ending[0], 14, 80, 228, 228);
  } else if (game_id == QLatin1String("GGPE01")) {
    // GP 1: the logo cut from the title screen (off its white background),
    // and Pac-Man, Mario and Bowser racing from the character select.
    Picture title;
    if (read_screen(disc, "E_sp_title.tpl", &title)) {
      logo = crop(title, 60, 36, 520, 162);
      key_out_background(&logo);
      logo = trim(logo);
    }
    std::vector<Picture> racers;
    if (read_tpl(disc, "sprite_sel_3_5.tpl", &racers)) tile = crop(racers[0], 64, 0, 128, 128);
  }
  save(logo, dir.filePath(QStringLiteral("logo.png")));
  save(tile, dir.filePath(QStringLiteral("tile.png")));

  // The course previews: course_<cup><letter>.tpl.
  int heroes = 0;
  for (char cup = '1'; cup <= '8'; ++cup)
    for (char letter = 'a'; letter <= 'h'; ++letter) {
      const std::string name = std::string("course_") + cup + letter;
      Picture screen;
      if (read_screen(disc, name + ".tpl", &screen) &&
          save(screen, dir.filePath(QStringLiteral("hero_%1.png").arg(QString::fromStdString(name.substr(7))))))
        ++heroes;
    }

  QSaveFile stamp(dir.filePath(QStringLiteral("art.toml")));
  if (!stamp.open(QIODevice::WriteOnly) || stamp.write(stamp_text(image).toUtf8()) < 0 || !stamp.commit()) {
    if (error) *error = QStringLiteral("cannot write ") + stamp.fileName();
    return false;
  }
  if (logo.width == 0 && tile.width == 0 && heroes == 0) {
    if (error) *error = QStringLiteral("no pictures found on the disc");
    return false;
  }
  return true;
}

}  // namespace launcher
