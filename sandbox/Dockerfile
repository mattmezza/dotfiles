FROM archlinux:base-devel

# 1. Install system dependencies & your shell
RUN pacman-key --init && \
    pacman -Syu --noconfirm && \
    pacman -S --noconfirm git zsh tmux neovim stow sudo openssh direnv curl

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

RUN sudo curl -L "https://github.com/anomalyco/opencode/releases/latest/download/opencode-linux-amd64" \
    -o /usr/local/bin/opencode && \
    sudo chmod +x /usr/local/bin/opencode

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
ENV TERM=xterm-256color

RUN <<EOF cat > /home/matteo/entrypoint.sh
#!/bin/zsh
SESSION_NAME=\${1:-sandbox}

# 1. Ensure the tmux server is running a session in the background
tmux has-session -t "\$SESSION_NAME" 2>/dev/null
if [ \$? != 0 ]; then
  tmux new-session -d -s "\$SESSION_NAME" -n "main" "/usr/bin/zsh"
fi

# 2. If we are in an interactive terminal, attach to that session
if [ -t 0 ]; then
  tmux attach-session -t "\$SESSION_NAME"
fi

# 3. CRITICAL: This line keeps the container alive after you detach
# or if the attach command above exits.
sleep infinity
EOF

RUN chmod +x /home/matteo/entrypoint.sh
ENTRYPOINT ["/home/matteo/entrypoint.sh"]
