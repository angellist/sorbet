"""
@generated
cargo-raze crate build file.

DO NOT EDIT! Replaced on runs of cargo-raze
"""

package(default_visibility = [
    # Public for visibility by "@raze__crate__version//" targets.
    #
    # Prefer access through "//third_party/cargo", which limits external
    # visibility to explicit Cargo.toml dependencies.
    "//visibility:public",
])

licenses([
    "notice",  # MIT from expression "MIT OR Apache-2.0"
])

load(
    "@rules_rust//rust:rust.bzl",
    "rust_binary",
    "rust_library",
    "rust_test",
)

# Unsupported target "build-script-build" with type "custom-build" omitted

rust_library(
    name = "jemalloc_sys",
    srcs = glob(["**/*.rs"]),
    crate_features = [
        "background_threads_runtime_support",
        "disable_initial_exec_tls",
    ],
    crate_root = "src/lib.rs",
    crate_type = "lib",
    edition = "2015",
    rustc_flags = [
        "--cap-lints=allow",
    ],
    tags = ["cargo-raze"],
    version = "0.3.2",
    deps = [
        "@jemalloc//:jemalloc_impl",
        "@raze__libc__0_2_77//:libc",
    ],
)

# Unsupported target "malloc_conf_empty" with type "test" omitted
# Unsupported target "malloc_conf_set" with type "test" omitted
# Unsupported target "unprefixed_malloc" with type "test" omitted
