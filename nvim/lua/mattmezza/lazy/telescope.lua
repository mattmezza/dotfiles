return {
    "nvim-telescope/telescope.nvim",
    branch = "0.1.x",
    lazy = true,
    cmd = { "Telescope" },
    dependencies = {
        "nvim-lua/plenary.nvim"
    },
    config = function()
        local telescope = require('telescope')
        telescope.setup({})

        local builtin = require('telescope.builtin')
        vim.keymap.set('n', '<leader>ff', builtin.find_files, {})
        vim.keymap.set('n', '<leader>fg', builtin.git_files, {})
        vim.keymap.set('n', '<leader>fs', builtin.live_grep, {})
        vim.keymap.set('n', '<leader>fw', function()
            local word = vim.fn.expand("<cword>")
            builtin.grep_string({ search = word })
        end)
        vim.keymap.set('n', '<leader>fW', function()
            local word = vim.fn.expand("<cWORD>")
            builtin.grep_string({ search = word })
        end)
        vim.keymap.set('n', '<leader>fh', builtin.help_tags, {})
        -- Enable telescope fzf native, if installed
		pcall(telescope.load_extension, "fzf")
end
}

