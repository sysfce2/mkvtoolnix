/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   class definitions for the converter from teletext to SRT subtitles

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

#include "common/debugging.h"
#include "common/timestamp.h"
#include "input/packet_converter.h"

#define TTX_PAGE_ROW_SIZE 24
#define TTX_PAGE_COL_SIZE 40

class teletext_to_srt_packet_converter_c: public packet_converter_c {
protected:
  struct ttx_page_data_t {
    int page, subpage;
    unsigned int flags, national_set;
    bool erase_flag;
    std::vector<std::string> page_buffer;

    ttx_page_data_t() {
      reset();
    }

    void reset();
  };

  struct track_data_t {
    ttx_page_data_t m_page_data;

    timestamp_c m_queued_timestamp, m_page_timestamp;
    packet_cptr m_queued_packet;
    std::optional<int> m_forced_char_map_idx;
    bool m_page_changed{};
    generic_packetizer_c *m_ptzr{};
    int m_magazine;

    track_data_t(generic_packetizer_c *ptzr)
      : m_ptzr{ptzr}
      , m_magazine{-1}
    {
    }
  };

  using track_data_cptr = std::shared_ptr<track_data_t>;
  using char_map_t      = std::unordered_map<int, char const *>;

  static std::vector<char_map_t> ms_char_maps;

  size_t m_in_size, m_pos, m_data_length;
  unsigned char *m_buf;
  timestamp_c m_current_packet_timestamp;
  std::unordered_map<int, track_data_cptr> m_track_data;
  track_data_t *m_current_track{};

  mtx::regex::jp::Regex m_page_re1, m_page_re2, m_page_re3;

  debugging_option_c m_debug, m_debug_packet, m_debug_conversion;

public:
  teletext_to_srt_packet_converter_c();
  virtual ~teletext_to_srt_packet_converter_c() {};

  virtual bool convert(packet_cptr const &packet) override;
  virtual void flush() override;

  virtual void override_encoding(int page, std::string const &iso639_2_code);
  virtual void demux_page(int page, generic_packetizer_c *ptzr);

protected:
  void process_ttx_packet();
  std::string page_to_string() const;
  bool decode_line(unsigned char const *buffer, unsigned int row_number);
  void process_single_row(unsigned int row_number);
  void decode_page_data(unsigned char ttx_header_magazine);
  void queue_page_content(std::string const &content);
  void queue_packet(packet_cptr const &new_packet);
  void deliver_queued_packet();
  bool maybe_merge_queued_and_new_packet(packet_t const &new_packet);

protected:
  static int ttx_to_page(int ttx);
  static void bit_reverse(unsigned char *buffer, size_t length);
  static void unham(unsigned char const *in, unsigned char *out, size_t hambytes);
  static void remove_parity(unsigned char *buffer, size_t length);
  static void setup_character_maps();
};
