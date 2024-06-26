/** \brief command line parsing

   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   \author Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/common_pch.h"

#include "common/split_arg_parsing.h"
#include "common/strings/editing.h"
#include "common/strings/parsing.h"

namespace mtx::args {

std::vector<split_point_c>
parse_split_parts(const std::string &arg,
                  bool frames_fields) {
  std::string s = arg;

  if (balg::istarts_with(s, "parts:"))
    s.erase(0, 6);

  else if (balg::istarts_with(s, "parts-frames:"))
    s.erase(0, 13);

  if (s.empty())
    throw format_x{fmt::format(FY("Missing start/end specifications for '--split' in '--split {0}'.\n"), arg)};

  std::vector<std::tuple<int64_t, int64_t, bool> > requested_split_points;
  for (auto const &part_spec : mtx::string::split(s, ",")) {
    auto pair = mtx::string::split(part_spec, "-");
    if (pair.size() != 2)
      throw format_x{fmt::format(FY("Invalid start/end specification for '--split' in '--split {0}' (current part: {1}).\n"), arg, part_spec)};

    bool create_new_file = true;
    if (pair[0].substr(0, 1) == "+") {
      if (!requested_split_points.empty())
        create_new_file = false;
      pair[0].erase(0, 1);
    }

    int64_t start;
    if (pair[0].empty())
      start = requested_split_points.empty() ? 0 : std::get<1>(requested_split_points.back());

    else if (!frames_fields && !mtx::string::parse_timestamp(pair[0], start))
      throw format_x{fmt::format(FY("Invalid start time for '--split' in '--split {0}' (current part: {1}). Additional error message: {2}.\n"), arg, part_spec, mtx::string::timestamp_parser_error)};

    else if (frames_fields && (!mtx::string::parse_number(pair[0], start) || (0 > start)))
      throw format_x{fmt::format(FY("Invalid start frame/field number for '--split' in '--split {0}' (current part: {1}).\n"), arg, part_spec)};

    int64_t end;
    if (pair[1].empty())
      end = std::numeric_limits<int64_t>::max();

    else if (!frames_fields && !mtx::string::parse_timestamp(pair[1], end))
      throw format_x{fmt::format(FY("Invalid end time for '--split' in '--split {0}' (current part: {1}). Additional error message: {2}.\n"), arg, part_spec, mtx::string::timestamp_parser_error)};

    else if (frames_fields && (!mtx::string::parse_number(pair[1], end) || (0 > end)))
      throw format_x{fmt::format(FY("Invalid end frame/field number for '--split' in '--split {0}' (current part: {1}).\n"), arg, part_spec)};

    if (end <= start) {
      if (frames_fields)
        throw format_x{fmt::format(FY("Invalid end frame/field number for '--split' in '--split {0}' (current part: {1}). The end number must be bigger than the start number.\n"), arg, part_spec)};
      else
        throw format_x{fmt::format(FY("Invalid end time for '--split' in '--split {0}' (current part: {1}). The end time must be bigger than the start time.\n"), arg, part_spec)};
    }

    if (!requested_split_points.empty() && (start < std::get<1>(requested_split_points.back()))) {
      if (frames_fields)
        throw format_x{fmt::format(FY("Invalid start frame/field number for '--split' in '--split {0}' (current part: {1}). The start number must be bigger than or equal to the previous part's end number.\n"), arg, part_spec)};
      else
        throw format_x{fmt::format(FY("Invalid start time for '--split' in '--split {0}' (current part: {1}). The start time must be bigger than or equal to the previous part's end time.\n"), arg, part_spec)};
    }

    requested_split_points.emplace_back(start, end, create_new_file);
  }

  std::vector<split_point_c> split_points;
  auto sp_type         = frames_fields ? split_point_c::parts_frame_field : split_point_c::parts;
  int64_t previous_end = 0;

  for (auto &split_point : requested_split_points) {
    if (previous_end < std::get<0>(split_point))
      split_points.emplace_back(previous_end, sp_type, true, true, std::get<2>(split_point));
    split_points.emplace_back(std::get<0>(split_point), sp_type, true, false, std::get<2>(split_point));
    previous_end = std::get<1>(split_point);
  }

  if (std::get<1>(requested_split_points.back()) < std::numeric_limits<int64_t>::max())
    split_points.emplace_back(std::get<1>(requested_split_points.back()), sp_type, true, true);

  return split_points;
}

}
