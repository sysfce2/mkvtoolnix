/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   class definition for the Flac packetizer

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#if defined(HAVE_FLAC_FORMAT_H)

#include <FLAC/format.h>

#include "merge/generic_packetizer.h"

class flac_packetizer_c: public generic_packetizer_c {
private:
  memory_cptr m_header;
  FLAC__StreamMetadata_StreamInfo m_stream_info;
  int64_t m_num_packets{};

public:
  flac_packetizer_c(generic_reader_c *p_reader, track_info_c &p_ti, unsigned char *header, int l_header);
  virtual ~flac_packetizer_c();

  virtual int process(packet_cptr packet);
  virtual void set_headers();

  virtual translatable_string_c get_format_name() const {
    return YT("FLAC");
  }

  virtual connection_result_e can_connect_to(generic_packetizer_c *src, std::string &error_message) override;
  virtual split_result_e can_be_split(std::string &error_message) override;
};

#endif  // HAVE_FLAC_STREAM_DECODER_H
