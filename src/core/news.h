// SPDX-License-Identifier: GPL-3.0-or-later
//
// The news feed: announcements written by hand in news.toml (a public gist;
// see docs/news.md), which the launcher downloads and keeps a copy of.
//
//   [[news]]
//   id = "2026-10-16-launcher"   # unique; "unread" goes by it
//   date = 2026-10-16
//   title = "The new launcher"
//   tag = "update"               # update, announcement or event
//   pinned = false               # pinned items come first
//   link = "https://..."         # optional; https only
//   body = """
//   Markdown text.
//   """
#pragma once

#include <set>
#include <string>
#include <vector>

namespace launcher {

struct NewsItem {
  std::string id, date, title, tag, link, body;  // date: "YYYY-MM-DD"
  bool pinned = false;
};

// Reads a feed. Items without an id, date or title, or with a link that is
// not https, are left out (and counted in `skipped`). Fails only if the text
// is not TOML or has no news. The items come pinned first, then newest first.
bool parse_news(const std::string& text, std::vector<NewsItem>* items, int* skipped, std::string* error);

// How many of the items the player has not seen.
int unread_news(const std::vector<NewsItem>& items, const std::set<std::string>& seen);

// For the notice that a newer OpenGP is out (qt/news.h): the version of a
// release from its tag, "1.0.2" from "v1.0.2" or "1.0.2", or "" if the tag is
// not three numbers (a prerelease's "1.1.0-rc1" is not).
std::string release_version(const std::string& tag);

// Whether version `a` is later than `b`, number by number (1.0.10 is later
// than 1.0.9); false if either is not a version as release_version reads it.
bool newer_version(const std::string& a, const std::string& b);

}  // namespace launcher
