#!/bin/bash
# Generate ECDSA certificates for ESP32

set -e

echo "Generating ECDSA P-256 certificates for ESP32..."

# Generate EC private key
openssl ecparam -genkey -name prime256v1 -out server_key.pem
echo "✓ Generated EC private key"

# Generate self-signed certificate (valid 10 years)
openssl req -new -x509 -key server_key.pem -out server_cert.pem -days 3650 \
    -subj "/C=SG/ST=Singapore/L=Singapore/O=Sharpie/CN=esp32.local"
echo "✓ Generated self-signed certificate"

# Convert to DER format
openssl x509 -in server_cert.pem -outform DER -out server_cert.der
openssl ec -in server_key.pem -outform DER -out server_key.der
echo "✓ Converted to DER format"

# Convert to C arrays
echo "// Auto-generated certificate data" > SslCertStore.h
echo "#pragma once" >> SslCertStore.h
echo "" >> SslCertStore.h

echo "unsigned char sslCertDER[] = {" >> SslCertStore.h
xxd -i < server_cert.der | sed 's/^/  /' | sed '$s/$/;/' >> SslCertStore.h
echo "};" >> SslCertStore.h
echo "unsigned int sslCertDERLen = sizeof(sslCertDER);" >> SslCertStore.h
echo "" >> SslCertStore.h

echo "unsigned char sslPrivateKeyDER[] = {" >> SslCertStore.h
xxd -i < server_key.der | sed 's/^/  /' | sed '$s/$/;/' >> SslCertStore.h
echo "};" >> SslCertStore.h
echo "unsigned int sslPrivateKeyDERLen = sizeof(sslPrivateKeyDER);" >> SslCertStore.h

echo "✓ Generated SslCertStore.h"

# Show certificate info
echo ""
echo "Certificate details:"
openssl x509 -in server_cert.pem -text -noout | grep -E "(Subject:|Not Before|Not After|Public-Key)"

# Show sizes
CERT_SIZE=$(stat -f%z server_cert.der 2>/dev/null || stat -c%s server_cert.der 2>/dev/null)
KEY_SIZE=$(stat -f%z server_key.der 2>/dev/null || stat -c%s server_key.der 2>/dev/null)
echo ""
echo "Certificate size: $CERT_SIZE bytes (was ~1024 bytes for RSA-2048)"
echo "Private key size: $KEY_SIZE bytes (was ~1216 bytes for RSA-2048)"
echo ""
echo "Copy SslCertStore.h to src/AppServer/ssl/"
