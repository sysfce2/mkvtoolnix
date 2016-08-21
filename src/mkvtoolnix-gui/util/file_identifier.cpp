#include "common/common_pch.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStringList>

#include "common/json.h"
#include "common/qt.h"
#include "common/strings/editing.h"
#include "mkvtoolnix-gui/util/file_identifier.h"
#include "mkvtoolnix-gui/util/json.h"
#include "mkvtoolnix-gui/util/process.h"
#include "mkvtoolnix-gui/util/settings.h"

namespace mtx { namespace gui { namespace Util {

class FileIdentifierPrivate {
  friend class FileIdentifier;

  int m_exitCode{};
  QStringList m_output;
  QString m_fileName, m_errorTitle, m_errorText;
  mtx::gui::Merge::SourceFilePtr m_file;

  explicit FileIdentifierPrivate(QString const &fileName)
    : m_fileName{fileName}
  {
  }
};

using namespace mtx::gui;

FileIdentifier::FileIdentifier(QString const &fileName)
  : d_ptr{new FileIdentifierPrivate{QDir::toNativeSeparators(fileName)}}
{
}

FileIdentifier::~FileIdentifier() {
}

bool
FileIdentifier::identify() {
  Q_D(FileIdentifier);

  if (d->m_fileName.isEmpty())
    return false;

  auto &cfg = Settings::get();

  auto args = QStringList{} << "--output-charset" << "utf-8" << "--identification-format" << "json" << "--identify" << d->m_fileName;

  addProbeRangePercentageArg(args, cfg.m_probeRangePercentage);

  if (cfg.m_defaultAdditionalMergeOptions.contains(Q("keep_last_chapter_in_mpls")))
    args << "--engage" << "keep_last_chapter_in_mpls";

  auto process  = Process::execute(cfg.actualMkvmergeExe(), args);
  d->m_exitCode = process->process().exitCode();

  if (process->hasError()) {
    setError(QY("Error executing mkvmerge"), QY("The mkvmerge executable was not found."));
    return false;
  }

  d->m_output = process->output();

  return parseOutput();
}

QString const &
FileIdentifier::fileName()
  const {
  Q_D(const FileIdentifier);

  return d->m_fileName;
}

void
FileIdentifier::setFileName(QString const &fileName) {
  Q_D(FileIdentifier);

  d->m_fileName = QDir::toNativeSeparators(fileName);
}

QString const &
FileIdentifier::errorTitle()
  const {
  Q_D(const FileIdentifier);

  return d->m_errorTitle;
}

QString const &
FileIdentifier::errorText()
  const {
  Q_D(const FileIdentifier);

  return d->m_errorText;
}

void
FileIdentifier::setError(QString const &errorTitle,
                         QString const &errorText) {
  Q_D(FileIdentifier);

  d->m_errorTitle = errorTitle;
  d->m_errorText  = errorText;
}

int
FileIdentifier::exitCode()
  const {
  Q_D(const FileIdentifier);

  return d->m_exitCode;
}

QStringList const &
FileIdentifier::output()
  const {
  Q_D(const FileIdentifier);

  return d->m_output;
}

Merge::SourceFilePtr const &
FileIdentifier::file()
  const {
  Q_D(const FileIdentifier);

  return d->m_file;
}

bool
FileIdentifier::parseOutput() {
  Q_D(FileIdentifier);

  auto root = QVariantMap{};

  try {
    auto doc = mtx::json::parse(to_utf8(d->m_output.join("\n")));
    root     = nlohmannJsonToVariant(doc).toMap();

  } catch (std::exception const &ex) {
    setError(QY("Error executing mkvmerge"), QY("The JSON output generated by mkvmerge could not be parsed (parser's error message: %1).").arg(Q(ex.what())));
    return false;
  }

  auto container = root.value("container").toMap();

  if (!container.value("recognized").toBool()) {
    setError(QY("Unrecognized file format"), QY("The file was not recognized as a supported format (exit code: %1).").arg(d->m_exitCode));
    return false;
  }

  if (!container.value("supported").toBool()) {
    setError(QY("Unsupported file format"), QY("The file is an unsupported container format (%1).").arg(container.value("type").toString()));
    return false;
  }

  d->m_file                         = std::make_shared<Merge::SourceFile>(d->m_fileName);
  d->m_file->m_probeRangePercentage = Settings::get().m_probeRangePercentage;

  parseContainer(container);

  for (auto const &val : root.value("tracks").toList())
    parseTrack(val.toMap());

  for (auto const &val : root.value("attachments").toList())
    parseAttachment(val.toMap());

  for (auto const &val : root.value("chapters").toList())
    parseChapters(val.toMap());

  for (auto const &val : root.value("global_tags").toList())
    parseGlobalTags(val.toMap());

  for (auto const &val : root.value("track_tags").toList())
    parseTrackTags(val.toMap());

  return d->m_file->isValid();
}

// "content_type": "text/plain",
// "description": "",
// "file_name": "vde.srt",
// "id": 1,
// "properties": {
//   "uid": 14629734976961512390
// },
// "size": 1274
void
FileIdentifier::parseAttachment(QVariantMap const &obj) {
  Q_D(FileIdentifier);

  auto track                     = std::make_shared<Merge::Track>(d->m_file.get(), Merge::Track::Attachment);
  track->m_properties            = obj.value("properties").toMap();
  track->m_id                    = obj.value("id").toULongLong();
  track->m_codec                 = obj.value("content_type").toString();
  track->m_size                  = obj.value("size").toULongLong();
  track->m_attachmentDescription = obj.value("description").toString();
  track->m_name                  = QDir::toNativeSeparators(obj.value("file_name").toString());

  d->m_file->m_attachedFiles << track;
}

// "num_entries": 5
void
FileIdentifier::parseChapters(QVariantMap const &obj) {
  Q_D(FileIdentifier);

  auto track    = std::make_shared<Merge::Track>(d->m_file.get(), Merge::Track::Chapters);
  track->m_size = obj.value("num_entries").toULongLong();

  d->m_file->m_tracks << track;
}

// "properties": {
//   "container_type": 17,
//   "duration": 71255000000,
//   "is_providing_timecodes": true,
//   "segment_uid": "a93dd71320097620bc002ac740ac4b50"
// },
// "recognized": true,
// "supported": true,
// "type": "Matroska"
void
FileIdentifier::parseContainer(QVariantMap const &obj) {
  Q_D(FileIdentifier);

  d->m_file->m_properties       = obj.value("properties").toMap();
  d->m_file->m_type             = static_cast<file_type_e>(d->m_file->m_properties.value("container_type").toUInt());
  d->m_file->m_isPlaylist       = d->m_file->m_properties.value("playlist").toBool();
  d->m_file->m_playlistDuration = d->m_file->m_properties.value("playlist_duration").toULongLong();
  d->m_file->m_playlistSize     = d->m_file->m_properties.value("playlist_size").toULongLong();
  d->m_file->m_playlistChapters = d->m_file->m_properties.value("playlist_chapters").toULongLong();

  if (d->m_file->m_isPlaylist)
    for (auto const &fileName : d->m_file->m_properties.value("playlist_file").toStringList())
      d->m_file->m_playlistFiles << QFileInfo{fileName};

  auto otherFiles = d->m_file->m_properties.value("other_file").toStringList();
  for (auto &fileName : otherFiles) {
    auto additionalPart              = std::make_shared<Merge::SourceFile>(fileName);
    additionalPart->m_additionalPart = true;
    additionalPart->m_appendedTo     = d->m_file.get();
    d->m_file->m_additionalParts       << additionalPart;
  }
}

// "num_entries": 5
void
FileIdentifier::parseGlobalTags(QVariantMap const &obj) {
  Q_D(FileIdentifier);

  auto track    = std::make_shared<Merge::Track>(d->m_file.get(), Merge::Track::GlobalTags);
  track->m_size = obj.value("num_entries").toULongLong();

  d->m_file->m_tracks << track;
}

// "num_entries": 7,
// "track_id": 0
void
FileIdentifier::parseTrackTags(QVariantMap const &obj) {
  Q_D(FileIdentifier);

  auto track    = std::make_shared<Merge::Track>(d->m_file.get(), Merge::Track::Tags);
  track->m_id   = obj.value("track_id").toULongLong();
  track->m_size = obj.value("num_entries").toULongLong();

  d->m_file->m_tracks << track;
}

// "codec": "AAC",
// "id": 3,
// "properties": {
//   "audio_channels": 2,
//   "audio_sampling_frequency": 48000,
//   "codec_id": "A_AAC",
//   "uid": 15551853204593941928
// },
// "type": "audio"
void
FileIdentifier::parseTrack(QVariantMap const &obj) {
  Q_D(FileIdentifier);

  auto typeStr        = obj.value("type").toString();
  auto type           = typeStr == "audio"     ? Merge::Track::Audio
                      : typeStr == "video"     ? Merge::Track::Video
                      : typeStr == "subtitles" ? Merge::Track::Subtitles
                      :                          Merge::Track::Buttons;
  auto track          = std::make_shared<Merge::Track>(d->m_file.get(), type);
  track->m_id         = obj.value("id").toULongLong();
  track->m_codec      = obj.value("codec").toString();
  track->m_properties = obj.value("properties").toMap();

  d->m_file->m_tracks << track;

  track->setDefaults();
}

void
FileIdentifier::addProbeRangePercentageArg(QStringList &args,
                                           double probeRangePercentage) {
  if (probeRangePercentage <= 0)
    return;

  auto integerPart = static_cast<unsigned int>(std::round(probeRangePercentage * 100)) / 100;
  auto decimalPart = static_cast<unsigned int>(std::round(probeRangePercentage * 100)) % 100;

  if (integerPart >= 100)
    return;

  if (   (integerPart != 0)
      || (   (decimalPart !=  0)
          && (decimalPart != 30)))
    args << "--probe-range-percentage" << Q(boost::format("%1%.%|2$02d|") % integerPart % decimalPart);
}

}}}
