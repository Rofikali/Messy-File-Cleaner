# 🧠 Arch Linux Setup in GitHub Codespaces — Full Workflow

## 📌 Overview

This document records the complete process of setting up an Arch Linux environment inside GitHub Codespaces, configuring Git, resolving system issues, and successfully pushing code to GitHub.

---

# ⚙️ 1. Arch Linux Environment Setup

## 🔹 Update System

```bash
sudo pacman -Syu
```

**Explanation:**

* `-S` → sync/install
* `-y` → refresh package database
* `-u` → upgrade system

---

## 🔹 Install Core Development Tools

```bash
sudo pacman -S base-devel git neovim
sudo pacman -S less
sudo pacman -S git-lfs
git lfs install

git config --global user.email "alirofikr@gmail.com"
git config --global user.name "Rofik"


```

**Installed:**

* `base-devel` → compilers & build tools
* `git` → version control
* `neovim` → editor

---

## 🔹 Install Missing Utilities

```bash
sudo pacman -S less
```

**Why:**
Fixes Git pager error:

```
cannot run less: No such file or directory
```

---

# 🧠 2. Understanding Codespaces Architecture

* Running **Arch userspace inside a container**
* Not a full OS (no real boot system)
* Kernel controlled by host
* Some system services (systemd) are disabled

---

# 🔐 3. Fix Git “Dubious Ownership” Error

## Problem:

```
fatal: detected dubious ownership in repository
```

## Fix:

```bash
git config --global --add safe.directory /workspaces/SecureCleaner-Kernel
```

**Reason:**

* Repo owned by UID 1000 (host)
* Container user = root
* Git blocks mismatched ownership for security

---

# 👤 4. Configure Git Identity

## Problem:

```
Author identity unknown
```

## Fix:

```bash
git config --global user.email "alirofikr@gmail.com"
git config --global user.name "Rofik"
```

---

# 📦 5. Fix Git LFS Issue

## Problem:

```
Git LFS not found (pre-push hook failed)
```

## Fix:

```bash
sudo pacman -S git-lfs
git lfs install
```

---

# 🚀 6. Git Workflow

## Add Changes

```bash
git add .
```

## Commit

```bash
git commit -m "Updated OS to Arch Linux"
```

## Push

```bash
git push
```

---

# 🔑 7. Authentication (Important)

If prompted:

* Username → GitHub username
* Password → Personal Access Token (NOT your GitHub password)

---

# 🧠 8. Key Learnings

## 🔹 Arch vs Debian Commands

| Debian      | Arch |
| ----------- | ---- |
| apt update  | ❌    |
| pacman -Syu | ✅    |

---

## 🔹 Container Reality

* Not a full OS
* No systemd
* Limited service management

---

## 🔹 Git Security Model

* Based on file ownership (UID)
* Prevents execution of untrusted repositories

---

## 🔹 Multi-User Complexity

* root vs non-root environments behave differently
* Git config is per-user

---

# ⚠️ 9. Common Pitfalls

* ❌ Using `apt` commands in Arch
* ❌ Partial upgrades (`pacman -Sy`)
* ❌ Missing Git identity
* ❌ Ignoring Git LFS hooks
* ❌ Mixing multiple users (root vs custom user)

---

# 🏁 Final State

✅ Arch Linux running inside Codespaces
✅ Development tools installed
✅ Git configured correctly
✅ LFS working
✅ Repository successfully pushed

---

# 🚀 Next Steps (Optional)

* Create reproducible `.devcontainer`
* Optimize build toolchain
* Setup CI/CD pipeline
* Remove root dependency for cleaner architecture

---

# 📌 Summary

This setup demonstrates:

> Running a minimal, controlled Arch Linux development environment inside a containerized cloud system, while resolving real-world issues involving package management, filesystem ownership, authentication, and Git workflows.

---
