/*
   mkvextract -- extract tracks from Matroska files into other files

   Distributed under the GPL
   see the file COPYING for details
   or visit http://www.gnu.org/copyleft/gpl.html

   extracts tracks from Matroska files into other files

   Written by Moritz Bunkus <moritz@bunkus.org>.
*/

#include "common/os.h"

#include <cassert>
#include <string>
#include <vector>

#include <ebml/EbmlHead.h>
#include <ebml/EbmlSubHead.h>
#include <ebml/EbmlStream.h>
#include <ebml/EbmlVersion.h>
#include <ebml/EbmlVoid.h>

#include <matroska/KaxBlock.h>
#include <matroska/KaxBlockData.h>
#include <matroska/KaxCluster.h>
#include <matroska/KaxClusterData.h>
#include <matroska/KaxInfo.h>
#include <matroska/KaxInfoData.h>
#include <matroska/KaxSegment.h>
#include <matroska/KaxTracks.h>
#include <matroska/KaxTrackEntryData.h>
#include <matroska/KaxTrackAudio.h>
#include <matroska/KaxTrackVideo.h>

#include "common/common.h"
#include "common/ebml.h"
#include "common/matroska.h"
#include "common/mm_io.h"
#include "extract/mkvextract.h"
#include "extract/xtr_base.h"

using namespace libmatroska;
using namespace std;

static vector<xtr_base_c *> extractors;

// ------------------------------------------------------------------------

static void
create_extractors(KaxTracks &kax_tracks,
                  vector<track_spec_t> &tracks) {
  int i;

  for (i = 0; i < kax_tracks.ListSize(); i++) {
    if (!is_id(kax_tracks[i], KaxTrackEntry))
      continue;

    KaxTrackEntry &track = *static_cast<KaxTrackEntry *>(kax_tracks[i]);
    int64_t tnum         = kt_get_number(track);

    // Is the track number present and valid?
    if (0 == tnum)
      continue;

    // Is there more than one track with the same track number?
    xtr_base_c *extractor = NULL;
    int k;
    for (k = 0; k < extractors.size(); k++)
      if (extractors[k]->m_tid == tnum) {
        mxwarn(boost::format(Y("More than one track with the track number %1% found.\n")) % tnum);
        extractor = extractors[k];
        break;
      }
    if (NULL != extractor)
      continue;

    // Does the user want this track to be extracted?
    track_spec_t *tspec = NULL;
    for (k = 0; k < tracks.size(); k++)
      if (tracks[k].tid == tnum) {
        tspec = &tracks[k];
        break;
      }
    if (NULL == tspec)
      continue;

    // Let's find the codec ID and create an extractor for it.
    string codec_id = kt_get_codec_id(track);
    if (codec_id.empty())
      mxerror(boost::format(Y("The track number %1% does not have a valid CodecID.\n")) % tnum);

    extractor = xtr_base_c::create_extractor(codec_id, tnum, *tspec);
    if (NULL == extractor)
      mxerror(boost::format(Y("Extraction of track number %1% with the CodecID '%2%' is not supported.\n")) % tnum % codec_id);

    // Has there another file been requested with the same name?
    xtr_base_c *master = NULL;
    for (k = 0; k < extractors.size(); k++)
      if (extractors[k]->m_file_name == tspec->out_name) {
        master = extractors[k];
        break;
      }

    // Let the extractor create the file.
    extractor->create_file(master, track);

    // We're done.
    extractors.push_back(extractor);

    mxinfo(boost::format(Y("Extracting track %1% with the CodecID '%2%' to the file '%3%'. Container format: %4%\n"))
           % tnum % codec_id % tspec->out_name % extractor->get_container_name());
  }

  // Signal that all headers have been taken care of.
  for (i = 0; i < extractors.size(); i++)
    extractors[i]->headers_done();
}

