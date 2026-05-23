#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/datetime.hpp>

namespace taxi::jwt {

namespace detail {

inline bool ConstantTimeEqual(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    unsigned diff = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a[i]) ^
                static_cast<unsigned char>(b[i]);
    }
    return diff == 0;
}

}  // namespace detail

inline std::string Encode(std::string_view login,
                          std::string_view secret,
                          int64_t ttl_seconds) {
    namespace b64 = userver::crypto::base64;
    namespace hash = userver::crypto::hash;

    const auto exp =
        std::chrono::duration_cast<std::chrono::seconds>(
            userver::utils::datetime::Now().time_since_epoch())
            .count() +
        ttl_seconds;

    static const std::string kHeader = R"({"alg":"HS256","typ":"JWT"})";
    const auto payload_str = userver::formats::json::ToString(
        userver::formats::json::MakeObject(
            "sub", std::string(login),
            "exp", exp));

    const auto h = b64::Base64UrlEncode(kHeader, b64::Pad::kWithout);
    const auto p = b64::Base64UrlEncode(payload_str, b64::Pad::kWithout);
    const std::string signing_input = h + "." + p;
    const auto sig = hash::HmacSha256(secret, signing_input,
                                      hash::OutputEncoding::kBinary);
    return signing_input + "." +
           b64::Base64UrlEncode(sig, b64::Pad::kWithout);
}

inline std::optional<std::string> Decode(std::string_view token,
                                         std::string_view secret) {
    namespace b64 = userver::crypto::base64;
    namespace hash = userver::crypto::hash;

    const auto p1 = token.find('.');
    if (p1 == std::string_view::npos) return std::nullopt;
    const auto p2 = token.find('.', p1 + 1);
    if (p2 == std::string_view::npos) return std::nullopt;

    const auto signing_input = token.substr(0, p2);
    const auto sig_b64       = token.substr(p2 + 1);

    std::string sig;
    std::string payload_str;
    try {
        sig = b64::Base64UrlDecode(sig_b64);
        payload_str =
            b64::Base64UrlDecode(token.substr(p1 + 1, p2 - p1 - 1));
    } catch (const std::exception&) {
        return std::nullopt;
    }

    const auto expected = hash::HmacSha256(secret, signing_input,
                                           hash::OutputEncoding::kBinary);
    if (!detail::ConstantTimeEqual(sig, expected)) return std::nullopt;

    userver::formats::json::Value payload;
    try {
        payload = userver::formats::json::FromString(payload_str);
    } catch (const std::exception&) {
        return std::nullopt;
    }

    const auto exp = payload["exp"].As<int64_t>(0);
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                         userver::utils::datetime::Now().time_since_epoch())
                         .count();
    if (exp != 0 && now >= exp) return std::nullopt;

    auto sub = payload["sub"].As<std::string>("");
    if (sub.empty()) return std::nullopt;
    return sub;
}

inline std::string HashPassword(std::string_view password,
                                std::string_view pepper) {
    std::string buf;
    buf.reserve(pepper.size() + password.size());
    buf.append(pepper).append(password);
    return userver::crypto::hash::Sha256(
        buf, userver::crypto::hash::OutputEncoding::kBase16);
}

}  // namespace taxi::jwt
