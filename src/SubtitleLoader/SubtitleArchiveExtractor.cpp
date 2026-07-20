#include "SubtitleArchiveExtractor.h"

#include <QtCore/qtypes.h>
#include <algorithm>
#include <array>
#include <limits>
#include <optional>

#include <QFileInfo>
#include <QtEndian>

#include <zlib.h>

namespace TorrentPlayer::Subtitles {
namespace {

constexpr auto MAX_ARCHIVE_BYTES = qsizetype { 20 } * 1024 * 1024;
constexpr auto MAX_ENTRY_BYTES = qsizetype { 10 } * 1024 * 1024;
constexpr auto MAX_TOTAL_OUTPUT_BYTES = qsizetype { 30 } * 1024 * 1024;
constexpr auto MAX_ARCHIVE_ENTRIES = 128;

constexpr auto END_OF_CENTRAL_DIRECTORY_SIGNATURE = 0x06054b50;
constexpr auto CENTRAL_DIRECTORY_SIGNATURE = 0x02014b50;
constexpr auto LOCAL_FILE_SIGNATURE = 0x04034b50;

std::optional<quint16> Read16(const QByteArray & data, qsizetype offset)
{
	if (offset < 0 || offset + 2 > data.size())
		return std::nullopt;
	return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(data.constData() + offset));
}

std::optional<quint32> Read32(const QByteArray & data, qsizetype offset)
{
	if (offset < 0 || offset + 4 > data.size())
		return std::nullopt;
	return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(data.constData() + offset));
}

std::optional<qsizetype> FindEndOfCentralDirectory(const QByteArray & archive)
{
	static constexpr auto minimumRecordSize = 22;
	static constexpr auto maximumCommentSize = 65535;
	if (archive.size() < minimumRecordSize)
		return std::nullopt;

	const auto minimumOffset = std::max<qsizetype>(0, archive.size() - minimumRecordSize - maximumCommentSize);
	for (auto offset = archive.size() - minimumRecordSize; offset >= minimumOffset; --offset)
	{
		if (Read32(archive, offset) == END_OF_CENTRAL_DIRECTORY_SIGNATURE)
			return offset;
	}

	return std::nullopt;
}

std::optional<QByteArray> Decompress(
	const QByteArray & archive,
	qsizetype dataOffset,
	quint32 compressedSize,
	quint32 uncompressedSize,
	quint16 compression)
{
	if (uncompressedSize > MAX_ENTRY_BYTES || dataOffset < 0 || dataOffset + compressedSize > archive.size())
		return std::nullopt;

	if (compression == 0)
	{
		if (compressedSize != uncompressedSize)
			return std::nullopt;
		return archive.mid(dataOffset, compressedSize);
	}

	if (compression != 8)
		return std::nullopt;

	z_stream stream {};
	if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
		return std::nullopt;

	stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(archive.constData() + dataOffset));
	stream.avail_in = compressedSize;

	QByteArray result;
	result.reserve(uncompressedSize);
	std::array<char, 8192> buffer {};
	auto inflateResult = Z_OK;
	while (inflateResult == Z_OK)
	{
		stream.next_out = reinterpret_cast<Bytef *>(buffer.data());
		stream.avail_out = buffer.size();
		inflateResult = inflate(&stream, Z_NO_FLUSH);
		if (inflateResult != Z_OK && inflateResult != Z_STREAM_END)
			break;

		result.append(buffer.data(), static_cast<qsizetype>(buffer.size() - stream.avail_out));
		if (result.size() > MAX_ENTRY_BYTES || result.size() > static_cast<qsizetype>(uncompressedSize))
		{
			inflateResult = Z_MEM_ERROR;
			break;
		}
	}

	inflateEnd(&stream);
	if (inflateResult != Z_STREAM_END || result.size() != static_cast<qsizetype>(uncompressedSize))
		return std::nullopt;

	return result;
}

}

