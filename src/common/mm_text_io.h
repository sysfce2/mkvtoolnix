/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL v2
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   IO callback class definitions

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#pragma once

#include "common/common_pch.h"

class mm_text_io_c: public mm_proxy_io_c {
protected:
  byte_order_e m_byte_order;
  unsigned int m_bom_len;
  bool m_uses_carriage_returns, m_uses_newlines, m_eol_style_detected;

public:
  mm_text_io_c(mm_io_cptr const &in);

  virtual void setFilePointer(int64 offset, seek_mode mode=seek_beginning);
  virtual std::string getline(boost::optional<std::size_t> max_chars = boost::none);
  virtual std::string read_next_codepoint();
  virtual byte_order_e get_byte_order() const {
    return m_byte_order;
  }
  virtual unsigned int get_byte_order_length() const {
    return m_bom_len;
  }
  virtual void set_byte_order(byte_order_e byte_order) {
    m_byte_order = byte_order;
  }
  virtual boost::optional<std::string> get_encoding() const {
    return get_encoding(m_byte_order);
  }

protected:
  virtual void detect_eol_style();

public:
  static bool has_byte_order_marker(const std::string &string);
  static bool detect_byte_order_marker(const unsigned char *buffer, unsigned int size, byte_order_e &byte_order, unsigned int &bom_length);
  static boost::optional<std::string> get_encoding(byte_order_e byte_order);
};
