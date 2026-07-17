#pragma once

#include <functional>
#include <memory>

#include <QString>
#include <QVariantMap>

#include "SubtitleTypes.h"

namespace TorrentPlayer::Subtitles {

class SubtitleRequestCoordinator
{
public:
	using Completion = std::function<void(SubtitleRequestOutcome)>;

	SubtitleRequestCoordinator();
	~SubtitleRequestCoordinator();

	void SetUserData(const QVariantMap & userData);
	void Request(
		const QString & videoPath,
		const QString & language,
		const QString & explicitImdbId,
		Completion completion);
	void Cancel();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace TorrentPlayer::Subtitles
