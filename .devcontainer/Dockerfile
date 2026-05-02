# ============================================================
# Fedora 44 — Professional Dev Container
# DNF5 | Released April 2026
# ============================================================
FROM fedora:44

LABEL maintainer="devcontainer"
LABEL description="Production-grade Fedora 44 dev environment"
LABEL version="1.0.0"

# System upgrade
RUN dnf5 upgrade -y --refresh && \
    dnf5 clean all

# Install all packages — NO inline comments inside this block
RUN dnf5 install -y --skip-unavailable \
    zsh \
    bash-completion \
    tmux \
    git \
    git-lfs \
    wget \
    unzip \
    zip \
    gcc \
    gcc-c++ \
    make \
    cmake \
    ninja-build \
    pkg-config \
    python3 \
    python3-pip \
    python3-devel \
    nodejs \
    npm \
    htop \
    tree \
    jq \
    ripgrep \
    fd-find \
    bat \
    iproute \
    iputils \
    nmap \
    openssh-clients \
    neovim \
    nano \
    man-db \
    man-pages \
    sudo \
    gh \
    && dnf5 clean all \
    && rm -rf /var/cache/dnf

# Create non-root vscode user
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd --gid $USER_GID $USERNAME && \
    useradd --uid $USER_UID --gid $USER_GID --shell /bin/zsh --create-home $USERNAME && \
    echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME

# Switch to vscode user
USER $USERNAME
WORKDIR /home/$USERNAME

# Install Oh My Zsh
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended

# Configure .zshrc using printf
RUN printf '\nexport EDITOR=nvim\nexport VISUAL=nvim\nexport TERM=xterm-256color\nexport PATH="$HOME/.local/bin:$HOME/.cargo/bin:$PATH"\nalias ll="ls -alF --color=auto"\nalias g="git"\nalias gs="git status"\nalias gp="git pull"\nalias install="sudo dnf5 install -y"\nalias update="sudo dnf5 upgrade -y"\n' >> /home/$USERNAME/.zshrc

# Git global config
RUN git config --global core.editor "nvim" && \
    git config --global init.defaultBranch "main" && \
    git config --global pull.rebase false && \
    git config --global color.ui auto

USER root

CMD ["/bin/zsh"]