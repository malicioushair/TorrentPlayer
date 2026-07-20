#include "SubdlProvider.h"

#include <utility>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QUrl>
#include <QUrlQuery>

namespace TorrentPlayer::Subtitles {
namespace {

constexpr auto SUBDL_API_KEY = "subDlApiKey";
constexpr auto MAX_NETWORK_PAYLOAD_BYTES = 20 * 1024 * 1024;
constexpr auto NETWORK_TIMEOUT_MS = 15000;

QNetworkRequest MakeRequest(const QUrl & url)
{
	QNetworkRequest request(url);
	request.setRawHeader("Accept", "application/json");
	request.setTransferTimeout(NETWORK_TIMEOUT_MS);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	return request;
}

bool ReadLimitedReply(QNetworkReply & reply, QByteArray * content, QString * errorDescription)
{
	const auto contentLength = reply.header(QNetworkRequest::ContentLengthHeader).toLongLong();
	if (contentLength > MAX_NETWORK_PAYLOAD_BYTES)
	{
		*errorDescription = QStringLiteral("The subtitle provider returned an unexpectedly large response.");
		return false;
	}

	*content = reply.readAll();
	if (content->size() > MAX_NETWORK_PAYLOAD_BYTES)
	{
		*errorDescription = QStringLiteral("The subtitle provider response exceeded the size limit.");
		return false;
	}

	return true;
}

QUrl ResolveDownloadUrl(const QString & value)
{
	const QUrl url(value);
	return url.isRelative() ? QUrl(QStringLiteral("https://dl.subdl.com/")).resolved(url) : url;
}

QUrl ResolveSubtitleUrl(const QJsonObject & subtitle)
{
	const auto unpackFiles = subtitle.value(QStringLiteral("unpack_files")).toArray();
	if (!unpackFiles.empty())
		return ResolveDownloadUrl(unpackFiles.first().toObject().value(QStringLiteral("url")).toString());

	const auto downloadLink = subtitle.value(QStringLiteral("download_link")).toString();
	if (!downloadLink.isEmpty())
		return ResolveDownloadUrl(downloadLink);

	return ResolveDownloadUrl(subtitle.value(QStringLiteral("url")).toString());
}

}

struct SubdlProvider::Impl
{
	QNetworkAccessManager networkManager;
	QSet<QNetworkReply *> activeReplies;
	QVariantMap userData;
	quint64 generation {};

	void Cancel()
	{
		++generation;
		const auto replies = activeReplies;
		for (auto * reply : replies)
			reply->abort();
	}

	void Request(const SubtitleRequestContext & context, Completion completion)
	{
		Cancel();
		const auto requestGeneration = ++generation;
		const auto apiKey = userData.value(SUBDL_API_KEY).toString().isEmpty() ? DEFAULT_SUBDL_API_KEY : userData.value(SUBDL_API_KEY).toString().trimmed();

		QUrlQuery query;
		query.addQueryItem(QStringLiteral("api_key"), apiKey);
		query.addQueryItem(QStringLiteral("file_name"), QFileInfo(context.videoPath).fileName());
		query.addQueryItem(QStringLiteral("languages"), context.language.toUpper());
		query.addQueryItem(QStringLiteral("subs_per_page"), QStringLiteral("1"));
		query.addQueryItem(QStringLiteral("unpack"), QStringLiteral("1"));
		if (!context.metadata.imdbId.isEmpty())
			query.addQueryItem(QStringLiteral("imdb_id"), context.metadata.imdbId);
		if (context.metadata.season)
			query.addQueryItem(QStringLiteral("season_number"), QString::number(*context.metadata.season));
		if (context.metadata.episode)
			query.addQueryItem(QStringLiteral("episode_number"), QString::number(*context.metadata.episode));
		if (context.metadata.season && context.metadata.episode)
			query.addQueryItem(QStringLiteral("type"), QStringLiteral("tv"));

		QUrl url(QStringLiteral("https://api.subdl.com/api/v1/subtitles"));
		url.setQuery(query);
		auto * reply = networkManager.get(MakeRequest(url));
		activeReplies.insert(reply);
		QObject::connect(reply, &QNetworkReply::finished, &networkManager, [this, reply, context, apiKey, completion, requestGeneration] {
			activeReplies.remove(reply);
			reply->deleteLater();
			if (requestGeneration != generation)
				return;

			if (reply->error() != QNetworkReply::NoError)
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::Subdl, {}, reply->errorString() });
				return;
			}

