/*
   mkvmerge -- utility for splicing together matroska files
   from component media subtypes

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   MPEG 4 part 10 ES video output module

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/os.h"

#include <cassert>

#include "common/common.h"
#include "common/hacks.h"
#include "common/math.h"
#include "common/matroska.h"
#include "common/mpeg4_common.h"
#include "merge/output_control.h"
#include "output/p_avc.h"

using namespace libmatroska;
using namespace mpeg4::p10;

mpeg4_p10_es_video_packetizer_c::
mpeg4_p10_es_video_packetizer_c(generic_reader_c *p_reader,
                                track_info_c &p_ti,
                                memory_cptr avcc,
                                int width,
                                int height)
  : generic_packetizer_c(p_reader, p_ti)
  , m_avcc(avcc)
  , m_width(width)
  , m_height(height)
  , m_allow_timecode_generation(true)
  , m_first_frame(true)
{

  relaxed_timecode_checking = true;

  if (0 != ti.nalu_size_length)
    m_parser.set_nalu_size_length(ti.nalu_size_length);

  if (get_cue_creation() == CUE_STRATEGY_UNSPECIFIED)
    set_cue_creation(CUE_STRATEGY_IFRAMES);

  set_track_type(track_video);

  set_codec_id(MKV_V_MPEG4_AVC);

  set_codec_private(m_avcc->get(), m_avcc->get_size());
  extract_aspect_ratio();
  m_parser.set_keep_ar_info(false);
}

void
mpeg4_p10_es_video_packetizer_c::set_headers() {
  if (m_allow_timecode_generation) {
    if (-1 == htrack_default_duration)
      htrack_default_duration = 40000000;
    m_parser.enable_timecode_generation(htrack_default_duration);
  }

  set_video_pixel_width(m_width);
  set_video_pixel_height(m_height);

  generic_packetizer_c::set_headers();

  track_entry->EnableLacing(false);
}

void
mpeg4_p10_es_video_packetizer_c::add_extra_data(memory_cptr data) {
  m_parser.add_bytes(data->get(), data->get_size());
}

int
mpeg4_p10_es_video_packetizer_c::process(packet_cptr packet) {
  try {
    if (!m_allow_timecode_generation)
      m_parser.add_timecode(packet->timecode);
    m_parser.add_bytes(packet->data->get(), packet->data->get_size());
    flush_frames();

  } catch (nalu_size_length_error_c &error) {
    mxerror_tid(ti.fname, ti.id,
                boost::format(Y("This AVC/h.264 contains frames that are too big for the current maximum NALU size. "
                                "You have to re-run mkvmerge and set the maximum NALU size to %1% for this track "
                                "(command line parameter '--nalu-size-length %2%:%1%').\n"))
                % error.get_required_length() % ti.id);

  } catch (error_c &error) {
    mxerror_tid(ti.fname, ti.id,
                boost::format(Y("mkvmerge encountered broken or unparsable data in this AVC/h.264 video track. "
                                "Either your file is damaged (which mkvmerge cannot cope with yet) or this is a bug in mkvmerge itself. "
                                "The error message was:\n%1%\n")) % error.get_error());
  }

  return FILE_STATUS_MOREDATA;
}

void
mpeg4_p10_es_video_packetizer_c::extract_aspect_ratio() {
  uint32_t num, den;
  unsigned char *priv = hcodec_private;

  if (mpeg4::p10::extract_par(hcodec_private, hcodec_private_length, num, den) && (0 != num) && (0 != den)) {
    if (!ti.aspect_ratio_given && !ti.display_dimensions_given) {
      double par = (double)num / (double)den;

      if (1 <= par) {
        ti.display_width  = irnd(m_width * par);
        ti.display_height = m_height;

      } else {
        ti.display_width  = m_width;
        ti.display_height = irnd(m_height / par);

      }

      ti.display_dimensions_given = true;
      mxinfo_tid(ti.fname, ti.id,
                 boost::format(Y("Extracted the aspect ratio information from the MPEG-4 layer 10 (AVC) video data "
                                 "and set the display dimensions to %1%/%2%.\n")) % ti.display_width % ti.display_height);
    }
  }

  if (priv != hcodec_private)
    safefree(priv);
}

void
mpeg4_p10_es_video_packetizer_c::flush() {
  m_parser.flush();
  flush_frames();
  generic_packetizer_c::flush();
}

void
mpeg4_p10_es_video_packetizer_c::flush_frames() {
  while (m_parser.frame_available()) {
    avc_frame_t frame(m_parser.get_frame());
    if (m_first_frame && (0 < m_parser.get_num_skipped_frames()))
      mxwarn_tid(ti.fname, ti.id,
                 boost::format(Y("This AVC/h.264 track does not start with a key frame. The first %1% frames have been skipped.\n")) % m_parser.get_num_skipped_frames());
    add_packet(new packet_t(frame.m_data, frame.m_start,
                            frame.m_end > frame.m_start ? frame.m_end - frame.m_start : htrack_default_duration,
                            frame.m_keyframe            ? -1                          : frame.m_start + frame.m_ref1));
    m_first_frame = false;
  }
}

void
mpeg4_p10_es_video_packetizer_c::enable_timecode_generation(bool enable,
                                                            int64_t default_duration) {
  m_allow_timecode_generation = enable;
  if (enable) {
    m_parser.enable_timecode_generation(default_duration);
    set_track_default_duration(default_duration);
  }
}

void
mpeg4_p10_es_video_packetizer_c::connect(generic_packetizer_c *src,
                                         int64_t p_append_timecode_offset) {
  generic_packetizer_c::connect(src, p_append_timecode_offset);

  if (2 != connected_to)
    return;

  mpeg4_p10_es_video_packetizer_c *real_src = dynamic_cast<mpeg4_p10_es_video_packetizer_c *>(src);
  assert(NULL != real_src);

  m_allow_timecode_generation = real_src->m_allow_timecode_generation;
  htrack_default_duration     = real_src->htrack_default_duration;

  if (m_allow_timecode_generation)
    m_parser.enable_timecode_generation(htrack_default_duration);
}

connection_result_e
mpeg4_p10_es_video_packetizer_c::can_connect_to(generic_packetizer_c *src,
                                                string &error_message) {
  mpeg4_p10_es_video_packetizer_c *vsrc = dynamic_cast<mpeg4_p10_es_video_packetizer_c *>(src);
  if (NULL == vsrc)
    return CAN_CONNECT_NO_FORMAT;

  connect_check_v_width(m_width, vsrc->m_width);
  connect_check_v_height(m_height, vsrc->m_height);
  connect_check_codec_id(hcodec_id, vsrc->hcodec_id);

  if (((NULL == ti.private_data) && (NULL != vsrc->ti.private_data)) ||
      ((NULL != ti.private_data) && (NULL == vsrc->ti.private_data)) ||
      (ti.private_size != vsrc->ti.private_size)) {
    error_message = (boost::format(Y("The codec's private data does not match (lengths: %1% and %2%).")) % ti.private_size % vsrc->ti.private_size).str();
    return CAN_CONNECT_MAYBE_CODECPRIVATE;
  }

  return CAN_CONNECT_YES;
}

