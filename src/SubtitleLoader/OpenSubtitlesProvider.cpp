#include "OpenSubtitlesProvider.h"

#include <utility>
#include <vector>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace TorrentPlayer::Subtitles {
namespace {

constexpr auto USERNAME = "openSubtitlesUsername";
constexpr auto PASSWORD = "openSubtitlesPassword";
constexpr auto RECENT_IMDB_ID = "recentImdbId";
constexpr auto USER_AGENT = "TorrentPlayer v0.1.0";
constexpr qsizetype MAX_NETWORK_PAYLOAD_BYTES = 20 * 1024 * 1024;
constexpr auto NETWORK_TIMEOUT_MS = 15000;

enum class SearchMode
{
	Imdb,
	MovieHash,
	FileName,
};

void ApplyHeaders(QNetworkRequest & request, const QString & apiKey, const QString & token = {})
{
	request.setRawHeader("Accept", "application/json");
	request.setRawHeader("Api-Key", apiKey.toUtf8());
	request.setRawHeader("User-Agent", USER_AGENT);
	request.setTransferTimeout(NETWORK_TIMEOUT_MS);
	if (!token.isEmpty())
		request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(token).toUtf8());
}

bool ReadLimitedReply(QNetworkReply & reply, QByteArray * content, QString * errorDescription)
{
	const auto contentLength = reply.header(QNetworkRequest::ContentLengthHeader).toLongLong();
	if (contentLength > MAX_NETWORK_PAYLOAD_BYTES)
	{
		*errorDescription = QStringLiteral("OpenSubtitles returned an unexpectedly large response.");
		return false;
	}

	*content = reply.readAll();
	if (content->size() > MAX_NETWORK_PAYLOAD_BYTES)
	{
		*errorDescription = QStringLiteral("The OpenSubtitles response exceeded the size limit.");
		return false;
	}

	return true;
}

QString HttpErrorDescription(const QNetworkReply & reply)
{
	const auto status = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (status == 401 || status == 403)
		return QStringLiteral("OpenSubtitles rejected the configured API key or account credentials (HTTP %1).").arg(status);
	if (status == 429)
		return QStringLiteral("OpenSubtitles rate-limited the request (HTTP 429). Please try again shortly.");
	if (status == 502 || status == 503)
		return QStringLiteral("OpenSubtitles is temporarily unavailable (HTTP %1).").arg(status);
	return reply.errorString();
}

std::vector<SearchMode> BuildSearchModes(const VideoSearchMetadata & metadata)
{
	QSettings settings;
	std::vector<SearchMode> result;
	if (!metadata.imdbId.isEmpty() || settings.value(RECENT_IMDB_ID).isValid())
		result.push_back(SearchMode::Imdb);
	if (!metadata.movieHash.isEmpty() && metadata.movieFileSize > 0)
		result.push_back(SearchMode::MovieHash);
	result.push_back(SearchMode::FileName);
	return result;
}

QUrl BuildSearchUrl(const SubtitleRequestContext & context, SearchMode mode)
{
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("languages"), context.language.toLower());
	if (context.metadata.season)
		query.addQueryItem(QStringLiteral("season_number"), QString::number(*context.metadata.season));
	if (context.metadata.episode)
		query.addQueryItem(QStringLiteral("episode_number"), QString::number(*context.metadata.episode));

	switch (mode)
	{
		case SearchMode::Imdb:
		{
			QSettings settings;
			const auto imdbId = settings.value(RECENT_IMDB_ID).toString();
			query.addQueryItem(QStringLiteral("imdb_id"), !imdbId.isEmpty() ? imdbId : context.metadata.imdbId.mid(2));
			break;
		}
		case SearchMode::MovieHash:
			query.addQueryItem(QStringLiteral("moviehash"), context.metadata.movieHash);
			query.addQueryItem(QStringLiteral("moviebytesize"), QString::number(context.metadata.movieFileSize));
			break;
		case SearchMode::FileName:
			query.addQueryItem(QStringLiteral("query"), QFileInfo(context.videoPath).completeBaseName());
			break;
	}

	QUrl url(QStringLiteral("https://api.opensubtitles.com/api/v1/subtitles"));
	url.setQuery(query);
	return url;
}

}

struct OpenSubtitlesProvider::Impl
{
	QNetworkAccessManager networkManager;
	QSet<QNetworkReply *> activeReplies;
	QVariantMap userData;
	QString token;
	quint64 generation {};

	void Cancel()
	{
		++generation;
		const auto replies = activeReplies;
		for (auto * reply : replies)
			reply->abort();
	}

