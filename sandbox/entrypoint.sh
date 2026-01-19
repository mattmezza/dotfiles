#!/bin/zsh
SESSION_NAME=${1:-sandbox}

# Start SSH agent and add keys if mounted
if [[ -d ~/.ssh ]] && ls ~/.ssh/id_* &>/dev/null; then
    eval "$(ssh-agent -s)" >/dev/null
    ssh-add ~/.ssh/id_* 2>/dev/null
fi

# Authenticate gh CLI if token provided
if [[ -n "$GH_TOKEN" ]]; then
    echo "$GH_TOKEN" | gh auth login --with-token 2>/dev/null
fi

# Ensure tmux session exists
tmux has-session -t "$SESSION_NAME" 2>/dev/null
if [[ $? != 0 ]]; then
    tmux new-session -d -s "$SESSION_NAME" -n main -c /workspace /usr/bin/zsh
fi

# Attach if interactive
[[ -t 0 ]] && tmux attach-session -t "$SESSION_NAME"

# Keep container alive
sleep infinity
