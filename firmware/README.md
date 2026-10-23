# Experimental prebuilt firmware

These are the locally compiled 0.1.0 release inputs, retained so GitHub Actions
can upload the tested bytes to Releases. They have not been run on physical
hardware. Do not flash before reading the main README and hardware checklist.

`docs/BUILD_REPORT.json` records hashes of these images, the relevant source,
and the two browser binaries. The release workflow verifies those hashes,
packages these files with full pinned source and licenses, then publishes a
prerelease. That job packages existing builds; it does not rebuild the target.

Build the device from source using the documented JieLi toolchain, or use
`tools/package_release.py` after a local device build. The `.fwsc` is the update
package; the `.bin` is a raw application image, not a standalone installer.
