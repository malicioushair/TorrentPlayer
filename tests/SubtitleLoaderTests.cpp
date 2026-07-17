#include <gtest/gtest.h>

#include <QByteArray>
#include <QDir>
#include <QString>
#include <QTemporaryDir>

#include "App/SubtitleLoader/SubtitleCache.h"
#include "App/SubtitleLoader/SubtitleContentProcessor.h"
#include "App/SubtitleLoader/SubtitleParser.h"
#include "App/SubtitleLoader/SubtitlePlaybackModel.h"
#include "App/SubtitleLoader/SubtitleTypes.h"

using namespace TorrentPlayer::Subtitles;

TEST(SubtitleParserTest, ParsesSrtAndSelectsCuesForPlayback)
{
	const SubtitlePayload payload {
		SubtitleProviderId::Subdl,
		QStringLiteral("example.srt"),
		SubtitleFormat::Srt,
		QByteArrayLiteral(
			"1\n"
			"00:00:01,000 --> 00:00:02,000\n"
			"First cue\n\n"
			"2\n"
			"00:00:03,000 --> 00:00:04,000\n"
			"Second cue\n"),
	};

	SubtitleParser parser;
	QString errorDescription;
	auto document = parser.Parse(payload, &errorDescription);
	ASSERT_TRUE(document.has_value()) << errorDescription.toStdString();
	ASSERT_EQ(document->cues.size(), 2);

	SubtitlePlaybackModel playbackModel;
	document->label = QStringLiteral("English");
	playbackModel.AddOrReplaceTrack(std::move(*document));
	EXPECT_EQ(playbackModel.GetActiveTrack(), 0);
	EXPECT_TRUE(playbackModel.SetPosition(1000));
	EXPECT_EQ(playbackModel.GetCurrentText(), QStringLiteral("First cue"));
	EXPECT_TRUE(playbackModel.SetPosition(2000));
	EXPECT_TRUE(playbackModel.GetCurrentText().isEmpty());
}

TEST(SubtitleContentProcessorTest, ExtractsAndParsesZipPayload)
{
	const auto archive = QByteArray::fromBase64(QByteArrayLiteral(
		"UEsDBBQAAAAIAPO771x+j1LfJAAAACwAAAAlABwAdG9ycmVudHBsYXllci1zdWJ0aXRsZS10ZXN0LWVudHJ5LnNydFVUCQADOvxXajr8V2p1eAsAAQT1AQAABAAAAAAz5DIwsAIhQx0DAwMFXV07BaiAEUiAy7EoOSOzLFUhuTSVCwBQSwECHgMUAAAACADzu+9cfo9S3yQAAAAsAAAAJQAYAAAAAAABAAAApIEAAAAAdG9ycmVudHBsYXllci1zdWJ0aXRsZS10ZXN0LWVudHJ5LnNydFVUBQADOvxXanV4CwABBPUBAAAEAAAAAFBLBQYAAAAAAQABAGsAAACDAAAAAAA="));
	const SubtitlePayload payload {
		SubtitleProviderId::OpenSubtitles,
		QStringLiteral("subtitles.zip"),
		SubtitleFormat::Unknown,
		archive,
	};

	SubtitleContentProcessor processor;
	QString errorDescription;
	bool wasArchive = false;
	const auto extracted = processor.Expand(payload, wasArchive, errorDescription);
	ASSERT_TRUE(wasArchive);
	ASSERT_EQ(extracted.size(), 1);
	const auto processed = processor.Process(extracted, {}, errorDescription);
	ASSERT_TRUE(processed.has_value()) << errorDescription.toStdString();
	EXPECT_EQ(processed->cachePayload.format, SubtitleFormat::Srt);
	ASSERT_FALSE(processed->document.cues.empty());
	EXPECT_EQ(processed->document.cues.front().text, QStringLiteral("Archive cue"));
}

TEST(SubtitleParserTest, RejectsInvalidSubtitle)
{
	const SubtitlePayload payload {
		SubtitleProviderId::Subdl,
		QStringLiteral("invalid.srt"),
		SubtitleFormat::Srt,
		QByteArrayLiteral("not a subtitle"),
	};

	SubtitleParser parser;
	QString errorDescription;
	EXPECT_FALSE(parser.Parse(payload, &errorDescription).has_value());
	EXPECT_FALSE(errorDescription.isEmpty());
}

TEST(SubtitleCacheTest, StoresArchiveEntriesAndFindsTheCurrentEpisode)
{
	QTemporaryDir temporaryDirectory;
	ASSERT_TRUE(temporaryDirectory.isValid());
	SubtitleCache cache(temporaryDirectory.path());

	const std::vector<SubtitlePayload> payloads {
		{
         SubtitleProviderId::OpenSubtitles,
         QStringLiteral("Black.Sun.S01E07.srt"),
         SubtitleFormat::Srt,
         QByteArrayLiteral("1\n00:00:01,000 --> 00:00:02,000\nEpisode 7\n"),
		 },
		{
         SubtitleProviderId::OpenSubtitles,
         QStringLiteral("Black.Sun.S01E08.srt"),
         SubtitleFormat::Srt,
         QByteArrayLiteral("1\n00:00:01,000 --> 00:00:02,000\nEpisode 8\n"),
		 },
	};

	QString errorDescription;
	VideoSearchMetadata metadata;
	metadata.season = 1;
	metadata.episode = 8;
	SubtitleContentProcessor processor;
	const auto selected = processor.Process(payloads, metadata, errorDescription);
	ASSERT_TRUE(selected.has_value()) << errorDescription.toStdString();
	ASSERT_FALSE(selected->document.cues.empty());
	EXPECT_EQ(selected->document.cues.front().text, QStringLiteral("Episode 8"));

	ASSERT_TRUE(cache.StoreArchiveEntries(
		QStringLiteral("ru"),
		SubtitleProviderId::OpenSubtitles,
		payloads,
		errorDescription))
		<< errorDescription.toStdString();

	const QDir cacheDirectory(temporaryDirectory.path());
	EXPECT_TRUE(cacheDirectory.exists(QStringLiteral("Black.Sun.S01E07--ru--opensubtitles.srt")));
	EXPECT_TRUE(cacheDirectory.exists(QStringLiteral("Black.Sun.S01E08--ru--opensubtitles.srt")));

	const auto cached = cache.FindArchiveEntries(
		QStringLiteral("/videos/Black.Sun.S01E08.1080p.mkv"),
		QStringLiteral("ru"),
		SubtitleProviderId::OpenSubtitles);
	ASSERT_EQ(cached.size(), 1);
	EXPECT_EQ(cached.front().fileName, QStringLiteral("Black.Sun.S01E08.srt"));

	const auto processed = processor.Process(cached, metadata, errorDescription);
	ASSERT_TRUE(processed.has_value()) << errorDescription.toStdString();
	ASSERT_FALSE(processed->document.cues.empty());
	EXPECT_EQ(processed->document.cues.front().text, QStringLiteral("Episode 8"));
}
