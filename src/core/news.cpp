// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/news.h"

#include <algorithm>
#include <cstdio>

#define TOML_EXCEPTIONS 0
#include <toml.hpp>

namespace launcher {

namespace {

// A TOML date, or a "YYYY-MM-DD" string.
std::string date_text(const toml::node_view<const toml::node>& node) {
  if (auto d = node.value<toml::date>()) {
    char text[16];
    std::snprintf(text, sizeof(text), "%04d-%02d-%02d", d->year, d->month, d->day);
    return text;
  }
  const std::string s = node.value_or(std::string());
  const bool ok = s.size() == 10 && s[4] == '-' && s[7] == '-' &&
                  std::all_of(s.begin(), s.end(), [](char c) { return (c >= '0' && c <= '9') || c == '-'; });
  return ok ? s : std::string();
}

}  // namespace

bool parse_news(const std::string& text, std::vector<NewsItem>* items, int* skipped, std::string* error) {
  toml::parse_result parsed = toml::parse(text);
  if (!parsed) {
    if (error) *error = std::string(parsed.error().description());
    return false;
  }
  const toml::array* news = parsed.table()["news"].as_array();
  if (!news) {
    if (error) *error = "no [[news]] items";
    return false;
  }
  std::vector<NewsItem> out;
  int bad = 0;
  for (const toml::node& node : *news) {
    const toml::table* t = node.as_table();
    if (!t) {
      ++bad;
      continue;
    }
    const toml::table& v = *t;
    NewsItem item;
    item.id = v["id"].value_or(std::string());
    item.date = date_text(v["date"]);
    item.title = v["title"].value_or(std::string());
    item.tag = v["tag"].value_or(std::string("announcement"));
    item.link = v["link"].value_or(std::string());
    item.body = v["body"].value_or(std::string());
    item.pinned = v["pinned"].value_or(false);
    const bool link_ok = item.link.empty() || item.link.rfind("https://", 0) == 0;
    const bool duplicate =
        std::any_of(out.begin(), out.end(), [&](const NewsItem& other) { return other.id == item.id; });
    if (item.id.empty() || item.date.empty() || item.title.empty() || !link_ok || duplicate) {
      ++bad;
      continue;
    }
    out.push_back(std::move(item));
  }
  std::stable_sort(out.begin(), out.end(), [](const NewsItem& a, const NewsItem& b) {
    if (a.pinned != b.pinned) return a.pinned;
    return a.date > b.date;
  });
  *items = std::move(out);
  if (skipped) *skipped = bad;
  return true;
}

int unread_news(const std::vector<NewsItem>& items, const std::set<std::string>& seen) {
  return static_cast<int>(std::count_if(items.begin(), items.end(), [&](const NewsItem& i) { return !seen.count(i.id); }));
}

}  // namespace launcher
