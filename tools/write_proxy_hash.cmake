file(SHA256 "${INPUT}" DIGEST)
file(WRITE "${OUTPUT}" "#pragma once\ninline constexpr char bundled_proxy_sha256[]=\"${DIGEST}\";\n")
