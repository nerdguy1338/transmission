// This file Copyright © 2007-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#ifndef TR_CRYPTO_UTILS_H
#define TR_CRYPTO_UTILS_H

#include <array>
#include <cstddef> // size_t
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "transmission.h" // tr_sha1_digest_t

/**
*** @addtogroup utils Utilities
*** @{
**/

class tr_sha1
{
public:
    static std::unique_ptr<tr_sha1> create();
    virtual ~tr_sha1() = default;

    virtual void clear() = 0;
    virtual void add(void const* data, size_t data_length) = 0;
    [[nodiscard]] virtual tr_sha1_digest_t finish() = 0;

    template<typename... T>
    [[nodiscard]] static tr_sha1_digest_t digest(T const&... args)
    {
        auto context = tr_sha1::create();
        (context->add(std::data(args), std::size(args)), ...);
        return context->finish();
    }
};

class tr_sha256
{
public:
    static std::unique_ptr<tr_sha256> create();
    virtual ~tr_sha256() = default;

    virtual void clear() = 0;
    virtual void add(void const* data, size_t data_length) = 0;
    [[nodiscard]] virtual tr_sha256_digest_t finish() = 0;

    template<typename... T>
    [[nodiscard]] static tr_sha256_digest_t digest(T const&... args)
    {
        auto context = tr_sha256::create();
        (context->add(std::data(args), std::size(args)), ...);
        return context->finish();
    }
};

/** @brief Opaque SSL context type. */
using tr_ssl_ctx_t = void*;
/** @brief Opaque X509 certificate store type. */
using tr_x509_store_t = void*;
/** @brief Opaque X509 certificate type. */
using tr_x509_cert_t = void*;

/**
 * @brief Get X509 certificate store from SSL context.
 */
tr_x509_store_t tr_ssl_get_x509_store(tr_ssl_ctx_t handle);

/**
 * @brief Add certificate to X509 certificate store.
 */
bool tr_x509_store_add(tr_x509_store_t handle, tr_x509_cert_t cert);

/**
 * @brief Allocate and initialize new X509 certificate from DER-encoded buffer.
 */
tr_x509_cert_t tr_x509_cert_new(void const* der, size_t der_length);

/**
 * @brief Free X509 certificate returned by @ref tr_x509_cert_new.
 */
void tr_x509_cert_free(tr_x509_cert_t handle);

/**
 * @brief Returns a random number in the range of [0...upper_bound).
 */
template<class IntType>
[[nodiscard]] IntType tr_rand_int(IntType upper_bound);

/**
 * @brief Returns a pseudorandom number in the range of [0...upper_bound).
 *
 * This is faster, BUT WEAKER, than tr_rand_int() and never be used in sensitive cases.
 * @see tr_rand_int()
 */
template<class IntType>
[[nodiscard]] IntType tr_rand_int_weak(IntType upper_bound);

/**
 * @brief Fill a buffer with random bytes.
 */
bool tr_rand_buffer(void* buffer, size_t length);

template<typename T>
T tr_rand_obj()
{
    auto t = T{};
    tr_rand_buffer(&t, sizeof(T));
    return t;
}

/**
 * @brief Generate a SSHA password from its plaintext source.
 */
[[nodiscard]] std::string tr_ssha1(std::string_view plaintext);

/**
 * @brief Return true if this is salted text, false otherwise
 */
[[nodiscard]] bool tr_ssha1_test(std::string_view text);

/**
 * @brief Validate a test password against the a ssha1 password.
 */
[[nodiscard]] bool tr_ssha1_matches(std::string_view ssha1, std::string_view plaintext);

/**
 * @brief Translate null-terminated string into base64.
 * @return a new std::string with the encoded contents
 */
[[nodiscard]] std::string tr_base64_encode(std::string_view input);

/**
 * @brief Translate a character range from base64 into raw form.
 * @return a new std::string with the decoded contents.
 */
[[nodiscard]] std::string tr_base64_decode(std::string_view input);

/**
 * @brief Generate an ascii hex string for a sha1 digest.
 */
[[nodiscard]] std::string tr_sha1_to_string(tr_sha1_digest_t const&);

/**
 * @brief Generate a sha1 digest from a hex string.
 */
[[nodiscard]] std::optional<tr_sha1_digest_t> tr_sha1_from_string(std::string_view hex);

/**
 * @brief Generate an ascii hex string for a sha256 digest.
 */
[[nodiscard]] std::string tr_sha256_to_string(tr_sha256_digest_t const&);

/**
 * @brief Generate a sha256 digest from a hex string.
 */
[[nodiscard]] std::optional<tr_sha256_digest_t> tr_sha256_from_string(std::string_view hex);

// Convenience utility to efficiently get many random small values.
// Use this instead of making a lot of calls to tr_rand_int().
template<typename T = uint8_t, size_t N = 1024U>
class tr_salt_shaker
{
public:
    [[nodiscard]] auto operator()() noexcept
    {
        if (pos == std::size(buf))
        {
            pos = 0U;
        }

        if (pos == 0U)
        {
            tr_rand_buffer(std::data(buf), std::size(buf) * sizeof(T));
        }

        return buf[pos++];
    }

private:
    size_t pos = 0;
    std::array<T, N> buf;
};

/** @} */

#endif /* TR_CRYPTO_UTILS_H */
