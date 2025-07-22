return {
    "lewis6991/gitsigns.nvim",
    version = "v0.7", -- Use for stability; omit to use `main` branch for the latest features
    event = "VeryLazy",
    config = function()
        require("gitsigns").setup({
            signs = {
                add          = { text = '+' },
                change       = { text = '│' },
                delete       = { text = '_' },
                topdelete    = { text = '‾' },
                changedelete = { text = '~' },
                untracked    = { text = '┆' },
            },
        })
    end
}