	void Request(const SubtitleRequestContext & context, ISubtitleProvider::Completion completion)
	{
		Cancel();
		const auto requestGeneration = ++generation;
		const auto username = userData.value(USERNAME).toString().trimmed();
		const auto password = userData.value(PASSWORD).toString();
		if (username.isEmpty() != password.isEmpty())
		{
			completion({
				ProviderRequestStatus::Failed,
				SubtitleProviderId::OpenSubtitles,
				{},
				QStringLiteral("Both an OpenSubtitles username and password are required when using an account."),
			});
			return;
		}

		if (!username.isEmpty() && token.isEmpty())
			Login(context, std::move(completion), requestGeneration);
		else
			StartSearch(context, std::move(completion), requestGeneration);
	}

	void Login(
		const SubtitleRequestContext & context,
		ISubtitleProvider::Completion completion,
		quint64 requestGeneration)
	{
		const auto apiKey = OPEN_SUBTITLES_API_KEY;
		QNetworkRequest request(QUrl(QStringLiteral("https://api.opensubtitles.com/api/v1/login")));
		request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
		ApplyHeaders(request, apiKey);

		QJsonObject body;
		body.insert(QStringLiteral("username"), userData.value(USERNAME).toString().trimmed());
		body.insert(QStringLiteral("password"), userData.value(PASSWORD).toString());

		auto * reply = networkManager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
		qDebug() << "FOO: " << request.url();
		activeReplies.insert(reply);
		QObject::connect(reply, &QNetworkReply::finished, &networkManager, [this, reply, context, completion, requestGeneration] {
			activeReplies.remove(reply);
			reply->deleteLater();
			if (requestGeneration != generation)
				return;

			if (reply->error() != QNetworkReply::NoError)
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, HttpErrorDescription(*reply) });
				return;
			}

