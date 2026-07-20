#pragma once

#include <functional>

#include <QVariantMap>

#include "SubtitleTypes.h"

namespace TorrentPlayer::Subtitles {

class ISubtitleProvider
{
public:
	using Completion = std::function<void(SubtitleProviderResult)>;

	virtual ~ISubtitleProvider() = default;

	virtual SubtitleProviderId GetId() const = 0;
	virtual void SetUserData(const QVariantMap & userData) = 0;
	virtual bool IsConfigured() const = 0;
	virtual void Request(const SubtitleRequestContext & context, Completion completion) = 0;
	virtual void Cancel() = 0;
};

} // namespace TorrentPlayer::Subtitles
