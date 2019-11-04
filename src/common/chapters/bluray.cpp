/*
  mkvmerge -- utility for splicing together matroska files
  from component media subtypes

  Distributed under the GPL v2
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html

  helper functions for chapters on Blu-rays

  Written by Moritz Bunkus <moritz@bunkus.org>.
*/
#include "common/common_pch.h"

#include "common/chapters/bluray.h"
#include "common/chapters/chapters.h"
#include "common/construct.h"
#include "common/ebml.h"
#include "common/unique_numbers.h"

namespace mtx::chapters {

std::shared_ptr<libmatroska::KaxChapters>
convert_mpls_chapters_kax_chapters(mtx::bluray::mpls::chapters_t const &mpls_chapters,
                                   std::string const &main_language_,
                                   std::string const &name_template_) {
  auto main_language  = main_language_ == "und" ? "eng"s : main_language_;
  auto name_template  = name_template_.empty() ? mtx::chapters::g_chapter_generation_name_template.get_translated() : name_template_;
  auto chapter_number = 0;
  auto kax_chapters   = std::make_shared<libmatroska::KaxChapters>();
  auto &edition       = GetChild<libmatroska::KaxEditionEntry>(*kax_chapters);

  GetChild<libmatroska::KaxEditionUID>(edition).SetValue(create_unique_number(UNIQUE_EDITION_IDS));

  for (auto const &entry : mpls_chapters) {
    ++chapter_number;

    std::string name;
    for (auto const &[entry_language, entry_name] : entry.names)
      if (entry_language == main_language) {
        name = entry_name;
        break;
      }

    if (name.empty())
      name = mtx::chapters::format_name_template(name_template, chapter_number, entry.timestamp);

    auto atom = mtx::construct::cons<libmatroska::KaxChapterAtom>(new libmatroska::KaxChapterUID,       create_unique_number(UNIQUE_CHAPTER_IDS),
                                                                  new libmatroska::KaxChapterTimeStart, entry.timestamp.to_ns());

    if (!name.empty())
      atom->PushElement(*mtx::construct::cons<libmatroska::KaxChapterDisplay>(new libmatroska::KaxChapterString,   name,
                                                                              new libmatroska::KaxChapterLanguage, main_language));

    for (auto const &[entry_language, entry_name] : entry.names)
      if ((entry_language != main_language) && !entry_name.empty())
        atom->PushElement(*mtx::construct::cons<libmatroska::KaxChapterDisplay>(new libmatroska::KaxChapterString,   entry_name,
                                                                                new libmatroska::KaxChapterLanguage, entry_language));

    edition.PushElement(*atom);
  }

  mtx::chapters::align_uids(kax_chapters.get());

  return kax_chapters;
}

}