static void
handle_blockgroup(KaxBlockGroup &blockgroup,
                  KaxCluster &cluster,
                  int64_t tc_scale) {
  // Only continue if this block group actually contains a block.
  KaxBlock *block = FINDFIRST(&blockgroup, KaxBlock);
  if ((NULL == block) || (0 == block->NumberFrames()))
    return;

  block->SetParent(cluster);

  // Do we need this block group?
  xtr_base_c *extractor = NULL;
  int i;
  for (i = 0; i < extractors.size(); i++)
    if (block->TrackNum() == extractors[i]->m_tid) {
      extractor = extractors[i];
      break;
    }
  if (NULL == extractor)
    return;

  // Next find the block duration if there is one.
  KaxBlockDuration *kduration   = FINDFIRST(&blockgroup, KaxBlockDuration);
  int64_t duration              = NULL == kduration ? -1 : (int64_t)uint64(*kduration) * tc_scale;

  // Now find backward and forward references.
  int64_t bref                  = 0;
  int64_t fref                  = 0;
  KaxReferenceBlock *kreference = FINDFIRST(&blockgroup, KaxReferenceBlock);
  for (i = 0; (2 > i) && (NULL != kreference); i++) {
    if (0 > int64(*kreference))
      bref = int64(*kreference);
    else
      fref = int64(*kreference);
    kreference = FINDNEXT(&blockgroup, KaxReferenceBlock, kreference);
  }

  // Any block additions present?
  KaxBlockAdditions *kadditions = FINDFIRST(&blockgroup, KaxBlockAdditions);

  if (0 > duration)
    duration = extractor->m_default_duration * block->NumberFrames();

  KaxCodecState *kcstate = FINDFIRST(&blockgroup, KaxCodecState);
  if (NULL != kcstate) {
    memory_cptr codec_state(new memory_c(kcstate->GetBuffer(), kcstate->GetSize(), false));
    extractor->handle_codec_state(codec_state);
  }

  for (i = 0; i < block->NumberFrames(); i++) {
    int64_t this_timecode, this_duration;

    if (0 > duration) {
      this_timecode = block->GlobalTimecode();
      this_duration = duration;
    } else {
      this_timecode = block->GlobalTimecode() + i * duration /
        block->NumberFrames();
      this_duration = duration / block->NumberFrames();
    }

    DataBuffer &data = block->GetBuffer(i);
    memory_cptr frame(new memory_c(data.Buffer(), data.Size(), false));
    extractor->handle_frame(frame, kadditions, this_timecode, this_duration, bref, fref, false, false, true);
  }
}

static void
handle_simpleblock(KaxSimpleBlock &simpleblock,
                   KaxCluster &cluster) {
  if (0 == simpleblock.NumberFrames())
    return;

  simpleblock.SetParent(cluster);

  // Do we need this block group?
  xtr_base_c *extractor = NULL;
  size_t i;
  for (i = 0; i < extractors.size(); i++)
    if (simpleblock.TrackNum() == extractors[i]->m_tid) {
      extractor = extractors[i];
      break;
    }

  if (NULL == extractor)
    return;

  int64_t duration = extractor->m_default_duration * simpleblock.NumberFrames();

  for (i = 0; i < simpleblock.NumberFrames(); i++) {
    int64_t this_timecode, this_duration;

    if (0 > duration) {
      this_timecode = simpleblock.GlobalTimecode();
      this_duration = duration;
    } else {
      this_timecode = simpleblock.GlobalTimecode() + i * duration / simpleblock.NumberFrames();
      this_duration = duration / simpleblock.NumberFrames();
    }

    DataBuffer &data = simpleblock.GetBuffer(i);
    memory_cptr frame(new memory_c(data.Buffer(), data.Size(), false));
    extractor->handle_frame(frame, NULL, this_timecode, this_duration, -1, -1, simpleblock.IsKeyframe(), simpleblock.IsDiscardable(), false);
  }
}

static void
close_extractors() {
  int i;

  for (i = 0; i < extractors.size(); i++)
    extractors[i]->finish_track();

  for (i = 0; i < extractors.size(); i++)
    if (NULL != extractors[i]->m_master)
      extractors[i]->finish_file();

  for (i = 0; i < extractors.size(); i++) {
    if (NULL == extractors[i]->m_master)
      extractors[i]->finish_file();
    delete extractors[i];
  }

  extractors.clear();
}

static void
write_all_cuesheets(KaxChapters &chapters,
                    KaxTags &tags,
                    vector<track_spec_t> &tspecs) {
  int i;
  mm_io_c *out = NULL;

  for (i = 0; i < tspecs.size(); i++) {
    if (tspecs[i].extract_cuesheet) {
      string file_name = tspecs[i].out_name;
      int pos          = file_name.rfind('/');
      int pos2         = file_name.rfind('\\');

      if (pos2 > pos)
        pos = pos2;
      if (pos >= 0)
        file_name.erase(0, pos2);

      string cue_file_name = (string)tspecs[i].out_name;
      pos                  = cue_file_name.rfind('.');
      pos2                 = cue_file_name.rfind('/');
      int pos3             = cue_file_name.rfind('\\');

      if ((0 <= pos) && (pos > pos2) && (pos > pos3))
        cue_file_name.erase(pos);
      cue_file_name += ".cue";

      try {
        out = new mm_file_io_c(cue_file_name.c_str(), MODE_CREATE);
      } catch(...) {
        mxerror(boost::format(Y("The file '%1%' could not be opened for writing (%2%).\n")) % cue_file_name % strerror(errno));
      }

      mxinfo(boost::format(Y("The CUE sheet for track %1% will be written to '%2%'.\n")) % tspecs[i].tid % cue_file_name);
      write_cuesheet(file_name.c_str(), chapters, tags, tspecs[i].tuid, *out);
      delete out;
    }
  }
}

