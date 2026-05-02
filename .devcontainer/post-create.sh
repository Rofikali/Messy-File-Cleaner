#!/usr/bin/env bash
# ============================================================
# post-create.sh — Runs after every rebuild
# ============================================================
set -euo pipefail

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ⚙️  Fedora 44 — Post Create Setup"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# ── Upgrade system packages via DNF5 ────────────────────────
echo "🔄 Upgrading system packages..."
sudo dnf5 upgrade -y --refresh --quiet
echo "✅ System packages up to date"

# ── Install pip tools ────────────────────────────────────────
echo "🐍 Configuring Python..."
pip3 install --user --quiet \
  pipenv \
  black \
  isort \
  flake8 \
  mypy \
  httpie \
  pre-commit
echo "✅ Python tools installed"

# ── Install Node global tools ────────────────────────────────
echo "📦 Configuring Node.js..."
npm install -g --silent \
  pnpm \
  yarn \
  typescript \
  ts-node \
  eslint \
  prettier
echo "✅ Node.js tools installed"

# ── Install Zsh plugins ──────────────────────────────────────
echo "🐚 Installing Zsh plugins..."
ZSH_CUSTOM="${ZSH_CUSTOM:-$HOME/.oh-my-zsh/custom}"

if [ ! -d "$ZSH_CUSTOM/plugins/zsh-autosuggestions" ]; then
  git clone --quiet https://github.com/zsh-users/zsh-autosuggestions \
    "$ZSH_CUSTOM/plugins/zsh-autosuggestions"
fi

if [ ! -d "$ZSH_CUSTOM/plugins/zsh-syntax-highlighting" ]; then
  git clone --quiet https://github.com/zsh-users/zsh-syntax-highlighting \
    "$ZSH_CUSTOM/plugins/zsh-syntax-highlighting"
fi

# Enable plugins in .zshrc
sed -i 's/plugins=(git)/plugins=(git zsh-autosuggestions zsh-syntax-highlighting docker kubectl npm python)/' \
  "$HOME/.zshrc" 2>/dev/null || true

echo "✅ Zsh plugins ready"

# ── Git LFS setup ────────────────────────────────────────────
git lfs install --quiet 2>/dev/null || true
echo "✅ Git LFS configured"

# ── Project-specific setup ───────────────────────────────────
# Python project
if [ -f "requirements.txt" ]; then
  echo "📦 Installing Python requirements..."
  pip3 install --user -r requirements.txt --quiet
fi

if [ -f "pyproject.toml" ]; then
  echo "📦 Installing pyproject dependencies..."
  pip3 install --user -e ".[dev]" --quiet 2>/dev/null || true
fi

# Node project
if [ -f "package.json" ]; then
  echo "📦 Installing npm dependencies..."
  npm install --silent
fi

# Pre-commit hooks
if [ -f ".pre-commit-config.yaml" ]; then
  echo "🪝 Installing pre-commit hooks..."
  pre-commit install --quiet
fi

# ── Final summary ────────────────────────────────────────────
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  ✅ Fedora 44 Dev Container Ready!"
echo "  🐧 $(cat /etc/os-release | grep PRETTY_NAME | cut -d= -f2 | tr -d '\"')"
echo "  🧠 Kernel: $(uname -r)"
echo "  🐍 Python: $(python3 --version)"
echo "  📦 Node:   $(node --version)"
echo "  🔧 DNF5:   $(dnf5 --version | head -1)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"