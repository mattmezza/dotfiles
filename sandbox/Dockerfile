FROM archlinux:base-devel

# 1. Install system dependencies & your shell
RUN pacman-key --init && \
    pacman -Syu --noconfirm && \
    pacman -S --noconfirm git zsh tmux neovim stow sudo openssh direnv

RUN echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen && \
    locale-gen
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

# 2. Create a non-root user (to mirror your setup)
ARG USERNAME=matteo
RUN useradd -m -G wheel -s /bin/zsh $USERNAME && \
    echo "$USERNAME ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
USER $USERNAME
WORKDIR /home/$USERNAME

# 3. Clone and apply dotfiles
RUN git clone https://github.com/mattmezza/dotfiles.git dotfiles && \
    cd dotfiles && \
    stow git nvim tmux zsh

RUN mkdir -p /home/matteo/dotfiles/pacchi/pacco && \
    curl -s -L https://raw.githubusercontent.com/mattmezza/pacco/master/pacco.sh \
    -o /home/matteo/dotfiles/pacchi/pacco/pacco.sh && \
    chmod +x /home/matteo/dotfiles/pacchi/pacco/pacco.sh

RUN GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" bash /home/matteo/dotfiles/pacchi/pacco/pacco.sh i pacco https://github.com/mattmezza/pacco.git 1.0.0

SHELL ["/usr/bin/zsh", "-c"]
# Set ZSH as default
# ENTRYPOINT ["/bin/zsh", "-l"]
# Or directly start in tmux
ENTRYPOINT ["/bin/zsh", "-c", "tmux new-session -d -s sandbox; sleep infinity"]