			QByteArray response;
			QString errorDescription;
			if (!ReadLimitedReply(*reply, &response, &errorDescription))
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, errorDescription });
				return;
			}

			QJsonParseError parseError;
			const auto document = QJsonDocument::fromJson(response, &parseError);
			token = document.object().value(QStringLiteral("token")).toString();
			if (parseError.error != QJsonParseError::NoError || token.isEmpty())
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, QStringLiteral("OpenSubtitles login returned an invalid response.") });
				return;
			}

			StartSearch(context, completion, requestGeneration);
		});
	}

	void StartSearch(
		const SubtitleRequestContext & context,
		ISubtitleProvider::Completion completion,
		quint64 requestGeneration)
	{
		Search(context, std::move(completion), BuildSearchModes(context.metadata), 0, 0, requestGeneration);
	}

	void Search(
		const SubtitleRequestContext & context,
		ISubtitleProvider::Completion completion,
		const std::vector<SearchMode> & modes,
		size_t modeIndex,
		int retryAttempt,
		quint64 requestGeneration)
	{
		if (requestGeneration != generation)
			return;
		if (modeIndex >= modes.size())
		{
			completion({ ProviderRequestStatus::NotFound, SubtitleProviderId::OpenSubtitles, {}, QStringLiteral("OpenSubtitles found no matching subtitles.") });
			return;
		}

		const auto apiKey = OPEN_SUBTITLES_API_KEY;
		QNetworkRequest request(BuildSearchUrl(context, modes[modeIndex]));
		ApplyHeaders(request, apiKey, token);
		auto * reply = networkManager.get(request);
		activeReplies.insert(reply);
		QObject::connect(reply, &QNetworkReply::finished, &networkManager, [this, reply, context, completion, modes, modeIndex, retryAttempt, requestGeneration] {
			activeReplies.remove(reply);
			reply->deleteLater();
			if (requestGeneration != generation)
				return;

			if (reply->error() != QNetworkReply::NoError)
			{
				const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
				if ((status == 429 || status == 502 || status == 503) && retryAttempt < 2)
				{
					const auto delayMs = 500 * (1 << retryAttempt);
					QTimer::singleShot(delayMs, &networkManager, [this, context, completion, modes, modeIndex, retryAttempt, requestGeneration] {
						Search(context, completion, modes, modeIndex, retryAttempt + 1, requestGeneration);
					});
					return;
				}

				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, HttpErrorDescription(*reply) });
				return;
			}

			QByteArray response;
			QString errorDescription;
			if (!ReadLimitedReply(*reply, &response, &errorDescription))
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, errorDescription });
				return;
			}

			QJsonParseError parseError;
			const auto document = QJsonDocument::fromJson(response, &parseError);
			if (parseError.error != QJsonParseError::NoError || !document.isObject())
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, QStringLiteral("OpenSubtitles returned invalid JSON.") });
				return;
			}

			const auto data = document.object().value(QStringLiteral("data")).toArray();
			if (data.empty())
			{
				Search(context, completion, modes, modeIndex + 1, 0, requestGeneration);
				return;
			}

			const auto files = data.first().toObject()
				.value(QStringLiteral("attributes")).toObject()
				.value(QStringLiteral("files")).toArray();
			const auto fileId = files.empty() ? 0 : files.first().toObject().value(QStringLiteral("file_id")).toInt();
			if (fileId <= 0)
			{
				Search(context, completion, modes, modeIndex + 1, 0, requestGeneration);
				return;
			}

			RequestDownloadLink(fileId, completion, requestGeneration);
		});
	}

	void RequestDownloadLink(int fileId, ISubtitleProvider::Completion completion, quint64 requestGeneration)
	{
		const auto apiKey = OPEN_SUBTITLES_API_KEY;
		QNetworkRequest request(QUrl(QStringLiteral("https://api.opensubtitles.com/api/v1/download")));
		request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
		ApplyHeaders(request, apiKey, token);

		QJsonObject body;
		body.insert(QStringLiteral("file_id"), fileId);
		auto * reply = networkManager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
		activeReplies.insert(reply);
		QObject::connect(reply, &QNetworkReply::finished, &networkManager, [this, reply, completion, requestGeneration] {
			activeReplies.remove(reply);
			reply->deleteLater();
			if (requestGeneration != generation)
				return;

			if (reply->error() != QNetworkReply::NoError)
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, HttpErrorDescription(*reply) });
				return;
			}

			QByteArray response;
			QString errorDescription;
			if (!ReadLimitedReply(*reply, &response, &errorDescription))
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, errorDescription });
				return;
			}

			QJsonParseError parseError;
			const auto document = QJsonDocument::fromJson(response, &parseError);
			const QUrl link(document.object().value(QStringLiteral("link")).toString());
			if (parseError.error != QJsonParseError::NoError
				|| !link.isValid()
				|| link.isEmpty()
				|| link.scheme() != QStringLiteral("https"))
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, QStringLiteral("OpenSubtitles returned an invalid download link.") });
				return;
			}

			Download(link, completion, requestGeneration);
		});
	}

	void Download(const QUrl & url, ISubtitleProvider::Completion completion, quint64 requestGeneration)
	{
		QNetworkRequest request(url);
		request.setTransferTimeout(NETWORK_TIMEOUT_MS);
		request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
		auto * reply = networkManager.get(request);
		activeReplies.insert(reply);
		QObject::connect(reply, &QNetworkReply::finished, &networkManager, [this, reply, url, completion, requestGeneration] {
			activeReplies.remove(reply);
			reply->deleteLater();
			if (requestGeneration != generation)
				return;

			if (reply->error() != QNetworkReply::NoError)
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, reply->errorString() });
				return;
			}

			QByteArray content;
			QString errorDescription;
			if (!ReadLimitedReply(*reply, &content, &errorDescription))
			{
				completion({ ProviderRequestStatus::Failed, SubtitleProviderId::OpenSubtitles, {}, errorDescription });
				return;
			}

			auto fileName = QFileInfo(url.path()).fileName();
			if (fileName.isEmpty())
				fileName = QStringLiteral("opensubtitles-subtitle.srt");
			SubtitlePayload payload {
				SubtitleProviderId::OpenSubtitles,
				fileName,
				SubtitleFormatFromFileName(fileName),
				std::move(content),
			};
			completion({ ProviderRequestStatus::Success, SubtitleProviderId::OpenSubtitles, std::move(payload), {} });
		});
	}
};

OpenSubtitlesProvider::OpenSubtitlesProvider()
	: m_impl(std::make_unique<Impl>())
{
}

OpenSubtitlesProvider::~OpenSubtitlesProvider()
{
	m_impl->Cancel();
}

SubtitleProviderId OpenSubtitlesProvider::GetId() const
{
	return SubtitleProviderId::OpenSubtitles;
}

void OpenSubtitlesProvider::SetUserData(const QVariantMap & userData)
{
	m_impl->Cancel();
	m_impl->userData = userData;
	m_impl->token.clear();
}

bool OpenSubtitlesProvider::IsConfigured() const
{
	// OPEN_SUBTITLES_API_KEY is required for open subtitles
	return !QStringLiteral(OPEN_SUBTITLES_API_KEY).isEmpty();
}

void OpenSubtitlesProvider::Request(const SubtitleRequestContext & context, Completion completion)
{
	m_impl->Request(context, std::move(completion));
}

void OpenSubtitlesProvider::Cancel()
{
	m_impl->Cancel();
}

} // namespace TorrentPlayer::Subtitles
