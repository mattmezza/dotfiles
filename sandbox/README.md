# Sandbox

Docker-based development sandboxes with persistent workspaces, AI CLI tools, and dotfiles pre-configured.

## Quick Start

```bash
cd ~/dotfiles/sandbox

# 1. Set up secrets
cp .env.example .env
# Edit .env with your API keys

# 2. Build and start
docker compose -f compose.dev.yaml build
docker compose -f compose.dev.yaml up -d

# 3. Attach
docker exec -it sandbox-dev tmux attach-session -t sandbox-dev
```

## Setup

### Secrets

Copy the example file and fill in your keys:

```bash
cp .env.example .env
```

Required variables:
- `ANTHROPIC_API_KEY` - For claude-code
- `OPENAI_API_KEY` - For opencode
- `GH_TOKEN` - For GitHub CLI authentication

### SSH Keys

SSH keys from `~/.ssh` are mounted read-only into the container. The entrypoint automatically starts an SSH agent and adds your keys.

## Sandbox Variants

### Dev (Full)

Full development environment with AI CLIs (claude-code, opencode), neovim, ripgrep, gh, and all dotfiles.

```bash
docker compose -f compose.dev.yaml build
docker compose -f compose.dev.yaml up -d
```

### Minimal

Lightweight sandbox without AI CLIs or heavy tooling. Includes git, zsh, tmux, and neovim only.

```bash
docker compose -f compose.minimal.yaml build
docker compose -f compose.minimal.yaml up -d
```

## Usage

### Start a sandbox

```bash
docker compose -f compose.dev.yaml up -d
```

### Attach to tmux session

```bash
docker exec -it sandbox-dev tmux attach-session -t sandbox-dev
```

### One-liner start + attach

```bash
docker compose -f compose.dev.yaml up -d && docker exec -it sandbox-dev tmux attach-session -t sandbox-dev
```

### Stop (preserves volume)

```bash
docker compose -f compose.dev.yaml stop
```

### Stop and remove container

```bash
docker compose -f compose.dev.yaml down
```

### Rebuild image

```bash
docker compose -f compose.dev.yaml build --no-cache
```

## Workspace Persistence

Each sandbox has a named volume for `/workspace`:
- `sandbox-dev-workspace` for dev
- `sandbox-minimal-workspace` for minimal

Files in `/workspace` persist across container restarts and rebuilds.

### List volumes

```bash
docker volume ls | grep sandbox
```

### Remove a volume (deletes all workspace data)

```bash
docker volume rm sandbox-dev-workspace
```

## Creating Custom Sandboxes

Copy an existing compose file and modify:

```bash
cp compose.dev.yaml compose.myproject.yaml
```

Edit the file to change:
- Service name (`dev` -> `myproject`)
- `container_name` and `hostname`
- Volume name (`sandbox-dev-workspace` -> `sandbox-myproject-workspace`)

Then run:

```bash
docker compose -f compose.myproject.yaml up -d
```

## Verification

Once inside the container, verify everything works:

```bash
# SSH keys
ssh -T git@github.com

# GitHub CLI
gh auth status

# API keys loaded
echo $ANTHROPIC_API_KEY

# Working directory
pwd  # Should show /workspace

# AI CLIs (dev sandbox only)
claude --version
opencode --version
```

## Troubleshooting

### Permission issues with volumes

The Dockerfile uses `USER_UID=1000` and `USER_GID=1000` by default. If your host user has different IDs, rebuild with:

```bash
docker compose -f compose.dev.yaml build --build-arg USER_UID=$(id -u) --build-arg USER_GID=$(id -g)
```

### SSH agent not working

Ensure your SSH keys exist in `~/.ssh` and have standard names (`id_*`). The entrypoint looks for these patterns.

### gh CLI not authenticated

Make sure `GH_TOKEN` is set in your `.env` file. You can generate a token at https://github.com/settings/tokens.
