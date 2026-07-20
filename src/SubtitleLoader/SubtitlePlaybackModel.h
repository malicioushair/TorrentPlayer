#pragma once

#include <memory>

#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "SubtitleTypes.h"

namespace TorrentPlayer::Subtitles {

class SubtitlePlaybackModel
{
public:
	SubtitlePlaybackModel();
	~SubtitlePlaybackModel();

	void Clear();
	void AddOrReplaceTrack(SubtitleDocument document);
	bool SetActiveTrack(int trackIndex);
	bool SetPosition(qint64 positionMs);

	int GetActiveTrack() const;
	QStringList GetTracks() const;
	QString GetCurrentText() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace TorrentPlayer::Subtitles
