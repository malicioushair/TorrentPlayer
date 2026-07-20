#include "SubtitleRequestCoordinator.h"

#include <memory>
#include <utility>
#include <vector>

#include <QStringList>

#include "OpenSubtitlesProvider.h"
#include "SubdlProvider.h"
#include "ISubtitleProvider.h"
#include "VideoMetadataResolver.h"

namespace TorrentPlayer::Subtitles {

struct SubtitleRequestCoordinator::Impl
{
	std::vector<std::unique_ptr<ISubtitleProvider>> providers;
	VideoMetadataResolver metadataResolver;
	quint64 generation {};
	quint64 nextRequestId {};

	Impl()
	{
		providers.push_back(std::make_unique<SubdlProvider>());
		providers.push_back(std::make_unique<OpenSubtitlesProvider>());
	}

	void SetUserData(const QVariantMap & userData)
	{
		Cancel();
		for (auto & provider : providers)
			provider->SetUserData(userData);
	}

	void Request(
		const QString & videoPath,
		const QString & language,
		const QString & explicitImdbId,
		Completion completion)
	{
		Cancel();
		const auto requestGeneration = ++generation;
		SubtitleRequestContext context {
			++nextRequestId,
			videoPath,
			language,
			metadataResolver.Resolve(videoPath, explicitImdbId),
		};
		TryProvider(std::move(context), std::move(completion), 0, {}, requestGeneration, 0);
	}

	void Cancel()
	{
		++generation;
		for (auto & provider : providers)
			provider->Cancel();
	}

	void TryProvider(
		SubtitleRequestContext context,
		Completion completion,
		size_t providerIndex,
		QStringList errors,
		quint64 requestGeneration,
		int configuredProviders)
	{
		if (requestGeneration != generation)
			return;

		while (providerIndex < providers.size() && !providers.at(providerIndex)->IsConfigured())
			++providerIndex;

		if (providerIndex >= providers.size())
		{
			const auto noProvidersConfigured = configuredProviders == 0;
			completion({
				false,
				std::move(context),
				SubtitleProviderId::Subdl,
				{},
				noProvidersConfigured
					? QStringLiteral("No subtitle provider configured.")
					: QStringLiteral("Subtitle search failed."),
				noProvidersConfigured
					? QStringLiteral("Set a SubDL or OpenSubtitles API key in Subtitle settings.")
					: errors.join(QLatin1Char('\n')),
			});
			return;
		}

		auto * provider = providers.at(providerIndex).get();
		provider->Request(context, [this, context, completion, providerIndex, errors, requestGeneration, configuredProviders](const SubtitleProviderResult & result) mutable {
			if (requestGeneration != generation)
				return;

			if (result.status == ProviderRequestStatus::Success)
			{
				completion({
					true,
					context,
					result.provider,
					std::move(result.payload),
					{},
					{},
				});
				return;
			}

			if (!result.errorDescription.isEmpty())
				errors.push_back(QStringLiteral("%1: %2").arg(SubtitleProviderName(result.provider), result.errorDescription));
			TryProvider(context, completion, providerIndex + 1, errors, requestGeneration, configuredProviders + 1);
		});
	}
};

SubtitleRequestCoordinator::SubtitleRequestCoordinator()
	: m_impl(std::make_unique<Impl>())
{
}

SubtitleRequestCoordinator::~SubtitleRequestCoordinator()
{
	m_impl->Cancel();
}

void SubtitleRequestCoordinator::SetUserData(const QVariantMap & userData)
{
	m_impl->SetUserData(userData);
}

void SubtitleRequestCoordinator::Request(
	const QString & videoPath,
	const QString & language,
	const QString & explicitImdbId,
	Completion completion)
{
	m_impl->Request(videoPath, language, explicitImdbId, std::move(completion));
}

void SubtitleRequestCoordinator::Cancel()
{
	m_impl->Cancel();
}

} // namespace TorrentPlayer::Subtitles
