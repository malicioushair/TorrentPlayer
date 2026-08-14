#pragma once

#include <QAbstractListModel>
#include <QString>

class SubtitleLanguageModel
	: public QAbstractListModel
{
	Q_OBJECT

public:
	enum Roles
	{
		CodeRole = Qt::UserRole + 1,
		NameRole,
	};

	explicit SubtitleLanguageModel(QObject * parent = nullptr);

	int rowCount(const QModelIndex & parent = QModelIndex()) const final;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const final;
	QHash<int, QByteArray> roleNames() const final;

	int IndexOfCode(const QString & code) const;
};
