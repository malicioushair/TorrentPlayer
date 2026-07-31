#include "SubtitleLoader.h"

#include <array>
#include <utility>

#include "SubtitleCache.h"
#include "SubtitleContentProcessor.h"
#include "SubtitlePlaybackModel.h"
#include "SubtitleRequestCoordinator.h"
#include "VideoMetadataResolver.h"

using namespace TorrentPlayer::Subtitles;

struct SubtitleLoader::Impl
{
	explicit Impl(SubtitleLoader & loader)
		: loader(loader)
	{
	}

	SubtitleLoader & loader;
	QString videoPath;
	QString imdbId;
	SubtitleCache cache;
	SubtitleContentProcessor contentProcessor;
	SubtitlePlaybackModel playbackModel;
	SubtitleRequestCoordinator requestCoordinator;
	VideoMetadataResolver metadataResolver;

	void ClearPlayback()
	{
		const auto hadTracks = !playbackModel.GetTracks().empty();
		const auto hadActiveTrack = playbackModel.GetActiveTrack() >= 0;
		const auto hadText = !playbackModel.GetCurrentText().isEmpty();
		playbackModel.Clear();
		if (hadTracks)
			emit loader.subtitleTracksChanged();
		if (hadActiveTrack)
			emit loader.activeSubtitleTrackChanged();
		if (hadText)
			emit loader.currentSubtitleTextChanged();
	}

	void Apply(ProcessedSubtitle processed, SubtitleProviderId provider, const QString & language)
	{
		const auto previousTracks = playbackModel.GetTracks();
		const auto previousActiveTrack = playbackModel.GetActiveTrack();
		const auto previousText = playbackModel.GetCurrentText();

		processed.document.label = QStringLiteral("%1 %2").arg(SubtitleProviderName(provider), language.toUpper());
		playbackModel.AddOrReplaceTrack(std::move(processed.document));

		if (previousTracks != playbackModel.GetTracks())
			emit loader.subtitleTracksChanged();
		if (previousActiveTrack != playbackModel.GetActiveTrack())
			emit loader.activeSubtitleTrackChanged();
		if (previousText != playbackModel.GetCurrentText())
			emit loader.currentSubtitleTextChanged();
	}

	bool TryLoadCached(const QString & language)
	{
		static constexpr std::array providers {
			SubtitleProviderId::Subdl,
			SubtitleProviderId::OpenSubtitles,
		};

		const auto metadata = metadataResolver.Resolve(videoPath, imdbId);
		for (const auto provider : providers)
		{
			auto payload = cache.Find(videoPath, language, provider);
			if (payload)
			{
				QString errorDescription;
				auto processed = contentProcessor.Process(*payload, metadata, errorDescription);
				if (processed)
				{
					Apply(std::move(*processed), provider, language);
					return true;
				}
				cache.Invalidate(videoPath, language, provider);
			}

			auto archiveEntries = cache.FindArchiveEntries(videoPath, language, provider);
			if (archiveEntries.empty())
				continue;

			QString errorDescription;
			auto processed = contentProcessor.Process(archiveEntries, metadata, errorDescription);
			if (processed)
			{
				Apply(std::move(*processed), provider, language);
				return true;
			}
			cache.InvalidateArchiveEntries(language, provider, archiveEntries);
		}

		return false;
	}

	void HandleRequestOutcome(SubtitleRequestOutcome outcome)
	{
		if (outcome.context.videoPath != videoPath)
			return;

		if (!outcome.success)
		{
			emit loader.showErrorMessage(outcome.errorTitle, outcome.errorDescription);
			return;
		}

		QString errorDescription;
		bool wasArchive = false;
		auto payloads = contentProcessor.Expand(outcome.payload, wasArchive, errorDescription);
		if (payloads.empty())
		{
			emit loader.showErrorMessage(
				QStringLiteral("Failed to load subtitles."),
				errorDescription);
			return;
		}

		if (wasArchive)
		{
			QString cacheError;
			cache.StoreArchiveEntries(
				outcome.context.language,
				outcome.provider,
				payloads,
				cacheError);
		}

		auto processed = contentProcessor.Process(payloads, outcome.context.metadata, errorDescription);
		if (!processed)
		{
			emit loader.showErrorMessage(
				QStringLiteral("Failed to load subtitles."),
				errorDescription);
			return;
		}

		if (!wasArchive)
		{
			QString cacheError;
			cache.Store(outcome.context.videoPath, outcome.context.language, processed->cachePayload, cacheError);
		}
		Apply(std::move(*processed), outcome.provider, outcome.context.language);
		emit loader.subtitleDownloadSucceeded();
	}
};

SubtitleLoader::SubtitleLoader(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(*this))
{
}

SubtitleLoader::~SubtitleLoader() = default;

void SubtitleLoader::SetVideoFile(const QString & videoPath)
{
	if (m_impl->videoPath == videoPath)
		return;

	m_impl->requestCoordinator.Cancel();
	m_impl->videoPath = videoPath;
	m_impl->ClearPlayback();
}

void SubtitleLoader::SetUserData(const QVariantMap & userData)
{
	m_impl->requestCoordinator.SetUserData(userData);
}

void SubtitleLoader::SetImdbId(const QString & imdbId)
{
	const auto normalizedImdbId = imdbId.trimmed();
	if (m_impl->imdbId == normalizedImdbId)
		return;

	m_impl->requestCoordinator.Cancel();
	m_impl->imdbId = normalizedImdbId;
}

void SubtitleLoader::DownloadSubtitles(const QString & language)
{
	const auto normalizedLanguage = language.trimmed();
	if (m_impl->videoPath.isEmpty())
	{
		emit showErrorMessage(
			tr("No video selected."),
			tr("Open a video before downloading subtitles."));
		return;
	}
	if (normalizedLanguage.isEmpty())
	{
		emit showErrorMessage(
			tr("No subtitle language selected."),
			tr("Enter a subtitle language before downloading."));
		return;
	}

	if (m_impl->TryLoadCached(normalizedLanguage))
	{
		emit subtitleDownloadSucceeded();
		return;
	}

	m_impl->requestCoordinator.Request(
		m_impl->videoPath,
		normalizedLanguage,
		m_impl->imdbId,
		[this](SubtitleRequestOutcome outcome) {
			m_impl->HandleRequestOutcome(std::move(outcome));
		});
}

void SubtitleLoader::SetPlaybackPosition(qint64 positionMs)
{
	if (m_impl->playbackModel.SetPosition(positionMs))
		emit currentSubtitleTextChanged();
}

void SubtitleLoader::SetActiveSubtitleTrack(int trackIndex)
{
	const auto previousText = m_impl->playbackModel.GetCurrentText();
	if (!m_impl->playbackModel.SetActiveTrack(trackIndex))
		return;

	emit activeSubtitleTrackChanged();
	if (previousText != m_impl->playbackModel.GetCurrentText())
		emit currentSubtitleTextChanged();
}

int SubtitleLoader::GetActiveSubtitleTrack() const
{
	return m_impl->playbackModel.GetActiveTrack();
}

QStringList SubtitleLoader::GetSubtitleTracks() const
{
	return m_impl->playbackModel.GetTracks();
}

QString SubtitleLoader::GetCurrentSubtitleText() const
{
	return m_impl->playbackModel.GetCurrentText();
}
