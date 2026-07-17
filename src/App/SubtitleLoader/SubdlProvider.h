#pragma once

#include <memory>

#include "ISubtitleProvider.h"

namespace TorrentPlayer::Subtitles {

class SubdlProvider final
	: public ISubtitleProvider
{
public:
	SubdlProvider();
	~SubdlProvider() override;

	SubtitleProviderId GetId() const override;
	void SetUserData(const QVariantMap & userData) override;
	bool IsConfigured() const override;
	void Request(const SubtitleRequestContext & context, Completion completion) override;
	void Cancel() override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace TorrentPlayer::Subtitles
