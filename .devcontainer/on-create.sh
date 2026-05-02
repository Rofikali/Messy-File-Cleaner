#!/usr/bin/env bash
# ============================================================
# on-create.sh — Runs ONCE when container is first created
# ============================================================
set -euo pipefail

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  🐧 Fedora 44 — Container Initializing"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# ── Verify OS ────────────────────────────────────────────────
echo "📦 OS: $(cat /etc/os-release | grep PRETTY_NAME | cut -d= -f2)"
echo "🧠 Kernel: $(uname -r)"
echo "👤 User: $(whoami)"

# ── Configure Git identity from Codespace env ────────────────
if [ -n "${GITHUB_USER:-}" ]; then
  git config --global user.name "$GITHUB_USER"
  echo "✅ Git user set: $GITHUB_USER"
fi

# ── Fix SSH key permissions if mounted ──────────────────────
if [ -d "$HOME/.ssh" ]; then
  chmod 700 "$HOME/.ssh"
  find "$HOME/.ssh" -type f -name "*.pub" -exec chmod 644 {} \;
  find "$HOME/.ssh" -type f ! -name "*.pub" ! -name "known_hosts" ! -name "config" -exec chmod 600 {} \; 2>/dev/null || true
  echo "✅ SSH key permissions fixed"
fi

echo "✅ on-create complete"