# Strategy Matrix

The scripts in this directory will generate a strategy matrix for GitHub Actions
CI, depending on the trigger that caused the workflow to run and the platform
specified.

There are several build, test, and publish settings that can be enabled for each
configuration. The settings are combined in a Cartesian product to generate the
full matrix, while filtering out any combinations not applicable to the trigger.

## Platforms

We support three platforms: Linux, macOS, and Windows.

### Linux

We support a variety of distributions (Debian, RHEL, and Ubuntu) and compilers
(GCC and Clang) on Linux. As there are so many combinations, we don't run them
all. Instead, we focus on a few key ones for PR commits and merges, while we run
most of them on a scheduled or ad hoc basis.

Some noteworthy configurations are:

- The official release build is GCC 14 on Debian Bullseye.
  - Although we generally enable assertions in release builds, we disable them
    for the official release build.
  - We publish .deb and .rpm packages for this build, as well as a Docker image.
  - For PR commits we also publish packages and images for testing purposes.
- Antithesis instrumentation is only supported on Clang 16+ on AMD64.
  - We publish a Docker image for this build, but no packages.
- Coverage reports are generated on Bullseye with GCC 15.
  - It must be enabled for both commits (to show PR coverage) and merges (to
    show default branch coverage).

Note that we try to run pipelines equally across both AMD64 and ARM64, but in
some cases we cannot build on ARM64:

- All Clang 20+ builds on ARM64 are currently skipped due to a Boost build
  error.
- All RHEL builds on AMD64 are currently skipped due to a build failure that
  needs further investigation.

Also note that to create a Docker image we ideally build on both AMD64 and
ARM64 to create a multi-arch image. Both configs should therefore be triggered
by the same event. However, as the script outputs individual configs, the
workflow must be able to run both builds separately and then merge the
single-arch images afterwards into a multi-arch image.

### MacOS

We support building on macOS, which uses the Apple Clang compiler and the ARM64
architecture. We use default settings for all builds, and don't publish any
packages or images.

### Windows

We also support building on Windows, which uses the MSVC compiler and the AMD64
architecture. While we could build on ARM64, we have not yet found a suitable
cloud machine to use as a GitHub runner. We use default settings for all builds,
and don't publish any packages or images.

## Triggers

We have four triggers that can cause the workflow to run:

- `commit`: A commit is pushed to a pull request.
- `merge`: A pull request is merged.
- `label`: A label is added to a pull request.
- `schedule`: The workflow is run on a scheduled basis.

The `label` trigger is currently not used, but it is reserved for future use.
The `schedule` trigger is used to run the workflow each weekday, and is also
used for ad hoc testing via the `workflow_dispatch` event.

## Usage

Our GitHub CI pipeline uses the `generate.py` script to generate the matrix for
the current workflow invocation. Naturally, the script can be run locally to
generate the matrix for testing purposes, e.g.:

```bash
python3 generate.py --platform=linux --trigger=commit
```

If you want to pretty-print the output, you can pipe it to `jq` after stripping
off the `matrix=` prefix, e.g.:

```bash
python3 generate.py --platform=linux --trigger=commit | cut -d= -f2- | jq
```
