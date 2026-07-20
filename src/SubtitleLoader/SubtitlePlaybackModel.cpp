#include "SubtitlePlaybackModel.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

namespace TorrentPlayer::Subtitles {

struct SubtitlePlaybackModel::Impl
{
	std::vector<SubtitleDocument> documents;
	int activeTrack { -1 };
	qint64 positionMs {};
	QString currentText;

	QString ResolveCurrentText() const
	{
		if (activeTrack < 0 || activeTrack >= static_cast<int>(documents.size()))
			return {};

		const auto & cues = documents[activeTrack].cues;
		const auto nextCue = std::ranges::upper_bound(cues, positionMs, {}, &SubtitleCue::startMs);
		if (nextCue == cues.begin())
			return {};

		const auto & cue = *std::prev(nextCue);
		return positionMs >= cue.startMs && positionMs < cue.endMs ? cue.text : QString();
	}

	bool UpdateCurrentText()
	{
		const auto text = ResolveCurrentText();
		if (text == currentText)
			return false;

		currentText = text;
		return true;
	}
};

SubtitlePlaybackModel::SubtitlePlaybackModel()
	: m_impl(std::make_unique<Impl>())
{
}

SubtitlePlaybackModel::~SubtitlePlaybackModel() = default;

void SubtitlePlaybackModel::Clear()
{
	m_impl->documents.clear();
	m_impl->activeTrack = -1;
	m_impl->positionMs = 0;
	m_impl->currentText.clear();
}

void SubtitlePlaybackModel::AddOrReplaceTrack(SubtitleDocument document)
{
	const auto existing = std::ranges::find(m_impl->documents, document.label, &SubtitleDocument::label);
	if (existing == m_impl->documents.end())
	{
		m_impl->documents.push_back(std::move(document));
		m_impl->activeTrack = static_cast<int>(m_impl->documents.size()) - 1;
	}
	else
	{
		*existing = std::move(document);
		m_impl->activeTrack = static_cast<int>(std::distance(m_impl->documents.begin(), existing));
	}

	m_impl->UpdateCurrentText();
}

bool SubtitlePlaybackModel::SetActiveTrack(int trackIndex)
{
	if (trackIndex < -1 || trackIndex >= static_cast<int>(m_impl->documents.size()) || trackIndex == m_impl->activeTrack)
		return false;

	m_impl->activeTrack = trackIndex;
	m_impl->UpdateCurrentText();
	return true;
}

bool SubtitlePlaybackModel::SetPosition(qint64 positionMs)
{
	m_impl->positionMs = std::max<qint64>(0, positionMs);
	return m_impl->UpdateCurrentText();
}

int SubtitlePlaybackModel::GetActiveTrack() const
{
	return m_impl->activeTrack;
}

QStringList SubtitlePlaybackModel::GetTracks() const
{
	QStringList result;
	for (const auto & document : m_impl->documents)
		result.push_back(document.label);
	return result;
}

QString SubtitlePlaybackModel::GetCurrentText() const
{
	return m_impl->currentText;
}

} // namespace TorrentPlayer::Subtitles