static void
find_track_uids(KaxTracks &tracks,
                vector<track_spec_t> &tspecs) {
  int t;

  for (t = 0; t < tracks.ListSize(); t++) {
    KaxTrackEntry *track_entry = dynamic_cast<KaxTrackEntry *>(tracks[t]);
    if (NULL == track_entry)
      continue;

    int64_t track_number = kt_get_number(*track_entry);

    int s;
    for (s = 0; tspecs.size() > s; ++s)
      if (tspecs[s].tid == track_number) {
        tspecs[s].tuid = kt_get_uid(*track_entry);
        break;
      }
  }
}

bool
extract_tracks(const char *file_name,
               vector<track_spec_t> &tspecs) {
  // open input file
  mm_io_c *in;
  try {
    in = new mm_file_io_c(file_name);
  } catch (...) {
    show_error(boost::format(Y("The file '%1%' could not be opened for reading (%2%).\n")) % file_name % strerror(errno));
    return false;
  }

  int64_t file_size = in->get_size();

  try {
    EbmlStream *es = new EbmlStream(*in);

    // Find the EbmlHead element. Must be the first one.
    EbmlElement *l0 = es->FindNextID(EbmlHead::ClassInfos, 0xFFFFFFFFL);
    if (NULL == l0) {
      show_error(Y("Error: No EBML head found."));
      delete es;

      return false;
    }

    // Don't verify its data for now.
    l0->SkipData(*es, l0->Generic().Context);
    delete l0;

    while (1) {
      // Next element must be a segment
      l0 = es->FindNextID(KaxSegment::ClassInfos, 0xFFFFFFFFFFFFFFFFLL);

      if (NULL == l0) {
        show_error(Y("No segment/level 0 element found."));
        return false;
      }

      if (EbmlId(*l0) == KaxSegment::ClassInfos.GlobalId) {
        show_element(l0, 0, Y("Segment"));
        break;
      }

      l0->SkipData(*es, l0->Generic().Context);
      delete l0;
    }

    bool tracks_found = false;
    int upper_lvl_el  = 0;
    // We've got our segment, so let's find the tracks
    EbmlElement *l1     = es->FindNextElement(l0->Generic().Context, upper_lvl_el, 0xFFFFFFFFFFFFFFFFLL, true, 1);
    EbmlElement *l2     = NULL;
    EbmlElement *l3     = NULL;

    uint64_t tc_scale   = TIMECODE_SCALE;
    uint64_t cluster_tc = 0;

    KaxCluster *cluster = NULL;
    KaxChapters all_chapters;
    KaxTags all_tags;

    while ((NULL != l1) && (0 >= upper_lvl_el)) {
      if (EbmlId(*l1) == KaxInfo::ClassInfos.GlobalId) {
        // General info about this Matroska file
        show_element(l1, 1, Y("Segment information"));

        upper_lvl_el = 0;
        l2           = es->FindNextElement(l1->Generic().Context, upper_lvl_el, 0xFFFFFFFFL, true, 1);

        while ((NULL != l2) && (0 >= upper_lvl_el)) {

          if (EbmlId(*l2) == KaxTimecodeScale::ClassInfos.GlobalId) {
            KaxTimecodeScale &ktc_scale = *static_cast<KaxTimecodeScale *>(l2);
            ktc_scale.ReadData(es->I_O());
            tc_scale = uint64(ktc_scale);
            show_element(l2, 2, boost::format(Y("Timecode scale: %1%")) % tc_scale);
          } else
            l2->SkipData(*es, l2->Generic().Context);

          if (!in_parent(l1)) {
            delete l2;
            break;
          }

          if (0 > upper_lvl_el) {
            upper_lvl_el++;
            if (0 > upper_lvl_el)
              break;

          }

          l2->SkipData(*es, l2->Generic().Context);
          delete l2;
          l2 = es->FindNextElement(l1->Generic().Context, upper_lvl_el, 0xFFFFFFFFL, true);
        }

      } else if ((EbmlId(*l1) == KaxTracks::ClassInfos.GlobalId) && !tracks_found) {

        // Yep, we've found our KaxTracks element. Now find all tracks
        // contained in this segment.
        show_element(l1, 1, Y("Segment tracks"));

        tracks_found = true;
        l1->Read(*es, KaxTracks::ClassInfos.Context, upper_lvl_el, l2, true);
        find_track_uids(*dynamic_cast<KaxTracks *>(l1), tspecs);
        create_extractors(*dynamic_cast<KaxTracks *>(l1), tspecs);

      } else if (EbmlId(*l1) == KaxCluster::ClassInfos.GlobalId) {
        show_element(l1, 1, Y("Cluster"));
        cluster = (KaxCluster *)l1;

        if (0 == verbose)
          mxinfo(boost::format(Y("Progress: %1%%%%2%")) % (int)(in->getFilePointer() * 100 / file_size) % "\r");

        upper_lvl_el = 0;
        l2           = es->FindNextElement(l1->Generic().Context, upper_lvl_el, 0xFFFFFFFFL, true, 1);
        while ((NULL != l2) && (0 >= upper_lvl_el)) {

          if (EbmlId(*l2) == KaxClusterTimecode::ClassInfos.GlobalId) {
            KaxClusterTimecode &ctc = *static_cast<KaxClusterTimecode *>(l2);
            ctc.ReadData(es->I_O());
            cluster_tc = uint64(ctc);
            show_element(l2, 2, boost::format(Y("Cluster timecode: %|1$.3f|s")) % ((float)cluster_tc * (float)tc_scale / 1000000000.0));
            cluster->InitTimecode(cluster_tc, tc_scale);

          } else if (EbmlId(*l2) == KaxBlockGroup::ClassInfos.GlobalId) {
            show_element(l2, 2, Y("Block group"));

            l2->Read(*es, KaxBlockGroup::ClassInfos.Context, upper_lvl_el, l3, true);
            handle_blockgroup(*static_cast<KaxBlockGroup *>(l2), *cluster, tc_scale);

          } else if (EbmlId(*l2) == KaxSimpleBlock::ClassInfos.GlobalId) {
            show_element(l2, 2, Y("SimpleBlock"));

            l2->Read(*es, KaxSimpleBlock::ClassInfos.Context, upper_lvl_el, l3, true);
            handle_simpleblock(*static_cast<KaxSimpleBlock *>(l2), *cluster);

          } else
            l2->SkipData(*es, l2->Generic().Context);

          if (!in_parent(l1)) {
            delete l2;
            break;
          }

          if (0 < upper_lvl_el) {
            upper_lvl_el--;
            if (0 < upper_lvl_el)
              break;
            delete l2;
            l2 = l3;
            continue;

          } else if (0 > upper_lvl_el) {
            upper_lvl_el++;
            if (0 > upper_lvl_el)
              break;

          }

          l2->SkipData(*es, l2->Generic().Context);
          delete l2;
          l2 = es->FindNextElement(l1->Generic().Context, upper_lvl_el, 0xFFFFFFFFL, true);

        } // while (l2 != NULL)

      } else if (EbmlId(*l1) == KaxChapters::ClassInfos.GlobalId) {
        KaxChapters &chapters = *static_cast<KaxChapters *>(l1);
        chapters.Read(*es, KaxChapters::ClassInfos.Context, upper_lvl_el, l2, true);
        while (chapters.ListSize() > 0) {
          if (EbmlId(*chapters[0]) == KaxEditionEntry::ClassInfos.GlobalId) {
            KaxEditionEntry &entry = *static_cast<KaxEditionEntry *>(chapters[0]);
            while (entry.ListSize() > 0) {
              if (EbmlId(*entry[0]) == KaxChapterAtom::ClassInfos.GlobalId)
                all_chapters.PushElement(*entry[0]);
              entry.Remove(0);
            }
          }
          chapters.Remove(0);
        }

      } else if (EbmlId(*l1) == KaxTags::ClassInfos.GlobalId) {
        KaxTags &tags = *static_cast<KaxTags *>(l1);
        tags.Read(*es, KaxTags::ClassInfos.Context, upper_lvl_el, l2, true);

        while (tags.ListSize() > 0) {
          all_tags.PushElement(*tags[0]);
          tags.Remove(0);
        }

      } else
        l1->SkipData(*es, l1->Generic().Context);

      if (!in_parent(l0)) {
        delete l1;
        break;
      }

      if (0 < upper_lvl_el) {
        upper_lvl_el--;
        if (0 < upper_lvl_el)
          break;
        delete l1;
        l1 = l2;
        continue;

      } else if (0 > upper_lvl_el) {
        upper_lvl_el++;
        if (0 > upper_lvl_el)
          break;

      }

      l1->SkipData(*es, l1->Generic().Context);
      delete l1;
      l1 = es->FindNextElement(l0->Generic().Context, upper_lvl_el, 0xFFFFFFFFL, true);

    } // while (l1 != NULL)

    delete l0;
    delete es;
    delete in;

    write_all_cuesheets(all_chapters, all_tags, tspecs);

    // Now just close the files and go to sleep. Mummy will sing you a
    // lullaby. Just close your eyes, listen to her sweet voice, singing,
    // singing, fading... fad... ing...
    close_extractors();

    return true;
  } catch (...) {
    show_error(Y("Caught exception"));
    delete in;

    return false;
  }
}
