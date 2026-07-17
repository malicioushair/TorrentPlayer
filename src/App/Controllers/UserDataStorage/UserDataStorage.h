#pragma once

#include <keychain/keychain.h>

#include <QString>

namespace TorrentPlayer::CredentialStore {

QVariantMap ReadUserData();
void StoreUserData(const QVariantMap & data);
void ClearUserData();

}