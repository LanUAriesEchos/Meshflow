# Auto-update

Meshflow checks for new versions on startup and can download and install updates automatically.

## How it works

1. On launch, Meshflow queries the GitHub releases API for the latest version tag.
2. If a newer version exists, a notification appears in the status bar.
3. Clicking the notification opens a confirmation dialog.
4. On confirm, Meshflow downloads the latest installer for your platform.
5. The SHA-256 checksum is verified against `sums.txt` from the release.
6. The installer runs silently and Meshflow restarts.

## Platform behavior

| Platform | Installer | Silent flag |
|----------|-----------|-------------|
| Windows | MSI | `msiexec /i ... /quiet /norestart` |
| Linux (DEB) | `.deb` | `sudo apt install -y ./meshflow-*.deb` via pkexec |
| Linux (RPM) | `.rpm` | `sudo dnf install -y meshflow-*.rpm` via pkexec |
| Linux (tar.gz) | Archive | Manual extraction prompt |

## Manual check

You can also check for updates manually from the menu: **Help > Check for updates**.

## Disabling update checks

Add the following to your Meshflow config file to disable update checks on startup:

```ini
[updates]
enabled = false
```

## Behind a proxy

If your network requires a proxy, set the `HTTPS_PROXY` environment variable before launching Meshflow:

```bash
export HTTPS_PROXY=http://proxy.local:8080
meshflow
```

## Troubleshooting

**"Could not check for updates"** - Meshflow could not reach `api.github.com`. Check your network connection, proxy settings, or firewall.

**"Checksum verification failed"** - The downloaded file was corrupted or tampered with. Delete the cached download and try again.

**"Update failed to install"** - On Linux, make sure `pkexec` is available and your user has permission to install packages. On Windows, make sure you have administrator privileges.