			QByteArray body;
			QString errorDescription;
			if (!ReadLimitedReply(*reply, &body, &errorDescription))
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::Subdl, {}, errorDescription });
				return;
			}

			QJsonParseError parseError;
			const auto document = QJsonDocument::fromJson(body, &parseError);
			if (parseError.error != QJsonParseError::NoError || !document.isObject())
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::Subdl, {}, QStringLiteral("SubDL returned invalid JSON.") });
				return;
			}

			const auto subtitles = document.object().value(QStringLiteral("subtitles")).toArray();
			if (subtitles.empty())
			{
				completion({ ProviderRequestStatus::NotFound, SubtitleProviderId::Subdl, {}, QStringLiteral("SubDL found no matching subtitles.") });
				return;
			}

			const auto subtitleUrl = ResolveSubtitleUrl(subtitles.first().toObject());
			if (!subtitleUrl.isValid() || subtitleUrl.isEmpty() || subtitleUrl.scheme() != QStringLiteral("https"))
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::Subdl, {}, QStringLiteral("SubDL returned an invalid download URL.") });
				return;
			}

			Download(subtitleUrl, apiKey, completion, requestGeneration);
		});
	}

	void Download(const QUrl & url, const QString & apiKey, Completion completion, quint64 requestGeneration)
	{
		auto request = MakeRequest(url);
		request.setRawHeader("Accept", "*/*");
		request.setRawHeader("x-api-key", apiKey.toUtf8());
		auto * reply = networkManager.get(request);
		activeReplies.insert(reply);
		QObject::connect(reply, &QNetworkReply::finished, &networkManager, [this, reply, url, completion, requestGeneration] {
			activeReplies.remove(reply);
			reply->deleteLater();
			if (requestGeneration != generation)
				return;

			if (reply->error() != QNetworkReply::NoError)
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::Subdl, {}, reply->errorString() });
				return;
			}

			QByteArray content;
			QString errorDescription;
			if (!ReadLimitedReply(*reply, &content, &errorDescription))
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::Subdl, {}, errorDescription });
				return;
			}

			auto fileName = QFileInfo(url.path()).fileName();
			if (fileName.isEmpty())
				fileName = QStringLiteral("subdl-subtitle.srt");
			SubtitlePayload payload {
				SubtitleProviderId::Subdl,
				fileName,
				SubtitleFormatFromFileName(fileName),
				std::move(content),
			};
			completion({ ProviderRequestStatus::Success, SubtitleProviderId::Subdl, std::move(payload), {} });
		});
	}
};

SubdlProvider::SubdlProvider()
	: m_impl(std::make_unique<Impl>())
{
}

SubdlProvider::~SubdlProvider()
{
	m_impl->Cancel();
}

SubtitleProviderId SubdlProvider::GetId() const
{
	return SubtitleProviderId::Subdl;
}

void SubdlProvider::SetUserData(const QVariantMap & userData)
{
	m_impl->Cancel();
	m_impl->userData = userData;
}

bool SubdlProvider::IsConfigured() const
{
	return !m_impl->userData.value(SUBDL_API_KEY).toString().trimmed().isEmpty() || !QStringLiteral(DEFAULT_SUBDL_API_KEY).isEmpty();
}

void SubdlProvider::Request(const SubtitleRequestContext & context, Completion completion)
{
	m_impl->Request(context, std::move(completion));
}

void SubdlProvider::Cancel()
{
	m_impl->Cancel();
}

} // namespace TorrentPlayer::Subtitles
