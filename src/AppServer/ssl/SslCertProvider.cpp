#include "SslCertProvider.hpp"

SSLCert SslCertProvider::cert =
    SSLCert(sslCertDER, sslCertDERLen, sslPrivateKeyDER, sslPrivateKeyDERLen);
