#include "App/Models/SubtitleLanguageModel.h"

#include <QtCore/qvariant.h>
#include <array>

namespace {

constexpr auto DEFAULT_SUBTITLE_LANGUAGE = "EN";

// ISO 639-1 codes accepted by both SubDL (uppercase) and OpenSubtitles (lowercase).
constexpr std::array<std::pair<const char *, const char *>, 53> SUBTITLE_LANGUAGES {{
	{ "SQ", "Albanian" },
	{ "AR", "Arabic" },
	{ "BE", "Belarusian" },
	{ "BN", "Bengali" },
	{ "BS", "Bosnian" },
	{ "BG", "Bulgarian" },
	{ "MY", "Burmese" },
	{ "CA", "Catalan" },
	{ "HR", "Croatian" },
	{ "CS", "Czech" },
	{ "DA", "Danish" },
	{ "NL", "Dutch" },
	{ "EN", "English" },
	{ "EO", "Esperanto" },
	{ "ET", "Estonian" },
	{ "FA", "Persian" },
	{ "FI", "Finnish" },
	{ "FR", "French" },
	{ "KA", "Georgian" },
	{ "DE", "German" },
	{ "EL", "Greek" },
	{ "HE", "Hebrew" },
	{ "HI", "Hindi" },
	{ "HU", "Hungarian" },
	{ "IS", "Icelandic" },
	{ "ID", "Indonesian" },
	{ "IT", "Italian" },
	{ "JA", "Japanese" },
	{ "KO", "Korean" },
	{ "KU", "Kurdish" },
	{ "LV", "Latvian" },
	{ "LT", "Lithuanian" },
	{ "MK", "Macedonian" },
	{ "MS", "Malay" },
	{ "ML", "Malayalam" },
	{ "NO", "Norwegian" },
	{ "PL", "Polish" },
	{ "RO", "Romanian" },
	{ "RU", "Russian" },
	{ "SR", "Serbian" },
	{ "SI", "Sinhalese" },
	{ "SK", "Slovak" },
	{ "SL", "Slovenian" },
	{ "ES", "Spanish" },
	{ "SV", "Swedish" },
	{ "TL", "Tagalog" },
	{ "TA", "Tamil" },
	{ "TE", "Telugu" },
	{ "TH", "Thai" },
	{ "TR", "Turkish" },
	{ "UK", "Ukrainian" },
	{ "UR", "Urdu" },
	{ "VI", "Vietnamese" },
}};

}

SubtitleLanguageModel::SubtitleLanguageModel(QObject * parent)
	: QAbstractListModel(parent)
{
}

int SubtitleLanguageModel::rowCount(const QModelIndex & parent) const
{
	if (parent.isValid())
		return 0;
	return static_cast<int>(SUBTITLE_LANGUAGES.size());
}

QVariant SubtitleLanguageModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(SUBTITLE_LANGUAGES.size()))
		return {};

	const auto & [code, name] = SUBTITLE_LANGUAGES.at(static_cast<size_t>(index.row()));
	switch (role)
	{
		case CodeRole:
			return code;
		case NameRole:
			return name;
		default:
			return {};
	}
}

QHash<int, QByteArray> SubtitleLanguageModel::roleNames() const
{
#define RoleName(NAME) { NAME, #NAME }
	return {
		RoleName(CodeRole),
		RoleName(NameRole),
	};
#undef RoleName
}

int SubtitleLanguageModel::IndexOfCode(const QString & code) const
{
	const auto normalized = code.trimmed().toUpper();
	auto englishIndex = 0;
	for (auto i = 0; i < static_cast<int>(SUBTITLE_LANGUAGES.size()); ++i)
	{
		const auto languageCode = QString::fromStdString(SUBTITLE_LANGUAGES.at(static_cast<size_t>(i)).first);
		if (languageCode == normalized)
			return i;
		if (languageCode == DEFAULT_SUBTITLE_LANGUAGE)
			englishIndex = i;
	}
	return englishIndex;
}
