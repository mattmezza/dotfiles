function SetColorScheme(color)
	color = color or "rose-pine"
	vim.cmd.colorscheme(color)

	vim.api.nvim_set_hl(0, "Normal", { bg = "none" })
	vim.api.nvim_set_hl(0, "NormalFloat", { bg = "none" })
    vim.api.nvim_set_hl(0, "EndOfBuffer", { bg = "none" })
end

return {
    "nendix/zen.nvim",
    config = function()
        require("zen").setup {
            palette = {bg0 = "#000000"}
        }
        SetColorScheme("zen")
    end,
    lazy = false,
    priority = 1000,
}
-- return {
--     {
--         "rose-pine/neovim",
--         name = "rose-pine",
--         config = function()
--             require('rose-pine').setup({
--                 disable_background = true,
--             })
--             SetColorScheme()
--         end
--     },
-- }
