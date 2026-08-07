#include "UserDataStorage.h"

#include <string>

#include <QCborValue>
#include <QVariantMap>

#include "keychain/keychain.h"
#include "glog/logging.h"

namespace {
constexpr auto PACKAGE = "com.dv.torrentplayer";
constexpr auto SERVICE = "torrentplayer";
constexpr auto USER_DATA = "userData";
}

using namespace TorrentPlayer;
QVariantMap CredentialStore::ReadUserData()
{
	keychain::Error err;
	const auto data = keychain::getPassword(PACKAGE, SERVICE, USER_DATA, err);
	if (err.type == keychain::ErrorType::NoError)
	{
		const auto res = QCborValue::fromCbor(QByteArray(data));
		if (!res.isMap())
		{
            LOG(WARNING) << "User data has to be a Map";
            return {};
        }
        return res.toVariant().toMap();
	}
	if (err)
	{
		LOG(WARNING) << "Keychain read failed:" << USER_DATA << err.message;
		return {};
	}
    LOG(WARNING) << "Could not read user data";
    return {};
}

void CredentialStore::StoreUserData(const QVariantMap & data)
{
    keychain::Error err;
	keychain::setPassword(PACKAGE, SERVICE, USER_DATA, QCborValue::fromVariant(data).toCbor().data(), err);
	if (err)
		LOG(WARNING) << "Keychain save failed:" << err.message;
}

void CredentialStore::ClearUserData()
{
	keychain::Error err;
	keychain::deletePassword(PACKAGE, SERVICE, USER_DATA, err);
}