struct SubtitleArchiveExtractor::Impl
{
	std::vector<SubtitlePayload> Extract(const SubtitlePayload & archive, QString & errorDescription) const
	{
		std::vector<SubtitlePayload> result;
		if (archive.content.isEmpty() || archive.content.size() > MAX_ARCHIVE_BYTES)
		{
			errorDescription = "The subtitle archive is empty or too large.";
			return result;
		}

		const auto endOffset = FindEndOfCentralDirectory(archive.content);
		if (!endOffset)
		{
			errorDescription = "The subtitle archive has no valid central directory.";
			return result;
		}

		const auto diskNumber = Read16(archive.content, *endOffset + 4);
		const auto centralDisk = Read16(archive.content, *endOffset + 6);
		const auto entriesOnDisk = Read16(archive.content, *endOffset + 8);
		const auto totalEntries = Read16(archive.content, *endOffset + 10);
		const auto centralSize = Read32(archive.content, *endOffset + 12);
		const auto centralOffset = Read32(archive.content, *endOffset + 16);
		if (false
			|| !diskNumber
			|| !centralDisk
			|| !entriesOnDisk
			|| !totalEntries
			|| !centralSize
			|| !centralOffset
			|| *diskNumber != 0 || *centralDisk != 0 || *entriesOnDisk != *totalEntries
			|| *totalEntries > MAX_ARCHIVE_ENTRIES
			|| static_cast<quint64>(*centralOffset) + *centralSize > static_cast<quint64>(archive.content.size()))
		{
			errorDescription = "The subtitle archive uses an unsupported ZIP layout.";
			return result;
		}

		auto offset = static_cast<qsizetype>(*centralOffset);
		qsizetype totalOutputBytes = 0;
		for (auto index = 0; index < *totalEntries; ++index)
		{
			if (Read32(archive.content, offset) != CENTRAL_DIRECTORY_SIGNATURE || offset + 46 > archive.content.size())
			{
				errorDescription = "The subtitle archive central directory is corrupt.";
				return {};
			}

			const auto flags = Read16(archive.content, offset + 8);
			const auto compression = Read16(archive.content, offset + 10);
			const auto expectedCrc = Read32(archive.content, offset + 16);
			const auto compressedSize = Read32(archive.content, offset + 20);
			const auto uncompressedSize = Read32(archive.content, offset + 24);
			const auto nameLength = Read16(archive.content, offset + 28);
			const auto extraLength = Read16(archive.content, offset + 30);
			const auto commentLength = Read16(archive.content, offset + 32);
			const auto localOffset = Read32(archive.content, offset + 42);
			if (false
				|| !flags
				|| !compression
				|| !expectedCrc
				|| !compressedSize
				|| !uncompressedSize
				|| !nameLength
				|| !extraLength
				|| !commentLength
				|| !localOffset)
			{
				errorDescription = "The subtitle archive contains an invalid entry.";
				return {};
			}

			const auto nextOffset = offset + 46 + nameLength.value() + extraLength.value() + commentLength.value();
			if (nextOffset > archive.content.size())
			{
				errorDescription = "The subtitle archive contains an invalid entry.";
				return {};
			}

			const auto encodedName = archive.content.mid(offset + 46, nameLength.value());
			const auto entryName = flags.value() & (1U << 11)
									 ? QString::fromUtf8(encodedName)
									 : QString::fromLatin1(encodedName);
			const auto safeFileName = QFileInfo(entryName).fileName();
			const auto format = SubtitleFormatFromFileName(safeFileName);

			if (true
				&& IsSubtitleFormat(format)
				&& !(flags.value() & 1U)
				&& compressedSize.value() != std::numeric_limits<quint32>::max()
				&& uncompressedSize.value() != std::numeric_limits<quint32>::max())
			{
				if (Read32(archive.content, localOffset.value()) != LOCAL_FILE_SIGNATURE)
				{
					errorDescription = "The subtitle archive contains an invalid local entry.";
					return {};
				}

				const auto localNameLength = Read16(archive.content, localOffset.value() + 26);
				const auto localExtraLength = Read16(archive.content, localOffset.value() + 28);
				if (!localNameLength || !localExtraLength)
					return {};
				const auto dataOffset = static_cast<qsizetype>(localOffset.value()) + 30 + localNameLength.value() + localExtraLength.value();
				const auto content = Decompress(archive.content, dataOffset, compressedSize.value(), uncompressedSize.value(), compression.value());
				if (content)
				{
					const auto actualCrc = crc32(0L, reinterpret_cast<const Bytef *>(content->constData()), content->size());
					if (actualCrc == expectedCrc.value())
					{
						totalOutputBytes += content->size();
						if (totalOutputBytes > MAX_TOTAL_OUTPUT_BYTES)
						{
							errorDescription = "The expanded subtitle archive is too large.";
							return {};
						}
						result.push_back({ archive.provider, safeFileName, format, *content });
					}
				}
			}

			offset = nextOffset;
		}

		if (result.empty())
			errorDescription = "The archive contains no supported subtitle files.";
		return result;
	}
};

SubtitleArchiveExtractor::SubtitleArchiveExtractor()
	: m_impl(std::make_unique<Impl>())
{
}

SubtitleArchiveExtractor::~SubtitleArchiveExtractor() = default;

std::vector<SubtitlePayload> SubtitleArchiveExtractor::Extract(
	const SubtitlePayload & archive,
	QString & errorDescription) const
{
	return m_impl->Extract(archive, errorDescription);
}

} // namespace TorrentPlayer::Subtitles
